#ifndef __SB_QUICKTIMEUTILS_H__
#define __SB_QUICKTIMEUTILS_H__

#include <Movies.h>

#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsILocalFile.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsMemory.h>
#include <nsNetUtil.h>
#include <nsRect.h>
#include <nsStringGlue.h>

#ifdef XP_MACOSX
#include <nsILocalFileMac.h>
#endif

#define NSRESULT_FROM_OSERR(err) ((err) == noErr) ? NS_OK : NS_ERROR_FAILURE

#define QT_ENSURE_NOERR(err)                                                   \
  NS_ENSURE_TRUE((err) == noErr, NS_ERROR_FAILURE);

#define QT_ENSURE_NOMOVIESERROR()                                              \
  QT_ENSURE_NOERR(::GetMoviesError());

#define countof(_array) sizeof(_array) / sizeof(_array[0])

#define SB_METADATA_KEY_TITLE       "title"
#define SB_METADATA_KEY_ARTIST      "artist"
#define SB_METADATA_KEY_ALBUM       "album"
#define SB_METADATA_KEY_GENRE       "genre"
#define SB_METADATA_KEY_COMMENT     "comment"
#define SB_METADATA_KEY_COMPOSER    "composer"
#define SB_METADATA_KEY_LENGTH      "length"
#define SB_METADATA_KEY_SRCURL      "url"
#define SB_METADATA_KEY_TRACKNO     "track_no"
#define SB_METADATA_KEY_TRACKTOTAL  "track_total"
#define SB_METADATA_KEY_DISCNO      "disc_no"
#define SB_METADATA_KEY_DISCTOTAL   "disc_total"
#define SB_METADATA_KEY_YEAR        "year"
#define SB_METADATA_KEY_COMPILATION "compilation"
#define SB_METADATA_KEY_BPM         "bpm"
#define SB_METADATA_KEY_VENDOR      "vendor"
#define SB_METADATA_KEY_LABEL       "label"

#define QT_METADATA_KEY_TITLE    kQTMetaDataCommonKeyDisplayName
#define QT_METADATA_KEY_ARTIST   kQTMetaDataCommonKeyArtist
#define QT_METADATA_KEY_ALBUM    kQTMetaDataCommonKeyAlbum
#define QT_METADATA_KEY_GENRE    kQTMetaDataCommonKeyGenre
#define QT_METADATA_KEY_COMMENT  kQTMetaDataCommonKeyComment
#define QT_METADATA_KEY_COMPOSER kQTMetaDataCommonKeyComposer
#define QT_METADATA_KEY_TRACKNO     'trkn'
#define QT_METADATA_KEY_TRACKTOTAL  'trkn'
#define QT_METADATA_KEY_DISCNO      'disk'
#define QT_METADATA_KEY_DISCTOTAL   'disk'
#define QT_METADATA_KEY_YEAR        '\xa9day'
#define QT_METADATA_KEY_COMPILATION 'cpil'
#define QT_METADATA_KEY_BPM         'tmpo'
#define QT_METADATA_KEY_VENDOR      '\xa9too'
#define QT_METADATA_KEY_LABEL       'labl'

/**
* Macros for mapping metadata value keys
 */
#define METADATA_MAP_HEAD(_keyString)                                          \
  nsAutoString _internalKeyString(_keyString);                                 \
  if (PR_FALSE) { /* nothing */ }

#define METADATA_MAP_ENTRY(_fromEntry, _toEntry, _toFormat)                    \
  else if (_internalKeyString.EqualsLiteral(_fromEntry)) {                     \
    *aQTKey = _toEntry;                                                        \
    *aQTKeyFormat  = _toFormat;                                                \
    return NS_OK;                                                              \
}

#define METADATA_MAP_TAIL                                                      \
  else { return NS_ERROR_NOT_AVAILABLE; }

