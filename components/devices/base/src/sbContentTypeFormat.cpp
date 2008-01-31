/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

#include "sbContentTypeFormat.h"

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsISimpleEnumerator.h>
#include <nsISupportsPrimitives.h>

class FourCCEnumerator : public nsISimpleEnumerator {
public:
  NS_DECL_NSISIMPLEENUMERATOR;
  NS_DECL_ISUPPORTS;
protected:
  friend class sbContentTypeFormat;
  FourCCEnumerator(sbContentTypeFormat* aFormat,
                   nsTArray<FourCC>* aArray)
   : mFormat(aFormat),
     mArray(aArray),
     mIndex(0)
  {
  }

private:
  ~FourCCEnumerator() {}

protected:
  nsRefPtr<sbContentTypeFormat> mFormat;
  nsTArray<FourCC> *mArray;
  PRUint32 mIndex; // index of next item to look at
};

NS_IMPL_THREADSAFE_ISUPPORTS1(FourCCEnumerator, nsISimpleEnumerator)

/* boolean hasMoreElements (); */
NS_IMETHODIMP FourCCEnumerator::HasMoreElements(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  *_retval = mIndex < mArray->Length();
  return NS_OK;
}

/* nsISupports getNext (); */
NS_IMETHODIMP FourCCEnumerator::GetNext(nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mIndex < mArray->Length(), NS_ERROR_UNEXPECTED);
  
  nsresult rv;
  
  nsCOMPtr<nsISupportsPRUint32> fourCC =
    do_CreateInstance(NS_SUPPORTS_PRUINT32_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fourCC->SetData(mArray->SafeElementAt(mIndex, 0));
  NS_ENSURE_SUCCESS(rv, rv);
  ++mIndex;
  
  return CallQueryInterface(fourCC, _retval);
}

/******************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS1(sbContentTypeFormat, sbIContentTypeFormat)

sbContentTypeFormat::sbContentTypeFormat()
 : mHasInitialized(PR_FALSE)
{
  mInitLock = nsAutoLock::NewLock(__FILE__);
}

sbContentTypeFormat::~sbContentTypeFormat()
{
  /* destructor code */
}

/* void Init (in FourCC aContainerFormat,
              [array, size_is (aEncodingFormatsCount)] in FourCC aEncodingFormats,
              in unsigned long aEncodingFormatsCount,
              [array, size_is (aDecodingFormatsCount)] in FourCC aDecodingFormats,
              in unsigned long aDecodingFormatsCount); */
NS_IMETHODIMP sbContentTypeFormat::Init(FourCC aContainerFormat,
                                        FourCC *aEncodingFormats,
                                        PRUint32 aEncodingFormatsCount,
                                        FourCC *aDecodingFormats,
                                        PRUint32 aDecodingFormatsCount)
{
  NS_ENSURE_TRUE(mInitLock, NS_ERROR_NOT_INITIALIZED);
  
  { /* scope the initialization lock */
    nsAutoLock initLock(mInitLock);
    if (mHasInitialized)
      return NS_ERROR_ALREADY_INITIALIZED;
    mHasInitialized = PR_TRUE;
  } /* end scope for initialization lock */
  
  mContainerFormat = aContainerFormat;
  // since this is init and the only mutator, it is safe to assume that the
  // arrays are all empty at this point.  Just append is fine.
  NS_ASSERTION(mEncodingFormats.IsEmpty(), "Encoding formats before init!");
  NS_ASSERTION(mDecodingFormats.IsEmpty(), "Decoding formats before init!");
  mEncodingFormats.AppendElements(aEncodingFormats, aEncodingFormatsCount);
  mDecodingFormats.AppendElements(aDecodingFormats, aDecodingFormatsCount);
  
  return NS_OK;
}

/* readonly attribute FourCC containerFormat; */
NS_IMETHODIMP sbContentTypeFormat::GetContainerFormat(FourCC *aContainerFormat)
{
  NS_ENSURE_ARG_POINTER(aContainerFormat);
  *aContainerFormat = mContainerFormat;
  return NS_OK;
}

/* readonly attribute nsISimpleEnumerator encodingFormats; */
NS_IMETHODIMP sbContentTypeFormat::GetEncodingFormats(nsISimpleEnumerator * *aEncodingFormats)
{
  NS_ENSURE_ARG_POINTER(aEncodingFormats);
  nsRefPtr<FourCCEnumerator> enumerator =
    new FourCCEnumerator(this, &mEncodingFormats);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(enumerator.get(), aEncodingFormats);
}

/* readonly attribute nsISimpleEnumerator decodingFormats; */
NS_IMETHODIMP sbContentTypeFormat::GetDecodingFormats(nsISimpleEnumerator * *aDecodingFormats)
{
  NS_ENSURE_ARG_POINTER(aDecodingFormats);
  nsRefPtr<FourCCEnumerator> enumerator =
    new FourCCEnumerator(this, &mDecodingFormats);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
  return CallQueryInterface(enumerator.get(), aDecodingFormats);
}
