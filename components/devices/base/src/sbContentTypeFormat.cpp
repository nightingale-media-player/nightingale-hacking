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
#include <nsISupportsPrimitives.h>

#include "sbTArrayStringEnumerator.h"


#if 0
class ContentTypeEnumerator : public nsISimpleEnumerator {
public:
  NS_DECL_NSISIMPLEENUMERATOR;
  NS_DECL_ISUPPORTS;
protected:
  friend class sbContentTypeFormat;
  FourCCEnumerator(sbContentTypeFormat* aFormat,
                   nsTArray<nsCString>* aArray)
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
#endif

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

/* void Init (in string aContainerFormat,
              [array, size_is (aEncodingFormatsCount)] in string aEncodingFormats,
              in unsigned long aEncodingFormatsCount,
              [array, size_is (aDecodingFormatsCount)] in string aDecodingFormats,
              in unsigned long aDecodingFormatsCount); */
NS_IMETHODIMP sbContentTypeFormat::Init(const char* aContainerFormat,
                                        const char**aEncodingFormats,
                                        PRUint32 aEncodingFormatsCount,
                                        const char**aDecodingFormats,
                                        PRUint32 aDecodingFormatsCount)
{
  NS_ENSURE_TRUE(mInitLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aContainerFormat);
  NS_ENSURE_ARG_POINTER(aEncodingFormats);
  NS_ENSURE_ARG_POINTER(aDecodingFormats);
  
  { /* scope the initialization lock */
    nsAutoLock initLock(mInitLock);
    if (mHasInitialized)
      return NS_ERROR_ALREADY_INITIALIZED;
    mHasInitialized = PR_TRUE;
  } /* end scope for initialization lock */
  
  mContainerFormat.Assign(aContainerFormat);
  // since this is init and the only mutator, it is safe to assume that the
  // arrays are all empty at this point.  Just append is fine.
  
  NS_ASSERTION(mEncodingFormats.IsEmpty(), "Encoding formats before init!");
  mEncodingFormats.SetCapacity(aEncodingFormatsCount);
  for (PRUint32 i = 0; i < aEncodingFormatsCount; ++i) {
    mEncodingFormats.AppendElement(nsDependentCString(aEncodingFormats[i]));
  }
  mEncodingFormats.Compact();

  NS_ASSERTION(mDecodingFormats.IsEmpty(), "Decoding formats before init!");
  mDecodingFormats.SetCapacity(aDecodingFormatsCount);
  for (PRUint32 i = 0; i < aDecodingFormatsCount; ++i) {
    mDecodingFormats.AppendElement(nsDependentCString(aDecodingFormats[i]));
  }
  mDecodingFormats.Compact();
  
  return NS_OK;
}

/* readonly attribute ACString containerFormat; */
NS_IMETHODIMP sbContentTypeFormat::GetContainerFormat(nsACString & aContainerFormat)
{
  aContainerFormat.Assign(mContainerFormat);
  return NS_OK;
}

/* readonly attribute nsIUTF8StringEnumerator encodingFormats; */
NS_IMETHODIMP sbContentTypeFormat::GetEncodingFormats(nsIUTF8StringEnumerator * *aEncodingFormats)
{
  NS_ENSURE_ARG_POINTER(aEncodingFormats);
  NS_IF_ADDREF(*aEncodingFormats = new sbTArrayCStringEnumerator(&mEncodingFormats));
  return *aEncodingFormats ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* readonly attribute nsIUTF8StringEnumerator decodingFormats; */
NS_IMETHODIMP sbContentTypeFormat::GetDecodingFormats(nsIUTF8StringEnumerator * *aDecodingFormats)
{
  NS_ENSURE_ARG_POINTER(aDecodingFormats);
  NS_IF_ADDREF(*aDecodingFormats = new sbTArrayCStringEnumerator(&mDecodingFormats));
  return *aDecodingFormats ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}