static nsresult
SB_ConvertSBMetadataKeyToQTMetadataKey(const nsAString& aSBKey,
                                       OSType* aQTKey,
                                       QTMetaDataKeyFormat* aQTKeyFormat)
{
  NS_ENSURE_ARG_POINTER(aQTKey);
  NS_ENSURE_ARG_POINTER(aQTKeyFormat);
  
  METADATA_MAP_HEAD(aSBKey)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_TITLE,      QT_METADATA_KEY_TITLE,      kQTMetaDataKeyFormatCommon)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_ARTIST,     QT_METADATA_KEY_ARTIST,     kQTMetaDataKeyFormatCommon)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_ALBUM,      QT_METADATA_KEY_ALBUM,      kQTMetaDataKeyFormatCommon)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_GENRE,      QT_METADATA_KEY_GENRE,      kQTMetaDataKeyFormatCommon)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_COMMENT,    QT_METADATA_KEY_COMMENT,    kQTMetaDataKeyFormatCommon)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_COMPOSER,   QT_METADATA_KEY_COMPOSER,   kQTMetaDataKeyFormatCommon)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_TRACKNO,    QT_METADATA_KEY_TRACKNO,    kQTMetaDataKeyFormatiTunesShortForm)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_TRACKTOTAL, QT_METADATA_KEY_TRACKTOTAL, kQTMetaDataKeyFormatiTunesShortForm)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_DISCNO,     QT_METADATA_KEY_DISCNO,     kQTMetaDataKeyFormatiTunesShortForm)
    METADATA_MAP_ENTRY(SB_METADATA_KEY_DISCTOTAL,  QT_METADATA_KEY_DISCTOTAL,  kQTMetaDataKeyFormatiTunesShortForm)
  METADATA_MAP_TAIL
}

/**
 * This is a helper class to auto-free any core foundation types
 */
template<class Type>
class sbCFRef
{
protected:
  Type mRef;

public:
  sbCFRef() : mRef(nsnull) { }
  sbCFRef(Type aRef) : mRef(aRef) { }
  
  ~sbCFRef() {
    if (mRef) ::CFRelease(mRef);
  }

  Type operator=(Type aRef) {
    if (mRef && (mRef != aRef))
      ::CFRelease(mRef);
    return (mRef = aRef);
  }
  
  operator Type() {
    return mRef;
  }
  
  operator int() {
    return (mRef != nsnull);
  }
  
  void reset() {
    mRef = nsnull;
  }
  
  Type* addressof() {
    return &mRef;
  }
};

/**
 * This is a helper class to auto-swap QuickDraw Graphics Ports
 */
class sbQDPortSwapper
{
protected:
  CGrafPtr mOldPort;
  PRBool mPortSwapped;
public:
  sbQDPortSwapper(CGrafPtr aNewPort) : mOldPort(nsnull),
                                       mPortSwapped(PR_FALSE) {
    if (aNewPort) {
      mPortSwapped = ::QDSwapPort(aNewPort, &mOldPort);
    }
  }
  
  ~sbQDPortSwapper() {
    if (mPortSwapped)
      ::QDSwapPort(mOldPort, nsnull);
  }
  
};

class sbQTMetaDataRefHolder
{
protected:
  QTMetaDataRef mRef;
  
public:
  sbQTMetaDataRefHolder() : mRef(nsnull) { }
  sbQTMetaDataRefHolder(QTMetaDataRef aRef) : mRef(aRef) { }
  
  ~sbQTMetaDataRefHolder() {
    if (mRef) QTMetaDataRelease(mRef);
  }
  
  operator QTMetaDataRef() { return mRef; }
  operator int() { return (mRef != nsnull); }
  QTMetaDataRef* addressof() { return &mRef; }
  
  QTMetaDataRef operator=(QTMetaDataRef aRef) {
    if (mRef && (mRef != aRef))
      QTMetaDataRelease(mRef);
    return (mRef = aRef);
  }
};

static nsresult
SB_CreateMovieFromURI(nsIURI* aURI,
                      Movie* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  if (!aURI) {
    // Passing in a null URI means that the caller wants an empty movie.

    // XXXben Used to use NewMovieFromProperties here with null parameters.
    //        That worked fine in QuickTime 7 for Windows but crashed hard in
    //        QuickTime 6.5. The docs say it should work, but NewMovie works
    //        just as well. Go figure.
    *_retval = ::NewMovie(0);
    QT_ENSURE_NOMOVIESERROR();

    return NS_OK;
  }

  OSStatus status;
  nsresult rv;
  
  nsCAutoString spec;
  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  QTNewMoviePropertyElement movieProperties[10] = {0};
  PRInt32 propCount = 0;

  sbCFRef<CFURLRef> urlRef;
  sbCFRef<CFStringRef> urlString;

#ifdef XP_MACOSX
  
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILocalFileMac> macFile = do_QueryInterface(file, &rv);
      if (NS_SUCCEEDED(rv)) {
        rv = macFile->GetCFURL(urlRef.addressof());
        if (NS_FAILED(rv))
          urlRef.reset();
      }
    }
  }

