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

#include "sbBooleanPropertyInfo.h"
#include <nsAutoPtr.h>

NS_IMPL_ISUPPORTS_INHERITED1(sbBooleanPropertyInfo, sbPropertyInfo,
                                                    sbIBooleanPropertyInfo)

sbBooleanPropertyInfo::sbBooleanPropertyInfo()
{
  MOZ_COUNT_CTOR(sbBooleanPropertyInfo);

  mType = NS_LITERAL_STRING("boolean");
}

sbBooleanPropertyInfo::~sbBooleanPropertyInfo()
{
  MOZ_COUNT_DTOR(sbBooleanPropertyInfo);
}

nsresult sbBooleanPropertyInfo::Init()
{
  nsresult rv;

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbBooleanPropertyInfo::InitializeOperators()
{
  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;
  nsresult rv;
  
  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.equal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.int.notequal"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP sbBooleanPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  if (aValue.EqualsLiteral("0") || aValue.EqualsLiteral("1")) {
    *_retval = PR_TRUE;
  }
  else {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP sbBooleanPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP sbBooleanPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRBool valid = PR_FALSE;

  _retval = aValue;

  rv = Validate(_retval, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if(!valid) {
    _retval.Truncate();
    rv = NS_ERROR_INVALID_ARG;
  }

  return rv;
}

NS_IMETHODIMP sbBooleanPropertyInfo::MakeSortable(const nsAString & aValue, nsAString & _retval)
{
  nsresult rv;
  PRBool valid = PR_FALSE;

  _retval = aValue;

  rv = Validate(_retval, &valid);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if(!valid) {
    _retval.Truncate();
    rv = NS_ERROR_INVALID_ARG;
  }

  return rv;
}
