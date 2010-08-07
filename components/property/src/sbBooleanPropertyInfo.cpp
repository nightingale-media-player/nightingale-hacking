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

#include "sbBooleanPropertyInfo.h"
#include "sbStandardOperators.h"
#include <nsAutoPtr.h>
#include <nsITreeView.h>

NS_IMPL_ISUPPORTS_INHERITED3(sbBooleanPropertyInfo,
                             sbPropertyInfo,
                             sbIBooleanPropertyInfo,
                             sbIClickablePropertyInfo,
                             sbITreeViewPropertyInfo)

sbBooleanPropertyInfo::sbBooleanPropertyInfo()
{
  MOZ_COUNT_CTOR(sbBooleanPropertyInfo);
  mType = NS_LITERAL_STRING("boolean");
  mSuppressSelect = PR_TRUE;
}

sbBooleanPropertyInfo::~sbBooleanPropertyInfo()
{
  MOZ_COUNT_DTOR(sbBooleanPropertyInfo);
}

nsresult
sbBooleanPropertyInfo::Init()
{
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitializeOperators();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBooleanPropertyInfo::InitializeOperators()
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

  rv = GetOPERATOR_ISTRUE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.bool.istrue"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetOPERATOR_ISFALSE(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op, NS_LITERAL_STRING("&smart.bool.isfalse"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  if (aValue.IsVoid() ||
      aValue.IsEmpty() ||
      aValue.EqualsLiteral("0") || 
      aValue.EqualsLiteral("1")) {
    *_retval = PR_TRUE;
  }
  else {
    *_retval = PR_FALSE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
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

NS_IMETHODIMP
sbBooleanPropertyInfo::GetOPERATOR_ISTRUE(nsAString & aOPERATOR_ISTRUE)
{
  aOPERATOR_ISTRUE = NS_LITERAL_STRING(SB_OPERATOR_ISTRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetOPERATOR_ISFALSE(nsAString & aOPERATOR_ISFALSE)
{
  aOPERATOR_ISFALSE = NS_LITERAL_STRING(SB_OPERATOR_ISFALSE);
  return NS_OK;
}

// sbITreeViewPropertyInfo

NS_IMETHODIMP
sbBooleanPropertyInfo::GetImageSrc(const nsAString& aValue,
                                   nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetProgressMode(const nsAString& aValue,
                                       PRInt32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsITreeView::PROGRESS_NONE;
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetCellValue(const nsAString& aValue,
                                    nsAString& _retval)
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

  // Force empty or nulls to be false.
  if (aValue.IsVoid() || aValue.IsEmpty()) {
    _retval.AssignLiteral("0");
  }

  return rv;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetRowProperties(const nsAString& aValue,
                                        nsAString& _retval)
{
  _retval.Truncate();
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetCellProperties(const nsAString& aValue,
                                         nsAString& _retval)
{
  _retval.AssignLiteral("checkbox");
  
  if (aValue.EqualsLiteral("1")) {
    _retval.AppendLiteral(" checked");
  } else {
    _retval.AppendLiteral(" unchecked");
  }
  
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetColumnType(nsAString& _retval)
{
  _retval.AssignLiteral("text");
  return NS_OK;
}

// sbIClickablePropertyInfo

NS_IMETHODIMP
sbBooleanPropertyInfo::GetSuppressSelect(PRBool* aSuppressSelect)
{
  NS_ENSURE_ARG_POINTER(aSuppressSelect);
  *aSuppressSelect = mSuppressSelect;
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::SetSuppressSelect(PRBool aSuppressSelect)
{
  mSuppressSelect = aSuppressSelect;
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::IsDisabled(const nsAString& aCurrentValue,
                                  PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::HitTest(const nsAString& aCurrentValue,
                               const nsAString& aPart,
                               PRUint32 aBoxWidth,
                               PRUint32 aBoxHeight,
                               PRUint32 aMouseX,
                               PRUint32 aMouseY,
                               PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbBooleanPropertyInfo::GetValueForClick(const nsAString& aCurrentValue,
                                        PRUint32 aBoxWidth,
                                        PRUint32 aBoxHeight,
                                        PRUint32 aMouseX,
                                        PRUint32 aMouseY,
                                        nsAString& _retval)
{
  if (aCurrentValue.EqualsLiteral("1")) {
    _retval.AssignLiteral("0");
  } else {
    _retval.AssignLiteral("1");
  }
  
  return NS_OK;
}

/* boolean onClick (in sbIMediaItem aItem,
                    [optional] in nsISupports aEvent,
                    [optional] in nsISupports aContext); */
NS_IMETHODIMP
sbBooleanPropertyInfo::OnClick(sbIMediaItem *aItem,
                               nsISupports *aEvent,
                               nsISupports *aContext,
                               PRBool *_retval NS_OUTPARAM)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;
  return NS_OK;
}