#endif

  if (!(int)urlRef) {
    // Windows will go down this path, as will Mac if it failed above
    urlString = CFStringCreateWithCString(kCFAllocatorDefault, spec.get(),
                                          kCFStringEncodingUTF8);
    urlRef = CFURLCreateWithString(kCFAllocatorDefault, urlString, nsnull);
  }
  
  NS_ENSURE_STATE((int)urlRef);
  
  movieProperties[propCount].propClass = kQTPropertyClass_DataLocation;
  movieProperties[propCount].propID = kQTDataLocationPropertyID_CFURL;
  movieProperties[propCount].propValueSize = sizeof(CFURLRef);
  movieProperties[propCount++].propValueAddress = urlRef.addressof();

  Boolean trueValue = PR_TRUE;
  movieProperties[propCount].propClass = kQTPropertyClass_NewMovieProperty;
  movieProperties[propCount].propID = kQTNewMoviePropertyID_Active;
  movieProperties[propCount].propValueSize = sizeof(trueValue);
  movieProperties[propCount++].propValueAddress = &trueValue;
  
  // Ensure that we have some sort of GWorld already. This is a QuickTime
  // requirement.
  CGrafPtr oldPort = nsnull;
  GetPort((GrafPtr*)&oldPort);
  if (!oldPort)
    MacSetPort(nsnull);

  status = ::NewMovieFromProperties(propCount, movieProperties, 0, nsnull,
                                    _retval);
  QT_ENSURE_NOERR(status);

  return NS_OK;
}

inline nsresult
SB_CreateMovieFromFile(nsIFile* aFile,
                       Movie* _retval)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewFileURI(getter_AddRefs(uri), aFile);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "NS_NewFileURI failed");
  return SB_CreateMovieFromURI(uri, _retval);

}

inline nsresult
SB_CreateMovieFromURL(nsIURL* aURL,
                      Movie* _retval)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri = do_QueryInterface(aURL, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "QI failed");
  return SB_CreateMovieFromURI(uri, _retval);
}

inline nsresult
SB_CreateMovieFromString(const nsAString& aURL,
                         Movie* _retval)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "NS_NewURI failed");
  return SB_CreateMovieFromURI(uri, _retval);
}

inline nsresult
SB_CreateMovieFromCString(nsACString& aURL,
                          Movie* _retval)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "NS_NewURI failed");
  return SB_CreateMovieFromURI(uri, _retval);
}

inline nsresult
SB_CreateEmptyMovie(Movie* _retval)
{
  return SB_CreateMovieFromURI(nsnull, _retval);
}

inline void
RectToMacRect(const nsRect& aRect,
              Rect& aMacRect)
{
  aMacRect.left = aRect.x;
  aMacRect.top = aRect.y;
  aMacRect.right = aRect.x + aRect.width;
  aMacRect.bottom = aRect.y + aRect.height;
}

inline void
MacRectToRect(const Rect& aMacRect,
              nsRect& aRect)
{
  aRect.x = aMacRect.left;
  aRect.y = aMacRect.top;
  aRect.width = aMacRect.right - aMacRect.left;
  aRect.height = aMacRect.bottom - aMacRect.top;
}

static nsresult
SB_GetFileExtensionFromURI(nsIURI* aURI,
                           nsACString& _retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  
  nsCAutoString strExtension;
  
  nsresult rv;
  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    // Try the easy way...
    rv = url->GetFileExtension(strExtension);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Try the hard way...
    nsCAutoString spec;
    rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRInt32 index = spec.RFindChar('.');
    if (index >= 0) {
      strExtension.Assign(StringTail(spec, spec.Length() -1 - index));
    }
  }
  
  if (strExtension.IsEmpty())
    return NS_ERROR_MALFORMED_URI;
  
  // Strip '.' from the beginning and end, if it exists
  strExtension.Trim(".");
  
  _retval.Assign(strExtension);
  return NS_OK;
}

