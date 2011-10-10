/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbPropertyInfo.h"
#include "sbStandardOperators.h"

#include <nsArrayEnumerator.h>
#include <nsAutoPtr.h>
#include <nsISimpleEnumerator.h>
#include <sbLockUtils.h>

/*
 *  * To log this module, set the following environment variable:
 *   *   NSPR_LOG_MODULES=sbPropInfo:5
 *    */
#include <prlog.h>
#ifdef PR_LOGGING
static PRLogModuleInfo* gPropInfoLog = nsnull;
#endif

#define LOG(args) PR_LOG(gPropInfoLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif


NS_IMPL_THREADSAFE_ISUPPORTS1(sbPropertyOperator, sbIPropertyOperator)

sbPropertyOperator::sbPropertyOperator()
: mInitialized(PR_FALSE)
{
}

sbPropertyOperator::sbPropertyOperator(const nsAString& aOperator,
                                       const nsAString& aOperatorReadable)
: mInitialized(PR_TRUE)
, mOperator(aOperator)
, mOperatorReadable(aOperatorReadable)
{
}

sbPropertyOperator::~sbPropertyOperator()
{
}

NS_IMETHODIMP sbPropertyOperator::GetOperator(nsAString & aOperator)
{
  aOperator = mOperator;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyOperator::GetOperatorReadable(nsAString & aOperatorReadable)
{
  aOperatorReadable = mOperatorReadable;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyOperator::Init(const nsAString & aOperator, const nsAString & aOperatorReadable)
{
  NS_ENSURE_TRUE(mInitialized == PR_FALSE, NS_ERROR_ALREADY_INITIALIZED);
  mOperator = aOperator;
  mOperatorReadable = aOperatorReadable;
  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(sbPropertyInfo, sbIPropertyInfo, nsISupportsWeakReference)

sbPropertyInfo::sbPropertyInfo()
: mNullSort(sbIPropertyInfo::SORT_NULL_SMALL)
, mSecondarySortLock(nsnull)
, mSecondarySort(nsnull)
, mIDLock(nsnull)
, mTypeLock(nsnull)
, mDisplayNameLock(nsnull)
, mLocalizationKeyLock(nsnull)
, mUserViewableLock(nsnull)
, mUserViewable(PR_FALSE)
, mUserEditableLock(nsnull)
, mUserEditable(PR_TRUE)
, mOperatorsLock(nsnull)
, mRemoteReadableLock(nsnull)
, mRemoteReadable(PR_FALSE)
, mRemoteWritableLock(nsnull)
, mRemoteWritable(PR_FALSE)
, mUnitConverter(nsnull)
, mUnitConverterLock(nsnull)
{
#ifdef PR_LOGGING
  if (!gPropInfoLog) {
    gPropInfoLog = PR_NewLogModule("sbPropInfo");
  }
#endif

  mSecondarySortLock = PR_NewLock();

  NS_ASSERTION(mSecondarySortLock,
    "sbPropertyInfo::mSecondarySortLock failed to create lock!");

  mIDLock = PR_NewLock();
  NS_ASSERTION(mIDLock,
    "sbPropertyInfo::mIDLock failed to create lock!");

  mTypeLock = PR_NewLock();
  NS_ASSERTION(mTypeLock,
    "sbPropertyInfo::mTypeLock failed to create lock!");

  mDisplayNameLock = PR_NewLock();
  NS_ASSERTION(mDisplayNameLock,
    "sbPropertyInfo::mDisplayNameLock failed to create lock!");

  mLocalizationKeyLock = PR_NewLock();
  NS_ASSERTION(mLocalizationKeyLock,
    "sbPropertyInfo::mLocalizationKeyLock failed to create lock!");

  mUserViewableLock = PR_NewLock();
  NS_ASSERTION(mUserViewableLock,
    "sbPropertyInfo::mUserViewableLock failed to create lock!");

  mUserEditableLock = PR_NewLock();
  NS_ASSERTION(mUserEditableLock,
    "sbPropertyInfo::mUserEditableLock failed to create lock!");

  mOperatorsLock = PR_NewLock();
  NS_ASSERTION(mOperatorsLock,
    "sbPropertyInfo::mOperatorsLock failed to create lock!");

  mRemoteReadableLock = PR_NewLock();
  NS_ASSERTION(mRemoteReadableLock,
    "sbPropertyInfo::mRemoteReadableLock failed to create lock!");

  mRemoteWritableLock = PR_NewLock();
  NS_ASSERTION(mRemoteWritableLock,
    "sbPropertyInfo::mRemoteWritableLock failed to create lock!");

  mUnitConverterLock = PR_NewLock();
  NS_ASSERTION(mUnitConverterLock,
    "sbPropertyInfo::mUnitConverterLock failed to create lock!");
}

sbPropertyInfo::~sbPropertyInfo()
{
  if(mSecondarySortLock) {
    PR_DestroyLock(mSecondarySortLock);
  }

  if(mIDLock) {
    PR_DestroyLock(mIDLock);
  }

  if(mTypeLock) {
    PR_DestroyLock(mTypeLock);
  }

  if(mDisplayNameLock) {
    PR_DestroyLock(mDisplayNameLock);
  }

  if(mLocalizationKeyLock) {
    PR_DestroyLock(mLocalizationKeyLock);
  }

  if(mUserViewableLock) {
    PR_DestroyLock(mUserViewableLock);
  }

  if(mUserEditableLock) {
    PR_DestroyLock(mUserEditableLock);
  }

  if(mOperatorsLock) {
    PR_DestroyLock(mOperatorsLock);
  }

  if(mRemoteReadableLock) {
    PR_DestroyLock(mRemoteReadableLock);
  }

  if(mRemoteWritableLock) {
    PR_DestroyLock(mRemoteWritableLock);
  }

  if(mUnitConverterLock) {
    PR_DestroyLock(mUnitConverterLock);
  }
}

nsresult
sbPropertyInfo::Init()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_ISSET(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.isset"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_ISNOTSET(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.isnotset"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_EQUALS(nsAString & aOPERATOR_EQUALS)
{
  aOPERATOR_EQUALS = NS_LITERAL_STRING(SB_OPERATOR_EQUALS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_NOTEQUALS(nsAString & aOPERATOR_NOTEQUALS)
{
  aOPERATOR_NOTEQUALS = NS_LITERAL_STRING(SB_OPERATOR_NOTEQUALS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_GREATER(nsAString & aOPERATOR_GREATER)
{
  aOPERATOR_GREATER = NS_LITERAL_STRING(SB_OPERATOR_GREATER);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_GREATEREQUAL(nsAString & aOPERATOR_GREATEREQUAL)
{
  aOPERATOR_GREATEREQUAL = NS_LITERAL_STRING(SB_OPERATOR_GREATEREQUAL);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_LESS(nsAString & aOPERATOR_LESS)
{
  aOPERATOR_LESS = NS_LITERAL_STRING(SB_OPERATOR_LESS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_LESSEQUAL(nsAString & aOPERATOR_LESSEQUAL)
{
  aOPERATOR_LESSEQUAL = NS_LITERAL_STRING(SB_OPERATOR_LESSEQUAL);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_CONTAINS(nsAString & aOPERATOR_CONTAINS)
{
  aOPERATOR_CONTAINS = NS_LITERAL_STRING(SB_OPERATOR_CONTAINS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_NOTCONTAINS(nsAString & aOPERATOR_NOTCONTAINS)
{
  aOPERATOR_NOTCONTAINS = NS_LITERAL_STRING(SB_OPERATOR_NOTCONTAINS);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_BEGINSWITH(nsAString & aOPERATOR_BEGINSWITH)
{
  aOPERATOR_BEGINSWITH = NS_LITERAL_STRING(SB_OPERATOR_BEGINSWITH);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_NOTBEGINSWITH(nsAString & aOPERATOR_NOTBEGINSWITH)
{
  aOPERATOR_NOTBEGINSWITH = NS_LITERAL_STRING(SB_OPERATOR_NOTBEGINSWITH);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_ENDSWITH(nsAString & aOPERATOR_ENDSWITH)
{
  aOPERATOR_ENDSWITH = NS_LITERAL_STRING(SB_OPERATOR_ENDSWITH);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_NOTENDSWITH(nsAString & aOPERATOR_NOTENDSWITH)
{
  aOPERATOR_NOTENDSWITH = NS_LITERAL_STRING(SB_OPERATOR_NOTENDSWITH);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_BETWEEN(nsAString & aOPERATOR_BETWEEN)
{
  aOPERATOR_BETWEEN = NS_LITERAL_STRING(SB_OPERATOR_BETWEEN);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_ISSET(nsAString & aOPERATOR_ISSET)
{
  aOPERATOR_ISSET = NS_LITERAL_STRING(SB_OPERATOR_ISSET);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOPERATOR_ISNOTSET(nsAString & aOPERATOR_ISNOTSET)
{
  aOPERATOR_ISNOTSET = NS_LITERAL_STRING(SB_OPERATOR_ISNOTSET);
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetNullSort(PRUint32 aNullSort)
{
  mNullSort = aNullSort;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::GetNullSort(PRUint32 *aNullSort)
{
  NS_ENSURE_ARG_POINTER(aNullSort);
  *aNullSort = mNullSort;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetSecondarySort(sbIPropertyArray * aSecondarySort)
{
  NS_ENSURE_ARG_POINTER(aSecondarySort);

  sbSimpleAutoLock lock(mSecondarySortLock);
  
  // XXX - Due to caching we cannot allow the secondary sort
  // to be updated more than once.  This is a nasty hack
  // and will have to be fixed with a large scale architectural
  // change.  
  //
  // See Bug 12677 – "[sorting] cached sortable values should 
  //                  auto-invalidate when property implementations change"
  //
  // If you need to change the secondary sort for a property that
  // may already be cached in the db, check out 
  // sbILocalDatabasePropertyCache.InvalidateSortData()
  if (mSecondarySort) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }
  
  mSecondarySort = aSecondarySort;

  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::GetSecondarySort(sbIPropertyArray * *aSecondarySort)
{
  NS_ENSURE_ARG_POINTER(aSecondarySort);

  sbSimpleAutoLock lock(mSecondarySortLock);
  *aSecondarySort = mSecondarySort;
  NS_IF_ADDREF(*aSecondarySort);

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetId(nsAString & aID)
{
  sbSimpleAutoLock lock(mIDLock);
  aID = mID;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetId(const nsAString &aID)
{
  LOG(( "sbPropertyInfo::SetId(%s)", NS_LossyConvertUTF16toASCII(aID).get() ));

  sbSimpleAutoLock lock(mIDLock);

  if(mID.IsEmpty()) {
    mID = aID;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetType(nsAString & aType)
{
  sbSimpleAutoLock lock(mTypeLock);
  aType = mType;
  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetType(const nsAString &aType)
{
  sbSimpleAutoLock lock(mTypeLock);

  if(mType.IsEmpty()) {
    mType = aType;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbPropertyInfo::GetDisplayName(nsAString & aDisplayName)
{
  sbSimpleAutoLock lock(mDisplayNameLock);

  if(mDisplayName.IsEmpty()) {
    sbSimpleAutoLock lock(mIDLock);
    aDisplayName = mID;
  }
  else {
    aDisplayName = mDisplayName;
  }

  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetDisplayName(const nsAString &aDisplayName)
{
  sbSimpleAutoLock lock(mDisplayNameLock);

  if(mDisplayName.IsEmpty()) {
    mDisplayName = aDisplayName;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

/* attribute AString localizationKey; */
NS_IMETHODIMP sbPropertyInfo::GetLocalizationKey(nsAString & aLocalizationKey)
{
  sbSimpleAutoLock lock(mLocalizationKeyLock);

  if(mLocalizationKey.IsEmpty()) {
    sbSimpleAutoLock lock(mIDLock);
    aLocalizationKey = mID;
  }
  else {
    aLocalizationKey = mLocalizationKey;
  }

  return NS_OK;
}
NS_IMETHODIMP sbPropertyInfo::SetLocalizationKey(const nsAString & aLocalizationKey)
{
  sbSimpleAutoLock lock(mLocalizationKeyLock);

  if(mLocalizationKey.IsEmpty()) {
    mLocalizationKey = aLocalizationKey;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}


NS_IMETHODIMP sbPropertyInfo::GetUserViewable(PRBool *aUserViewable)
{
  NS_ENSURE_ARG_POINTER(aUserViewable);

  sbSimpleAutoLock lock(mUserViewableLock);
  *aUserViewable = mUserViewable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetUserViewable(PRBool aUserViewable)
{
  sbSimpleAutoLock lock(mUserViewableLock);
  mUserViewable = aUserViewable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetUserEditable(PRBool *aUserEditable)
{
  NS_ENSURE_ARG_POINTER(aUserEditable);

  sbSimpleAutoLock lock(mUserEditableLock);
  *aUserEditable = mUserEditable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetUserEditable(PRBool aUserEditable)
{
  sbSimpleAutoLock lock(mUserEditableLock);
  mUserEditable = aUserEditable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOperators(nsISimpleEnumerator * *aOperators)
{
  NS_ENSURE_ARG_POINTER(aOperators);

  sbSimpleAutoLock lock(mOperatorsLock);
  return NS_NewArrayEnumerator(aOperators, mOperators);
}
NS_IMETHODIMP sbPropertyInfo::SetOperators(nsISimpleEnumerator * aOperators)
{
  NS_ENSURE_ARG_POINTER(aOperators);

  sbSimpleAutoLock lock(mOperatorsLock);
  mOperators.Clear();

  PRBool hasMore = PR_FALSE;
  nsCOMPtr<nsISupports> object;

  while( NS_SUCCEEDED(aOperators->HasMoreElements(&hasMore)) &&
         hasMore  &&
         NS_SUCCEEDED(aOperators->GetNext(getter_AddRefs(object)))) {
    nsresult rv;
    nsCOMPtr<sbIPropertyOperator> po = do_QueryInterface(object, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mOperators.AppendObject(po);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetOperator(const nsAString & aOperator,
                                          sbIPropertyOperator * *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  sbSimpleAutoLock lock(mOperatorsLock);

  PRUint32 length = mOperators.Count();
  for (PRUint32 i = 0; i < length; i++) {
    nsAutoString op;
    nsresult rv = mOperators[i]->GetOperator(op);
    NS_ENSURE_SUCCESS(rv, rv);
    if (op.Equals(aOperator)) {
      NS_ADDREF(*_retval = mOperators[i]);
      return NS_OK;
    }
  }

  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  // by default, the sortable value of a property is the same as the searchable
  // value. this may be changed by specific properties, for instance by text
  // properties which compute collation data for local-specific sort orders.
  return MakeSearchable(aValue, _retval);
}

NS_IMETHODIMP sbPropertyInfo::GetRemoteReadable(PRBool *aRemoteReadable)
{
  NS_ENSURE_ARG_POINTER(aRemoteReadable);

  sbSimpleAutoLock lock(mRemoteReadableLock);
  *aRemoteReadable = mRemoteReadable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetRemoteReadable(PRBool aRemoteReadable)
{
  sbSimpleAutoLock lock(mRemoteReadableLock);
  mRemoteReadable = aRemoteReadable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetRemoteWritable(PRBool *aRemoteWritable)
{
  NS_ENSURE_ARG_POINTER(aRemoteWritable);

  sbSimpleAutoLock lock(mRemoteWritableLock);
  *aRemoteWritable = mRemoteWritable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetRemoteWritable(PRBool aRemoteWritable)
{
  sbSimpleAutoLock lock(mRemoteWritableLock);
  mRemoteWritable = aRemoteWritable;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::GetUnitConverter(sbIPropertyUnitConverter **aUnitConverter)
{
  NS_ENSURE_ARG_POINTER(aUnitConverter);

  sbSimpleAutoLock lock(mUnitConverterLock);
  if (mUnitConverter) {
    NS_ADDREF(*aUnitConverter = mUnitConverter);
  } else
    *aUnitConverter = nsnull;

  return NS_OK;
}

NS_IMETHODIMP sbPropertyInfo::SetUnitConverter(sbIPropertyUnitConverter *aUnitConverter)
{
  sbSimpleAutoLock lock(mUnitConverterLock);
  mUnitConverter = aUnitConverter;
  if (mUnitConverter)  
    mUnitConverter->SetPropertyInfo(this);

  return NS_OK;
}

