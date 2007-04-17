/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#include <nsAutoLock.h>
#include <nsAutoPtr.h>

#include <nsServiceManagerUtils.h>
#include <nsNetUtil.h>

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

  InitializeOperators();
}

sbURIPropertyInfo::~sbURIPropertyInfo()
{
  if(mURISchemeConstraintLock) {
    PR_DestroyLock(mURISchemeConstraintLock);
  }
}

void sbURIPropertyInfo::InitializeOperators()
{
  nsAutoString op;
  nsAutoPtr<sbPropertyOperator> propOp;

  sbPropertyInfo::GetOPERATOR_CONTAINS(op);
  propOp =  new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.contains"));
  mOperators.AppendObject(propOp);
  propOp.forget();

  sbPropertyInfo::GetOPERATOR_BEGINSWITH(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.starts"));
  mOperators.AppendObject(propOp);
  propOp.forget();

  sbPropertyInfo::GetOPERATOR_ENDSWITH(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.ends"));
  mOperators.AppendObject(propOp);
  propOp.forget();

  sbPropertyInfo::GetOPERATOR_EQUALS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is"));
  mOperators.AppendObject(propOp);
  propOp.forget();

  sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.text.is_not"));
  mOperators.AppendObject(propOp);
  propOp.forget();

  return;
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

  nsAutoLock lock(mURISchemeConstraintLock);
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

NS_IMETHODIMP sbURIPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv = EnsureIOService();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> uri;
  rv = NS_NewURI(getter_AddRefs(uri), aValue, nsnull, nsnull, mIOService);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoLock lock(mURISchemeConstraintLock);
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

  nsAutoLock lock(mURISchemeConstraintLock);
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
  nsAutoLock lock(mURISchemeConstraintLock);
  aConstrainScheme = mURISchemeConstraint;
  return NS_OK;
}
NS_IMETHODIMP sbURIPropertyInfo::SetConstrainScheme(const nsAString & aConstrainScheme)
{
  nsAutoLock lock(mURISchemeConstraintLock);

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