static nsresult
SB_GetQuickTimeMetadataForKey(Movie aMovie,
                              const nsAString& aKey,
                              PRUint32* aType,
                              nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aMovie);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_TRUE(!aKey.IsEmpty(), NS_ERROR_INVALID_ARG);
  
  // Special case for length
  if (aKey.EqualsLiteral(SB_METADATA_KEY_LENGTH)) {
    TimeValue duration = ::GetMovieDuration(aMovie);
    QT_ENSURE_NOMOVIESERROR();
    
    TimeScale scale = ::GetMovieTimeScale(aMovie);
    QT_ENSURE_NOMOVIESERROR();
    
    // No divide by zero, please.
    NS_ENSURE_STATE(scale);
    
    //LOG(("GetLength: duration = %d, scale = %d", duration, scale));
    
    PRUint64 result;
    LL_DIV(result, duration, scale);
    LL_MUL(result, result, 1000);
    
    nsAutoString intString;
    intString.AppendInt((PRInt64)result);
    _retval.Assign(intString);
    return NS_OK;
  }
  
	sbQTMetaDataRefHolder metaDataRef;
	OSStatus status = QTCopyMovieMetaData(aMovie, metaDataRef.addressof());
	QT_ENSURE_NOERR(status);
  
  //OSType metaDataKey = kQTMetaDataCommonKeyArtist;
  OSType metaDataKey = nsnull;
  QTMetaDataKeyFormat metaDataKeyFormat = nsnull;
  nsresult rv = SB_ConvertSBMetadataKeyToQTMetadataKey(aKey, &metaDataKey,
                                                       &metaDataKeyFormat);
  if (NS_FAILED(rv))
    return rv;
  
  QTMetaDataItem item = kQTMetaDataItemUninitialized;
  status = QTMetaDataGetNextItem(metaDataRef,
                                 kQTMetaDataStorageFormatWildcard,
                                 kQTMetaDataItemUninitialized,
                                 metaDataKeyFormat,
                                 (const UInt8*)&metaDataKey,
                                 sizeof(metaDataKey), &item);
  if (status != noErr) {
    // This metadata isn't here...
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  // Get the size of the data
  ByteCount size = 0;
  status = QTMetaDataGetItemValue(metaDataRef, item, nsnull, 0, &size);
  QT_ENSURE_NOERR(status);
  
  // Get the type of data
  QTPropertyValueType dataType;
  status = QTMetaDataGetItemProperty(metaDataRef, item, 
                                     kPropertyClass_MetaDataItem, 
                                     kQTMetaDataItemPropertyID_DataType, 
                                     sizeof(dataType), &dataType, nsnull);
  QT_ENSURE_NOERR(status);
  
  // String data by default
  nsCAutoString strValue;
  *aType = 0;

  if (dataType == kQTMetaDataTypeBinary) {
    
    PRUint8* buffer = (PRUint8*)nsMemory::Alloc(size);
    NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);
    
    status = QTMetaDataGetItemValue(metaDataRef, item, (UInt8*)buffer, size, nsnull);
    QT_ENSURE_NOERR(status);
    
    // 'trkn'  and 'disk' format:
    //   00 00 xx xx yy yy 00 00
    // xx = track/disc number, big endian
    // yy = total tracks on disc/total discs in set, big endian
    
    if (aKey.EqualsLiteral(SB_METADATA_KEY_TRACKNO) ||
        aKey.EqualsLiteral(SB_METADATA_KEY_DISCNO)) {
      
      NS_ENSURE_TRUE(size >= 4, NS_ERROR_UNEXPECTED);
      
      // Skip the first two bytes
      PRUint8* newBuffer = buffer + 2;
      
      PRUint16 number;
      PRUint8* low = (PRUint8*)&number;
      PRUint8* high = low + 1;
      
      // Byteswap the track/disc number
      *low = *(newBuffer + 1);
      *high = *newBuffer;

      strValue.AppendInt(number);
    }
    else if (aKey.EqualsLiteral(SB_METADATA_KEY_TRACKTOTAL) ||
             aKey.EqualsLiteral(SB_METADATA_KEY_DISCTOTAL)) {
      NS_ENSURE_TRUE(size >= 6, NS_ERROR_UNEXPECTED);
               
      // Skip the first four bytes
      PRUint8* newBuffer = buffer + 4;
               
      PRUint16 number;
      PRUint8* low = (PRUint8*)&number;
      PRUint8* high = low + 1;
               
      // Byteswap the total track/disc number
      *low = *(newBuffer + 1);
      *high = *newBuffer;
               
      strValue.AppendInt(number);
    }
    
    nsMemory::Free(buffer);
    
    // This is numeric data
    *aType = 1;
  }
  else if (dataType == kQTMetaDataTypeUTF8) {

    // Copy the data into a mozilla string
    strValue.SetLength(size);
    
    nsCAutoString::char_type* buffer;
    strValue.BeginWriting(&buffer);
    
    NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);
    
    status = QTMetaDataGetItemValue(metaDataRef, item, (UInt8*)buffer, size, nsnull);
    QT_ENSURE_NOERR(status);
  }
  
  _retval.Assign(NS_ConvertUTF8toUTF16(strValue));
  return NS_OK;
}

#endif /* __SB_QUICKTIMEUTILS_H__ */
