/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbDeviceFirmwareDownloader.h"

#include <nsIFile.h>
#include <nsIHttpChannel.h>
#include <nsIIOService.h>
#include <nsILocalFile.h>
#include <nsIMultiPartChannel.h>
#include <nsIProperties.h>
#include <nsIPropertyBag2.h>
#include <nsITextToSubURI.h>
#include <nsIURL.h>
#include <nsIUTF8ConverterService.h>
#include <nsIVariant.h>

#include <nsAppDirectoryServiceDefs.h>
#include <nsAutoLock.h>
#include <nsCRT.h>
#include <nsHashKeys.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOM.h>
#include <prmem.h>

#include <sbIDevice.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceProperties.h>

#include <sbProxiedComponentManager.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

#include "sbDeviceFirmwareUpdate.h"

#define FIRMWARE_FILE_PREF      "firmware.cache.file"
#define FIRMWARE_VERSION_PREF   "firmware.cache.version"
#define FIRMWARE_READABLE_PREF  "firmware.cache.readableVersion"

static PRInt32
codetovalue( unsigned char c )
{
  if( (c >= (unsigned char)'A') && (c <= (unsigned char)'Z') ) {
    return (PRInt32)(c - (unsigned char)'A');
  }
  else if( (c >= (unsigned char)'a') && (c <= (unsigned char)'z') ) {
    return ((PRInt32)(c - (unsigned char)'a') +26);
  }
  else if( (c >= (unsigned char)'0') && (c <= (unsigned char)'9') ) {
    return ((PRInt32)(c - (unsigned char)'0') +52);
  }
  else if( (unsigned char)'+' == c ) {
    return (PRInt32)62;
  }
  else if( (unsigned char)'/' == c ) {
    return (PRInt32)63;
  }

  return -1;
}

static PRStatus
decode4to3( const unsigned char    *src,
            unsigned char          *dest )
{
  PRUint32 b32 = (PRUint32)0;
  PRInt32 bits = 0;
  PRIntn i = 0;

  for( i = 0; i < 4; i++ ) {
    bits = codetovalue(src[i]);
    if( bits < 0 ) {
      return PR_FAILURE;
    }
    b32 <<= 6;
    b32 |= bits;
  }

  dest[0] = (unsigned char)((b32 >> 16) & 0xFF);
  dest[1] = (unsigned char)((b32 >>  8) & 0xFF);
  dest[2] = (unsigned char)((b32      ) & 0xFF);

  return PR_SUCCESS;
}

static PRStatus
decode3to2( const unsigned char    *src,
            unsigned char          *dest )
{
  PRUint32 b32 = (PRUint32)0;
  PRInt32 bits = 0;
  PRUint32 ubits = 0;

  bits = codetovalue(src[0]);
  if( bits < 0 ) {
    return PR_FAILURE;
  }

  b32 = (PRUint32)bits;
  b32 <<= 6;

  bits = codetovalue(src[1]);
  if( bits < 0 ) {
    return PR_FAILURE;
  }

  b32 |= (PRUint32)bits;
  b32 <<= 4;

  bits = codetovalue(src[2]);
  if( bits < 0 ) {
    return PR_FAILURE;
  }

  ubits = (PRUint32)bits;
  b32 |= (ubits >> 2);

  dest[0] = (unsigned char)((b32 >> 8) & 0xFF);
  dest[1] = (unsigned char)((b32     ) & 0xFF);

  return PR_SUCCESS;
}

static PRStatus
decode2to1( const unsigned char    *src,
            unsigned char          *dest )
{
  PRUint32 b32 = 0;
  PRUint32 ubits = 0;
  PRInt32 bits = 0;

  bits = codetovalue(src[0]);
  if( bits < 0 ) {
    return PR_FAILURE;
  }

  ubits = (PRUint32)bits;
  b32 = (ubits << 2);

  bits = codetovalue(src[1]);
  if( bits < 0 ) {
    return PR_FAILURE;
  }

  ubits = (PRUint32)bits;
  b32 |= (ubits >> 4);

  dest[0] = (unsigned char)b32;

  return PR_SUCCESS;
}

