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

#include "sbURIPropertyInfo.h"

#include <nsAutoPtr.h>

#include <nsServiceManagerUtils.h>
#include <nsNetUtil.h>

#include <sbLockUtils.h>

NS_IMPL_ADDREF_INHERITED(sbURIPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbURIPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbURIPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbURIPropertyInfo, sbIURIPropertyInfo)
NS_INTERFACE_TABLE_ENTRY_AMBIGUOUS(sbURIPropertyInfo, sbIPropertyInfo, sbIURIPropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbURIPropertyInfo::sbURIPropertyInfo()
: mURISchemeConstraintLock(nsnull)
, mIOServiceLock(nsnull)
{
  mType = NS_LITERAL_STRING("uri");

  mURISchemeConstraintLock = PR_NewLock();
  NS_ASSERTION(mURISchemeConstraintLock,
    "sbURIPropertyInfo::mURISchemeConstraintLock failed to create lock!");

  mIOServiceLock = PR_NewLock();
  NS_ASSERTION(mIOServiceLock,
    "sbURIPropertyInfo::mIOServiceLock failed to create lock!");
}

sbURIPropertyInfo::~sbURIPropertyInfo()
{
  if(mURISchemeConstraintLock) {
    PR_DestroyLock(mURISchemeConstraintLock);
  }
}

nsresult
sbURIPropertyInfo::Init()
{
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbURIPropertyInfo::InitializeOperators()
{
  nsresult rv;
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;

  rv = sbPropertyInfo::GetOPERATOR_CONTAINS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.contains"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTCONTAINS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_contain"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is_not"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_BEGINSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.starts"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTBEGINSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_start"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_ENDSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.ends"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTENDSWITH(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.not_end"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbURIPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;

  nsresult rv = EnsureIOService();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
  if(NS_FAILED(rv)) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  sbSimpleAutoLock lock(mURISchemeConstraintLock);
  if(!mURISchemeConstraint.IsEmpty()) {
    NS_ConvertUTF16toUTF8 narrow(mURISchemeConstraint);
    PRBool valid = PR_FALSE;

    rv = uri->SchemeIs(narrow.get(), &valid);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!valid) {
      *_retval = PR_FALSE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP sbURIPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbURIPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  if (aValue.IsVoid()) {
    _retval.Truncate();
    return NS_OK;
  }

  nsresult rv = EnsureIOService();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
  NS_ENSURE_SUCCESS(rv, rv);

  sbSimpleAutoLock lock(mURISchemeConstraintLock);
  if(!mURISchemeConstraint.IsEmpty()) {
    NS_ConvertUTF16toUTF8 narrow(mURISchemeConstraint);
    PRBool valid = PR_FALSE;

    rv = uri->SchemeIs(narrow.get(), &valid);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!valid) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCAutoString spec;
  rv = uri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF8toUTF16 wide(spec);
  _retval = wide;

  return rv;
}

NS_IMETHODIMP sbURIPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv = EnsureIOService();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
  NS_ENSURE_SUCCESS(rv, rv);

  sbSimpleAutoLock lock(mURISchemeConstraintLock);
  if(!mURISchemeConstraint.IsEmpty()) {
    NS_ConvertUTF16toUTF8 narrow(mURISchemeConstraint);
    PRBool valid = PR_FALSE;

    rv = uri->SchemeIs(narrow.get(), &valid);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!valid) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCAutoString spec;
  rv = uri->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF8toUTF16 wide(spec);
  _retval = wide;

  return rv;
}

NS_IMETHODIMP sbURIPropertyInfo::GetConstrainScheme(nsAString & aConstrainScheme)
{
  sbSimpleAutoLock lock(mURISchemeConstraintLock);
  aConstrainScheme = mURISchemeConstraint;
  return NS_OK;
}
NS_IMETHODIMP sbURIPropertyInfo::SetConstrainScheme(const nsAString & aConstrainScheme)
{
  sbSimpleAutoLock lock(mURISchemeConstraintLock);

  if(mURISchemeConstraint.IsEmpty()) {
    mURISchemeConstraint = aConstrainScheme;
    return NS_OK;
  }

  return NS_ERROR_ALREADY_INITIALIZED;
}

NS_IMETHODIMP sbURIPropertyInfo::EnsureIOService()
{
  nsresult rv = NS_OK;

  if(!mIOService) {
    mIOService = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  }

  return rv;
}
