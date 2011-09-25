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

#include "sbImmutablePropertyInfo.h"
#include "sbStandardOperators.h"
#include "sbPropertyOperator.h"
#include <nsAutoPtr.h>

#include <nsIStringBundle.h>

#include <nsArrayEnumerator.h>
#include <sbIPropertyManager.h>
#include <sbLockUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(sbImmutablePropertyInfo, sbIPropertyInfo, nsISupportsWeakReference)

sbImmutablePropertyInfo::sbImmutablePropertyInfo() :
  mNullSort(sbIPropertyInfo::SORT_NULL_SMALL),
  mUserViewable(PR_FALSE),
  mUserEditable(PR_FALSE),
  mRemoteReadable(PR_FALSE),
  mRemoteWritable(PR_FALSE),
  mOperatorsLock(nsnull),
  mUnitConverter(nsnull)
{
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  mOperatorsLock = PR_NewLock();
  NS_ASSERTION(mOperatorsLock,
    "sbImmutablePropertyInfo::mOperatorsLock failed to create lock!");
}

sbImmutablePropertyInfo::~sbImmutablePropertyInfo() {
  if(mOperatorsLock) {
    PR_DestroyLock(mOperatorsLock);
  }
}

nsresult
sbImmutablePropertyInfo::Init()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbImmutablePropertyInfo::GetOPERATOR_ISSET(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.isset"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbImmutablePropertyInfo::GetOPERATOR_ISNOTSET(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.isnotset"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_EQUALS(nsAString& aOPERATOR_EQUALS)
{
  aOPERATOR_EQUALS.AssignLiteral(SB_OPERATOR_EQUALS);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_NOTEQUALS(nsAString& aOPERATOR_NOTEQUALS)
{
  aOPERATOR_NOTEQUALS.AssignLiteral(SB_OPERATOR_NOTEQUALS);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_GREATER(nsAString& aOPERATOR_GREATER)
{
  aOPERATOR_GREATER.AssignLiteral(SB_OPERATOR_GREATER);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_GREATEREQUAL(nsAString& aOPERATOR_GREATEREQUAL)
{
  aOPERATOR_GREATEREQUAL.AssignLiteral(SB_OPERATOR_GREATEREQUAL);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_LESS(nsAString& aOPERATOR_LESS)
{
  aOPERATOR_LESS.AssignLiteral(SB_OPERATOR_LESS);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_LESSEQUAL(nsAString& aOPERATOR_LESSEQUAL)
{
  aOPERATOR_LESSEQUAL.AssignLiteral(SB_OPERATOR_LESSEQUAL);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_CONTAINS(nsAString& aOPERATOR_CONTAINS)
{
  aOPERATOR_CONTAINS.AssignLiteral(SB_OPERATOR_CONTAINS);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_NOTCONTAINS(nsAString& aOPERATOR_NOTCONTAINS)
{
  aOPERATOR_NOTCONTAINS.AssignLiteral(SB_OPERATOR_NOTCONTAINS);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_BEGINSWITH(nsAString& aOPERATOR_BEGINSWITH)
{
  aOPERATOR_BEGINSWITH.AssignLiteral(SB_OPERATOR_BEGINSWITH);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_NOTBEGINSWITH(nsAString& aOPERATOR_NOTBEGINSWITH)
{
  aOPERATOR_NOTBEGINSWITH.AssignLiteral(SB_OPERATOR_NOTBEGINSWITH);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_ENDSWITH(nsAString& aOPERATOR_ENDSWITH)
{
  aOPERATOR_ENDSWITH.AssignLiteral(SB_OPERATOR_ENDSWITH);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_NOTENDSWITH(nsAString& aOPERATOR_NOTENDSWITH)
{
  aOPERATOR_NOTENDSWITH.AssignLiteral(SB_OPERATOR_NOTENDSWITH);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOPERATOR_BETWEEN(nsAString& aOPERATOR_BETWEEN)
{
  aOPERATOR_BETWEEN.AssignLiteral(SB_OPERATOR_BETWEEN);
  return NS_OK;
}

NS_IMETHODIMP 
sbImmutablePropertyInfo::GetOPERATOR_ISSET(nsAString & aOPERATOR_ISSET)
{
  aOPERATOR_ISSET = NS_LITERAL_STRING(SB_OPERATOR_ISSET);
  return NS_OK;
}

NS_IMETHODIMP 
sbImmutablePropertyInfo::GetOPERATOR_ISNOTSET(nsAString & aOPERATOR_ISNOTSET)
{
  aOPERATOR_ISNOTSET = NS_LITERAL_STRING(SB_OPERATOR_ISNOTSET);
  return NS_OK;
}


NS_IMETHODIMP
sbImmutablePropertyInfo::SetNullSort(PRUint32 aNullSort)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbImmutablePropertyInfo::GetNullSort(PRUint32* aNullSort)
{
  NS_ENSURE_ARG_POINTER(aNullSort);
  *aNullSort = mNullSort;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::SetSecondarySort(sbIPropertyArray* aSecondarySort)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
sbImmutablePropertyInfo::GetSecondarySort(sbIPropertyArray** aSecondarySort)
{
  NS_ENSURE_ARG_POINTER(aSecondarySort);
  NS_IF_ADDREF(*aSecondarySort = mSecondarySort);
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetId(nsAString& aID)
{
  aID = mID;
  return NS_OK;
}
NS_IMETHODIMP
sbImmutablePropertyInfo::SetId(const nsAString& aID)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetType(nsAString& aType)
{
  aType = mType;
  return NS_OK;
}
NS_IMETHODIMP
sbImmutablePropertyInfo::SetType(const nsAString& aType)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetDisplayName(nsAString& aDisplayName)
{
  aDisplayName = mDisplayName;
  return NS_OK;
}
NS_IMETHODIMP
sbImmutablePropertyInfo::SetDisplayName(const nsAString& aDisplayName)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute AString localizationKey; */
NS_IMETHODIMP
sbImmutablePropertyInfo::GetLocalizationKey(nsAString & aLocalizationKey)
{
  aLocalizationKey = mLocalizationKey;
  return NS_OK;
}
NS_IMETHODIMP
sbImmutablePropertyInfo::SetLocalizationKey(const nsAString & aLocalizationKey)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetUserViewable(PRBool *aUserViewable)
{
  NS_ENSURE_ARG_POINTER(aUserViewable);
  *aUserViewable = mUserViewable;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::SetUserViewable(PRBool aUserViewable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetUserEditable(PRBool* aUserEditable)
{
  NS_ENSURE_ARG_POINTER(aUserEditable);
  *aUserEditable = mUserEditable;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::SetUserEditable(PRBool aUserEditable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetRemoteReadable(PRBool* aRemoteReadable)
{
  NS_ENSURE_ARG_POINTER(aRemoteReadable);
  *aRemoteReadable = mRemoteReadable;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::SetRemoteReadable(PRBool aRemoteReadable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetRemoteWritable(PRBool* aRemoteWritable)
{
  NS_ENSURE_ARG_POINTER(aRemoteWritable);
  *aRemoteWritable = mRemoteWritable;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::SetRemoteWritable(PRBool aRemoteWritable)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOperators(nsISimpleEnumerator** aOperators)
{
  NS_ENSURE_ARG_POINTER(aOperators);

  sbSimpleAutoLock lock(mOperatorsLock);
  return NS_NewArrayEnumerator(aOperators, mOperators);
}
NS_IMETHODIMP
sbImmutablePropertyInfo::SetOperators(nsISimpleEnumerator* aOperators)
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

NS_IMETHODIMP
sbImmutablePropertyInfo::GetOperator(const nsAString& aOperator,
                                     sbIPropertyOperator** _retval)
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

NS_IMETHODIMP
sbImmutablePropertyInfo::Validate(const nsAString& aValue,
                                  PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::Sanitize(const nsAString& aValue,
                                  nsAString& _retval)
{
  _retval = aValue;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::Format(const nsAString& aValue,
                                nsAString& _retval)
{
  _retval = aValue;
  return NS_OK;
}

NS_IMETHODIMP
sbImmutablePropertyInfo::MakeSortable(const nsAString& aValue,
                                      nsAString& _retval)
{
  // by default, the sortable value of a property is the same as the searchable
  // value. this may be changed by specific properties, for instance by text
  // properties which compute collation data for local-specific sort orders.
  return MakeSearchable(aValue, _retval);
}

NS_IMETHODIMP
sbImmutablePropertyInfo::MakeSearchable(const nsAString& aValue,
                                        nsAString& _retval)
{
  _retval = aValue;
  return NS_OK;
}

NS_IMETHODIMP 
sbImmutablePropertyInfo::GetUnitConverter(sbIPropertyUnitConverter **retVal)
{
  *retVal = mUnitConverter;
  return NS_OK;
}

NS_IMETHODIMP 
sbImmutablePropertyInfo::SetUnitConverter(sbIPropertyUnitConverter *aUnitConverter)
{
  mUnitConverter = aUnitConverter;
  if (mUnitConverter)  
    mUnitConverter->SetPropertyInfo(this);
  return NS_OK;
}