static PRStatus
decode( const unsigned char    *src,
        PRUint32                srclen,
        unsigned char          *dest )
{
  PRStatus rv = PR_SUCCESS;

  while( srclen >= 4 ) {
    rv = decode4to3(src, dest);
    if( PR_SUCCESS != rv ) {
      return PR_FAILURE;
    }

    src += 4;
    dest += 3;
    srclen -= 4;
  }

  switch( srclen ) {
  case 3:
    rv = decode3to2(src, dest);
    break;
  case 2:
    rv = decode2to1(src, dest);
    break;
  case 1:
    rv = PR_FAILURE;
    break;
  case 0:
    rv = PR_SUCCESS;
    break;
  default:
    PR_NOT_REACHED("coding error");
  }

  return rv;
}

static char *
SB_Base64Decode( const char *src,
                 PRUint32    srclen,
                 char       *dest )
{
  PRStatus status;
  PRBool allocated = PR_FALSE;

  if( (char *)0 == src ) {
    return (char *)0;
  }

  if( 0 == srclen ) {
    srclen = strlen(src);
  }

  if( srclen && (0 == (srclen & 3)) ) {
    if( (char)'=' == src[ srclen-1 ] ) {
      if( (char)'=' == src[ srclen-2 ] ) {
        srclen -= 2;
      }
      else {
        srclen -= 1;
      }
    }
  }

  if( (char *)0 == dest ) {
    PRUint32 destlen = ((srclen * 3) / 4);
    dest = (char *)PR_MALLOC(destlen + 1);
    if( (char *)0 == dest ) {
      return (char *)0;
    }
    dest[ destlen ] = (char)0; /* null terminate */
    allocated = PR_TRUE;
  }

  status = decode((const unsigned char *)src, srclen, (unsigned char *)dest);
  if( PR_SUCCESS != status ) {
    if( PR_TRUE == allocated ) {
      PR_DELETE(dest);
    }
    return (char *)0;
  }

  return dest;
}

