/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#include "sbTranscodeError.h"

#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIURI.h>

#include <sbIMediaItem.h>
#include <sbITranscodeManager.h>

#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsTArray.h>

#include <sbStandardProperties.h>
#include <sbStringUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS5(sbTranscodeError,
                              sbITranscodeError,
                              nsIScriptError,
                              nsIConsoleMessage,
                              nsISupportsString,
                              nsISupportsPrimitive)

sbTranscodeError::sbTranscodeError()
  :mLock(nsnull)
{
  /* member initializers and constructor code */
}

sbTranscodeError::~sbTranscodeError()
{
  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
  /* destructor code */
}

/***** nsIConsoleMessage *****/

/* [binaryname (MessageMoz)] readonly attribute wstring message; */
NS_IMETHODIMP
sbTranscodeError::GetMessageMoz(PRUnichar * *aMessage)
{
  NS_ENSURE_ARG_POINTER(aMessage);
  nsString message;
  nsresult rv = GetData(message);
  NS_ENSURE_SUCCESS(rv, rv);
  *aMessage = ToNewUnicode(message);
  NS_ENSURE_TRUE(*aMessage, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/***** nsIScriptError *****/
/* readonly attribute AString errorMessage; */
NS_IMETHODIMP
sbTranscodeError::GetErrorMessage(nsAString & aErrorMessage)
{
  nsCString message;
  nsresult rv = GetData(aErrorMessage);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

/* readonly attribute AString sourceName; */
NS_IMETHODIMP
sbTranscodeError::GetSourceName(nsAString & aSourceName)
{
  aSourceName.Assign(mSrcUri);
  return NS_OK;
}

/* readonly attribute AString sourceLine; */
NS_IMETHODIMP
sbTranscodeError::GetSourceLine(nsAString & aSourceLine)
{
  return GetDetail(aSourceLine);
}

/* readonly attribute PRUint32 lineNumber; */
NS_IMETHODIMP
sbTranscodeError::GetLineNumber(PRUint32 *aLineNumber)
{
  NS_ENSURE_ARG_POINTER(aLineNumber);
  *aLineNumber = 0;
  return NS_OK;
}

/* readonly attribute PRUint32 columnNumber; */
NS_IMETHODIMP
sbTranscodeError::GetColumnNumber(PRUint32 *aColumnNumber)
{
  NS_ENSURE_ARG_POINTER(aColumnNumber);
  *aColumnNumber = 0;
  return NS_OK;
}

/* readonly attribute PRUint32 flags; */
NS_IMETHODIMP
sbTranscodeError::GetFlags(PRUint32 *aFlags)
{
  NS_ENSURE_ARG_POINTER(aFlags);
  *aFlags = nsIScriptError::errorFlag;
  return NS_OK;
}

/* readonly attribute string category; */
NS_IMETHODIMP
sbTranscodeError::GetCategory(char * *aCategory)
{
  NS_ENSURE_ARG_POINTER(aCategory);
  *aCategory = ToNewCString(NS_LITERAL_CSTRING("nightingale transcode"));
  return NS_OK;
}

/* void init (in wstring message,
 *            in wstring sourceName,
 *            in wstring sourceLine,
 *            in PRUint32 lineNumber,
 *            in PRUint32 columnNumber,
 *            in PRUint32 flags,
 *            in string category); */
NS_IMETHODIMP
sbTranscodeError::Init(const PRUnichar *message,
                       const PRUnichar *sourceName,
                       const PRUnichar *sourceLine,
                       PRUint32 lineNumber,
                       PRUint32 columnNumber,
                       PRUint32 flags,
                       const char *category)
{
  /* This doesn't make sense for this object */
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* AUTF8String toString (); */
NS_IMETHODIMP
sbTranscodeError::ToString(nsACString & _retval)
{
  nsString result;
  nsresult rv = GetData(result);
  NS_ENSURE_SUCCESS(rv, rv);
  CopyUTF16toUTF8(result, _retval);
  return NS_OK;
}

/***** nsISupportsString *****/
/* attribute AString data; */
NS_IMETHODIMP
sbTranscodeError::GetData(nsAString & aData)
{
  nsresult rv;
  nsString spec;
  nsCOMPtr<nsIURI> uri;
  nsCOMPtr<sbIMediaItem> item;
  {
    /* scope, avoid locking for too long */
    nsAutoLock lock(mLock);
    spec = mSrcUri;
    item = mSrcItem;
    if (!item) item = mDestItem;
  }

  nsString title, key(mMessageWithoutItem);
  if (item) {
    rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                            title);
    NS_ENSURE_SUCCESS(rv, rv);
    CompressWhitespace(title);
    if (!title.IsEmpty()) {
      key = mMessageWithItem;
    }
  }

  if (title.IsEmpty() && !spec.IsEmpty()) {
    rv = NS_NewURI(getter_AddRefs(uri), spec);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri, &rv);
    if (NS_SUCCEEDED(rv) && fileUrl) {
      nsCOMPtr<nsIFile> file;
      rv = fileUrl->GetFile(getter_AddRefs(file));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = file->GetPath(title);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      title = spec;
    }
  }

  if (title.IsEmpty()) {
    title = SBLocalizedString("transcode.error.item.unknown");
  }

  aData.Assign(SBLocalizedString(key));
  NS_NAMED_LITERAL_STRING(REPLACE_KEY, "%(item)");
  PRInt32 index = aData.Find(REPLACE_KEY);
  if (index > -1) {
    aData.Replace(index, REPLACE_KEY.Length(), title);
  }
  return NS_OK;
}
NS_IMETHODIMP
sbTranscodeError::SetData(const nsAString & aData)
{
  /* setting the string doesn't make any sense */
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* wstring toString (); */
NS_IMETHODIMP
sbTranscodeError::ToString(PRUnichar **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsString message;
  nsresult rv = GetData(message);
  NS_ENSURE_SUCCESS(rv, rv);
  *_retval = ToNewUnicode(message);
  NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

/***** nsISupportsPrimitive *****/

/* readonly attribute unsigned short type; */
NS_IMETHODIMP
sbTranscodeError::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);
  *aType = nsISupportsPrimitive::TYPE_STRING;
  return NS_OK;
}

/***** sbITranscodeError *****/

/* readonly attribute AString messageWithItem; */
NS_IMETHODIMP
sbTranscodeError::GetMessageWithItem(nsAString & aMessageWithItem)
{
  nsAutoLock lock(mLock);
  aMessageWithItem.Assign(mMessageWithItem);
  return NS_OK;
}

/* readonly attribute AString messageWithoutItem; */
NS_IMETHODIMP
sbTranscodeError::GetMessageWithoutItem(nsAString & aMessageWithoutItem)
{
  nsAutoLock lock(mLock);
  aMessageWithoutItem.Assign(mMessageWithoutItem);
  return NS_OK;
}

/* readonly attribute AString details; */
NS_IMETHODIMP
sbTranscodeError::GetDetail(nsAString & aDetails)
{
  nsAutoLock lock(mLock);
  aDetails.Assign(mDetails);
  return NS_OK;
}

/* attribute AString sourceUri; */
NS_IMETHODIMP
sbTranscodeError::GetSourceUri(nsAString & aSourceUri)
{
  nsAutoLock lock(mLock);
  aSourceUri.Assign(mSrcUri);
  return NS_OK;
}
NS_IMETHODIMP
sbTranscodeError::SetSourceUri(const nsAString & aSourceUri)
{
  nsAutoLock lock(mLock);
  mSrcUri.Assign(aSourceUri);
  return NS_OK;
}

/* attribute sbIMediaItem sourceItem; */
NS_IMETHODIMP
sbTranscodeError::GetSourceItem(sbIMediaItem * *aSourceItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*aSourceItem = mSrcItem);
  return NS_OK;
}
NS_IMETHODIMP
sbTranscodeError::SetSourceItem(sbIMediaItem * aSourceItem)
{
  nsAutoLock lock(mLock);
  mSrcItem = aSourceItem;
  return NS_OK;
}

/* attribute AString destUri; */
NS_IMETHODIMP
sbTranscodeError::GetDestUri(nsAString & aDestUri)
{
  nsAutoLock lock(mLock);
  aDestUri.Assign(mDestUri);
  return NS_OK;
}
NS_IMETHODIMP
sbTranscodeError::SetDestUri(const nsAString & aDestUri)
{
  nsAutoLock lock(mLock);
  mDestUri.Assign(aDestUri);
  return NS_OK;
}

/* attribute sbIMediaItem destItem; */
NS_IMETHODIMP
sbTranscodeError::GetDestItem(sbIMediaItem * *aDestItem)
{
  NS_ENSURE_ARG_POINTER(aDestItem);
  nsAutoLock lock(mLock);
  NS_IF_ADDREF(*aDestItem = mDestItem);
  return NS_OK;
}
NS_IMETHODIMP
sbTranscodeError::SetDestItem(sbIMediaItem * aDestItem)
{
  nsAutoLock lock(mLock);
  mDestItem = aDestItem;
  return NS_OK;
}

/* void init (in AString aMessageWithItem,
 *            in AString aMessageWithoutItem,
 *            in AString aDetails,
 *            in PRUint32 aSeverity); */
NS_IMETHODIMP
sbTranscodeError::Init(const nsAString & aMessageWithItem,
                       const nsAString & aMessageWithoutItem,
                       const nsAString & aDetails)
{
  NS_ENSURE_FALSE(mLock, NS_ERROR_ALREADY_INITIALIZED);
  mLock = nsAutoLock::NewLock("sbTranscodeError::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);
  nsAutoLock lock(mLock);
  mMessageWithItem.Assign(aMessageWithItem);
  mMessageWithoutItem.Assign(aMessageWithoutItem);
  mDetails.Assign(aDetails);
  return NS_OK;
}