static nsCString
GetContentDispositionFilename(const nsACString &contentDisposition)
{
  NS_NAMED_LITERAL_CSTRING(DISPOSITION_ATTACHEMENT, "attachment");
  NS_NAMED_LITERAL_CSTRING(DISPOSITION_FILENAME, "filename=");

  nsCAutoString unicodeDisposition(contentDisposition);
  unicodeDisposition.StripWhitespace();

  PRInt32 pos = unicodeDisposition.Find(DISPOSITION_ATTACHEMENT,
    CaseInsensitiveCompare);
  if(pos == -1 )
    return EmptyCString();

  pos = unicodeDisposition.Find(DISPOSITION_FILENAME,
    CaseInsensitiveCompare);
  if(pos == -1)
    return EmptyCString();

  pos += DISPOSITION_FILENAME.Length();

  // if the string is quoted, we look for the next quote to know when the
  // filename ends.
  PRInt32 endPos = -1;
  if(unicodeDisposition.CharAt(pos) == '"') {

    pos++;
    endPos = unicodeDisposition.FindChar('\"', pos);

    if(endPos == -1)
      return EmptyCString();
  }
  // if not, we find the next ';' or we take the rest.
  else {
    endPos = unicodeDisposition.FindChar(';', pos);

    if(endPos == -1)  {
      endPos = unicodeDisposition.Length();
    }
  }

  nsCString filename(Substring(unicodeDisposition, pos, endPos - pos));

  // string is encoded in a different character set.
  if(StringBeginsWith(filename, NS_LITERAL_CSTRING("=?")) &&
     StringEndsWith(filename, NS_LITERAL_CSTRING("?="))) {

    nsresult rv;
    nsCOMPtr<nsIUTF8ConverterService> convServ =
      do_GetService("@mozilla.org/intl/utf8converterservice;1", &rv);
    NS_ENSURE_SUCCESS(rv, EmptyCString());

    pos = 2;
    endPos = filename.FindChar('?', pos);

    if(endPos == -1)
      return EmptyCString();

    // found the charset
    nsCAutoString charset(Substring(filename, pos, endPos - pos));
    pos = endPos + 1;

    // find what encoding for the character set is used.
    endPos = filename.FindChar('?', pos);

    if(endPos == -1)
      return EmptyCString();

    nsCAutoString encoding(Substring(filename, pos, endPos - pos));
    pos = endPos + 1;

    ToLowerCase(encoding);

    // bad encoding.
    if(!encoding.EqualsLiteral("b") &&
       !encoding.EqualsLiteral("q")) {
      return EmptyCString();
    }

    // end of actual string to decode marked by ?=
    endPos = filename.FindChar('?', pos);
    // didn't find end, bad string
    if(endPos == -1 ||
       filename.CharAt(endPos + 1) != '=')
      return EmptyCString();

    nsCAutoString convertedFilename;
    nsCAutoString filenameToDecode(Substring(filename, pos, endPos - pos));

    if(encoding.EqualsLiteral("b")) {
      char *str = SB_Base64Decode(filenameToDecode.get(), filenameToDecode.Length(), nsnull);

      nsDependentCString strToConvert(str);
      rv = convServ->ConvertStringToUTF8(strToConvert, charset.get(), PR_TRUE, convertedFilename);

      PR_Free(str);
    }
    else if(encoding.EqualsLiteral("q")) {
      NS_WARNING("XXX: No support for Q encoded strings!");
    }

    if(NS_SUCCEEDED(rv)) {
      filename = convertedFilename;
    }
  }

  nsCString_ReplaceChars(filename,
                         nsDependentCString(FILE_ILLEGAL_CHARACTERS),
                         '_');

  return filename;
}

static nsresult UnescapeFragment(const nsACString& aFragment, nsIURI* aURI,
                                 nsAString& aResult)
{
  // First, we need a charset
  nsCAutoString originCharset;
  nsresult rv = aURI->GetOriginCharset(originCharset);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now, we need the unescaper
  nsCOMPtr<nsITextToSubURI> textToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return textToSubURI->UnEscapeURIForUI(originCharset, aFragment, aResult);
}

static nsresult UnescapeFragment(const nsACString& aFragment, nsIURI* aURI,
                                 nsACString& aResult)
{
  nsAutoString result;
  nsresult rv = UnescapeFragment(aFragment, aURI, result);
  if (NS_SUCCEEDED(rv)) {
    aResult = NS_ConvertUTF16toUTF8(result);
  }
  return rv;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceFirmwareDownloader,
                              sbIFileDownloaderListener)

sbDeviceFirmwareDownloader::sbDeviceFirmwareDownloader()
: mIsBusy(PR_FALSE)
{
}

sbDeviceFirmwareDownloader::~sbDeviceFirmwareDownloader()
{
}

nsresult
sbDeviceFirmwareDownloader::Init(sbIDevice *aDevice,
                                 sbIDeviceEventListener *aListener,
                                 sbIDeviceFirmwareHandler *aHandler)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aHandler);

  NS_ENSURE_FALSE(mDevice, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_FALSE(mHandler, NS_ERROR_ALREADY_INITIALIZED);

  mDevice   = aDevice;
  mListener = aListener;
  mHandler  = aHandler;

  nsresult rv = NS_ERROR_UNEXPECTED;

  mDownloader = do_CreateInstance(SB_FILEDOWNLOADER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDownloader->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateCacheRoot(getter_AddRefs(mCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateCacheDirForDevice(mDevice,
                               mCacheDir,
                               getter_AddRefs(mDeviceCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbDeviceFirmwareDownloader::Init(sbIDevice *aDevice,
                                 const nsAString &aCacheDirName,
                                 sbIDeviceEventListener *aListener,
                                 sbIDeviceFirmwareHandler *aHandler)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aHandler);

  NS_ENSURE_FALSE(mDevice, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_FALSE(mHandler, NS_ERROR_ALREADY_INITIALIZED);

  mDevice   = aDevice;
  mListener = aListener;
  mHandler  = aHandler;

  nsresult rv = NS_ERROR_UNEXPECTED;

  mDownloader = do_CreateInstance(SB_FILEDOWNLOADER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDownloader->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateCacheRoot(getter_AddRefs(mCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateCacheDirForDevice(aCacheDirName,
                               mCacheDir,
                               getter_AddRefs(mDeviceCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*static*/ nsresult
sbDeviceFirmwareDownloader::CreateCacheRoot(nsIFile **aCacheRoot)
{
  // Initialize the cache
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIProperties> directoryService =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> localDataDir;
  rv = directoryService->Get(NS_APP_USER_PROFILE_LOCAL_50_DIR,
    NS_GET_IID(nsIFile),
    getter_AddRefs(localDataDir));
  NS_ENSURE_SUCCESS(rv, rv);

  if(!localDataDir) {
    rv = directoryService->Get(NS_APP_USER_PROFILE_50_DIR,
      NS_GET_IID(nsIFile),
      getter_AddRefs(localDataDir));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_TRUE(localDataDir, NS_ERROR_UNEXPECTED);

  nsCOMPtr<nsIFile> cacheDir;
  rv = localDataDir->Clone(getter_AddRefs(cacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(firmwareCacheName, "firmware_cache");
  rv = cacheDir->Append(firmwareCacheName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  PRBool isDirectory = PR_FALSE;

  rv = cacheDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!exists) {
    rv = cacheDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = cacheDir->IsDirectory(&isDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!isDirectory) {
    rv = cacheDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool isReadable = PR_FALSE;
  PRBool isWritable = PR_FALSE;

  rv = cacheDir->IsReadable(&isReadable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = cacheDir->IsWritable(&isWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(isReadable, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(isWritable, NS_ERROR_FAILURE);

  cacheDir.forget(aCacheRoot);

  return NS_OK;
}

/*static*/ nsresult
sbDeviceFirmwareDownloader::CreateCacheDirForDevice(sbIDevice *aDevice,
                                                    nsIFile *aCacheRoot,
                                                    nsIFile **aCacheDir)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aCacheRoot);
  NS_ENSURE_ARG_POINTER(aCacheDir);

  nsCOMPtr<sbIDeviceProperties> properties;
  nsresult rv = aDevice->GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString deviceDirStr;
  rv = properties->GetVendorName(deviceDirStr);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> modelNumber;
  rv = properties->GetModelNumber(getter_AddRefs(modelNumber));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString modelNumberStr;
  rv = modelNumber->GetAsAString(modelNumberStr);
  NS_ENSURE_SUCCESS(rv, rv);

  deviceDirStr.AppendLiteral(" ");
  deviceDirStr.Append(modelNumberStr);

  rv = CreateCacheDirForDevice(deviceDirStr, aCacheRoot, aCacheDir);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*static*/ nsresult 
sbDeviceFirmwareDownloader::CreateCacheDirForDevice(
                                          const nsAString &aCacheDirName, 
                                          nsIFile *aCacheRoot, 
                                          nsIFile **aCacheDir)
{
  NS_ENSURE_ARG_POINTER(aCacheRoot);
  NS_ENSURE_ARG_POINTER(aCacheDir);

  nsCOMPtr<nsIFile> deviceCacheDir;
  nsresult rv = aCacheRoot->Clone(getter_AddRefs(deviceCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceCacheDir->Append(aCacheDirName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  PRBool isDirectory = PR_FALSE;

  rv = deviceCacheDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!exists) {
    rv = deviceCacheDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = deviceCacheDir->IsDirectory(&isDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!isDirectory) {
    rv = deviceCacheDir->Create(nsIFile::DIRECTORY_TYPE, 0755);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRBool isReadable = PR_FALSE;
  PRBool isWritable = PR_FALSE;

  rv = deviceCacheDir->IsReadable(&isReadable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceCacheDir->IsWritable(&isWritable);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(isReadable, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(isWritable, NS_ERROR_FAILURE);

  deviceCacheDir.forget(aCacheDir);

  return NS_OK;
}

/*static*/ nsresult
sbDeviceFirmwareDownloader::CacheFirmwareUpdate(sbIDevice *aDevice,
                                                sbIDeviceFirmwareUpdate *aFirmwareUpdate,
                                                sbIDeviceFirmwareUpdate **aCachedFirmwareUpdate)
{
  nsString cacheDirName;
  cacheDirName.SetIsVoid(PR_TRUE);

  nsresult rv = CacheFirmwareUpdate(aDevice, 
                                    cacheDirName, 
                                    aFirmwareUpdate, 
                                    aCachedFirmwareUpdate);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*static*/ nsresult 
sbDeviceFirmwareDownloader::CacheFirmwareUpdate(
                                      sbIDevice *aDevice, 
                                      const nsAString &aCacheDirName,
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate,
                                      sbIDeviceFirmwareUpdate **aCachedFirmwareUpdate)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aFirmwareUpdate);

  nsCOMPtr<nsIFile> cacheRoot;
  nsresult rv = CreateCacheRoot(getter_AddRefs(cacheRoot));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> deviceCache;
  if(aCacheDirName.IsVoid() || aCacheDirName.IsEmpty()) {
    rv = CreateCacheDirForDevice(aDevice, 
                                 cacheRoot, 
                                 getter_AddRefs(deviceCache));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = CreateCacheDirForDevice(aCacheDirName, 
                                 cacheRoot, 
                                 getter_AddRefs(deviceCache));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIFile> firmwareFile;
  rv = aFirmwareUpdate->GetFirmwareImageFile(getter_AddRefs(firmwareFile));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString firmwareReadableVersion;
  rv = aFirmwareUpdate->GetFirmwareReadableVersion(firmwareReadableVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 firmwareVersion = 0;
  rv = aFirmwareUpdate->GetFirmwareVersion(&firmwareVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString leafName;
  rv = firmwareFile->GetLeafName(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> finalDestFile;
  rv = deviceCache->Clone(getter_AddRefs(finalDestFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = finalDestFile->Append(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = finalDestFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(exists) {
    rv = finalDestFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = firmwareFile->CopyTo(deviceCache, leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString firmwarePath;
  rv = deviceCache->GetPath(firmwarePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> newFirmwareFile;
  rv = NS_NewLocalFile(firmwarePath, PR_FALSE, getter_AddRefs(newFirmwareFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = newFirmwareFile->Append(leafName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareVersionVariant =
    sbNewVariant(firmwareVersion).get();
  rv = aDevice->SetPreference(NS_LITERAL_STRING(FIRMWARE_VERSION_PREF),
                              firmwareVersionVariant);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareReadableVariant =
    sbNewVariant(firmwareReadableVersion).get();
  rv = aDevice->SetPreference(NS_LITERAL_STRING(FIRMWARE_READABLE_PREF),
                              firmwareReadableVariant);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString newFirmwareFilePath;
  rv = newFirmwareFile->GetPath(newFirmwareFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareFileVariant =
    sbNewVariant(newFirmwareFilePath).get();
  rv = aDevice->SetPreference(NS_LITERAL_STRING(FIRMWARE_FILE_PREF),
                              firmwareFileVariant);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceFirmwareUpdate> firmwareUpdate =
    do_CreateInstance(SB_DEVICEFIRMWAREUPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = firmwareUpdate->Init(newFirmwareFile,
                            firmwareReadableVersion,
                            firmwareVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  firmwareUpdate.forget(aCachedFirmwareUpdate);

  return NS_OK;
}

PRBool
sbDeviceFirmwareDownloader::IsAlreadyInCache()
{
  NS_ENSURE_STATE(mDevice);
  NS_ENSURE_STATE(mDeviceCacheDir);
  NS_ENSURE_STATE(mHandler);

  nsCOMPtr<nsIVariant> firmwareVersion;
  nsresult rv =
    mDevice->GetPreference(NS_LITERAL_STRING(FIRMWARE_VERSION_PREF),
                           getter_AddRefs(firmwareVersion));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRInt32 unsignedVersion = 0;
  rv = firmwareVersion->GetAsInt32(&unsignedVersion);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  PRUint32 prefVersion = static_cast<PRUint32>(unsignedVersion);

  PRUint32 handlerVersion = 0;
  rv = mHandler->GetLatestFirmwareVersion(&handlerVersion);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  if(prefVersion < handlerVersion) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIVariant> firmwareFilePath;
  rv = mDevice->GetPreference(NS_LITERAL_STRING(FIRMWARE_FILE_PREF),
                               getter_AddRefs(firmwareFilePath));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString filePath;
  rv = firmwareFilePath->GetAsAString(filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> localFile;
  rv = NS_NewLocalFile(filePath, PR_FALSE, getter_AddRefs(localFile));

  PRBool exists = PR_FALSE;
  rv = localFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!exists) {
    return PR_FALSE;
  }

  return PR_TRUE;
}

nsresult
sbDeviceFirmwareDownloader::GetCachedFile(nsIFile **aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);

  nsCOMPtr<nsIVariant> firmwareFilePath;
  nsresult rv = mDevice->GetPreference(NS_LITERAL_STRING(FIRMWARE_FILE_PREF),
                                       getter_AddRefs(firmwareFilePath));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString filePath;
  rv = firmwareFilePath->GetAsAString(filePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> localFile;
  rv = NS_NewLocalFile(filePath, PR_FALSE, getter_AddRefs(localFile));

  PRBool exists = PR_FALSE;
  rv = localFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!exists) {
    return NS_ERROR_FAILURE;
  }

  NS_ADDREF(*aFile = localFile);

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::Start()
{
  NS_ENSURE_STATE(mDownloader);
  NS_ENSURE_STATE(mDevice);
  NS_ENSURE_STATE(mHandler);
  NS_ENSURE_STATE(mDeviceCacheDir);
  NS_ENSURE_FALSE(mIsBusy, NS_ERROR_FAILURE);

  mIsBusy = PR_TRUE;

  nsresult rv = NS_ERROR_UNEXPECTED;
  PRBool inCache = IsAlreadyInCache();

  if(!inCache) {
    // Not in cache, download.
    nsCOMPtr<nsIURI> firmwareURI;
    rv =
      mHandler->GetLatestFirmwareLocation(getter_AddRefs(firmwareURI));
    NS_ENSURE_TRUE(firmwareURI, NS_ERROR_UNEXPECTED);

    rv = mDownloader->SetSourceURI(firmwareURI);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDownloader->Start();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = SendDeviceEvent(sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_START,
                       nsnull,
                       PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  if(inCache) {
    nsCOMPtr<nsIFile> file;
    rv = GetCachedFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDeviceFirmwareUpdate> firmwareUpdate =
      do_CreateInstance(SB_DEVICEFIRMWAREUPDATE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 firmwareVersion = 0;
    nsString firmwareReadableVersion;

    rv = mHandler->GetLatestFirmwareVersion(&firmwareVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mHandler->GetLatestFirmwareReadableVersion(firmwareReadableVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = firmwareUpdate->Init(file,
                              firmwareReadableVersion,
                              firmwareVersion);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIVariant> progress = sbNewVariant((PRUint32) 100).get();
    rv = SendDeviceEvent(sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_PROGRESS,
                         progress,
                         PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIVariant> firmwareUpdateVariant =
      sbNewVariant(firmwareUpdate).get();
    rv = SendDeviceEvent(sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_END,
                         firmwareUpdateVariant,
                         PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    mIsBusy = PR_FALSE;
  }

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::Cancel()
{
  NS_ENSURE_STATE(mDownloader);

  nsresult rv = NS_ERROR_UNEXPECTED;

  if(mIsBusy) {
    rv = mDownloader->Cancel();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Couldn't cancel download");

    mIsBusy = PR_FALSE;
  }

  nsCOMPtr<sbIFileDownloaderListener> grip(this);
  rv = mDownloader->SetListener(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::HandleProgress()
{
  NS_ENSURE_STATE(mDownloader);
  NS_ENSURE_STATE(mDevice);

  PRUint32 percentComplete = 0;
  nsresult rv = mDownloader->GetPercentComplete(&percentComplete);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> data =
    sbNewVariant(percentComplete).get();

  rv = SendDeviceEvent(sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_PROGRESS,
                       data,
                       PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::HandleComplete()
{
  NS_ENSURE_STATE(mDownloader);
  NS_ENSURE_STATE(mDevice);

  PRBool success = PR_FALSE;
  nsresult rv = mDownloader->GetSucceeded(&success);
  NS_ENSURE_SUCCESS(rv, rv);

  // Oops, looks like we didn't succeed downloading the firmware update.
  // Signal the error and exit early (no need to actually return an error,
  // the event indicates the operation was aborted).
  if(!success) {
    rv = SendDeviceEvent(sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_ERROR,
                         nsnull,
                         PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIFileDownloaderListener> grip(this);
    rv = mDownloader->SetListener(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    mIsBusy = PR_FALSE;

    return NS_OK;
  }

  nsCOMPtr<nsIRequest> request;
  rv = mDownloader->GetRequest(getter_AddRefs(request));
  NS_ENSURE_TRUE(request, NS_ERROR_UNEXPECTED);

  nsCString contentDisposition;
  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(request));
  if(httpChannel) {
    httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("content-disposition"),
                                   contentDisposition);
  }

  // Still Empty? Try with multipartChannel.
  if(contentDisposition.IsEmpty()) {
    nsCOMPtr<nsIMultiPartChannel> multipartChannel(do_QueryInterface(request));
    if(multipartChannel) {
      multipartChannel->GetContentDisposition(contentDisposition);
    }
  }

  nsCString filename;
  if(!contentDisposition.IsEmpty()) {
     filename = GetContentDispositionFilename(contentDisposition);
  }
  else {
    nsCOMPtr<nsIURI> firmwareURI;
    nsresult rv =
      mHandler->GetLatestFirmwareLocation(getter_AddRefs(firmwareURI));
    NS_ENSURE_TRUE(firmwareURI, NS_ERROR_UNEXPECTED);

    nsCOMPtr<nsIURL> url(do_QueryInterface(firmwareURI));
    if(url) {
      nsCString extension;
      url->GetFileExtension(extension);

      rv = UnescapeFragment(extension, url, extension);
      extension.Trim(".", PR_FALSE);

      nsCAutoString leafName;
      url->GetFileBaseName(leafName);

      if(!leafName.IsEmpty()) {
        rv = UnescapeFragment(leafName, url, filename);
        if(NS_FAILED(rv)) {
          filename = leafName;
        }
      }

      if(!extension.IsEmpty()) {
        filename.AppendLiteral(".");
        filename.Append(extension);
      }
    }
  }

  // Move the file to it's resting place.
  nsCOMPtr<nsIFile> file;
  rv = mDownloader->GetDestinationFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF8toUTF16 filename16(filename);
  rv = file->MoveTo(mDeviceCacheDir, filename16);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString firmwarePath;
  rv = mDeviceCacheDir->GetPath(firmwarePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> firmwareFile;
  rv = NS_NewLocalFile(firmwarePath, PR_FALSE, getter_AddRefs(firmwareFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = firmwareFile->Append(filename16);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDeviceFirmwareUpdate> firmwareUpdate =
    do_CreateInstance(SB_DEVICEFIRMWAREUPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 firmwareVersion = 0;
  nsString firmwareReadableVersion;

  rv = mHandler->GetLatestFirmwareVersion(&firmwareVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mHandler->GetLatestFirmwareReadableVersion(firmwareReadableVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = firmwareUpdate->Init(firmwareFile,
                            firmwareReadableVersion,
                            firmwareVersion);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareVersionVariant =
    sbNewVariant(firmwareVersion).get();
  rv = mDevice->SetPreference(NS_LITERAL_STRING(FIRMWARE_VERSION_PREF),
                              firmwareVersionVariant);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareReadableVariant =
    sbNewVariant(firmwareReadableVersion).get();
  rv = mDevice->SetPreference(NS_LITERAL_STRING(FIRMWARE_READABLE_PREF),
                              firmwareReadableVariant);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString firmwareFilePath;
  rv = firmwareFile->GetPath(firmwareFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> firmwareFileVariant =
    sbNewVariant(firmwareFilePath).get();
  rv = mDevice->SetPreference(NS_LITERAL_STRING(FIRMWARE_FILE_PREF),
                              firmwareFileVariant);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIFileDownloaderListener> grip(this);
  rv = mDownloader->SetListener(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  mIsBusy = PR_FALSE;

  nsCOMPtr<nsIVariant> data = sbNewVariant(firmwareUpdate).get();

  rv = SendDeviceEvent(sbIDeviceEvent::EVENT_FIRMWARE_DOWNLOAD_END,
                       data,
                       PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::CreateDeviceEvent(PRUint32 aType,
                                              nsIVariant *aData,
                                              sbIDeviceEvent **aEvent)
{
  NS_ENSURE_STATE(mDevice);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIDeviceManager2> deviceManager =
    do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deviceManager->CreateEvent(aType,
                                  aData,
                                  mDevice,
                                  sbIDevice::STATE_IDLE,
                                  sbIDevice::STATE_IDLE,
                                  aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::SendDeviceEvent(sbIDeviceEvent *aEvent,
                                            PRBool aAsync /*= PR_TRUE*/)
{
  NS_ENSURE_STATE(mDevice);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIDeviceEventListener> listener = mListener;

  NS_ENSURE_STATE(mDevice);
  nsCOMPtr<sbIDeviceEventTarget> target = do_QueryInterface(mDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool dispatched = PR_FALSE;
  rv = target->DispatchEvent(aEvent, aAsync, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_WARN_IF_FALSE(dispatched, "Event not dispatched");

  if(listener) {
    rv = listener->OnDeviceEvent(aEvent);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Error while calling listener.");
  }

  return NS_OK;
}

nsresult
sbDeviceFirmwareDownloader::SendDeviceEvent(PRUint32 aType,
                                            nsIVariant *aData,
                                            PRBool aAsync /*= PR_TRUE*/)
{
  nsCOMPtr<sbIDeviceEvent> deviceEvent;
  nsresult rv = CreateDeviceEvent(aType, aData, getter_AddRefs(deviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SendDeviceEvent(deviceEvent, aAsync);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// ----------------------------------------------------------------------------
// sbIFileDownloaderListener
// ----------------------------------------------------------------------------

NS_IMETHODIMP
sbDeviceFirmwareDownloader::OnProgress()
{
  nsresult rv = HandleProgress();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceFirmwareDownloader::OnComplete()
{
  nsresult rv = HandleComplete();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
