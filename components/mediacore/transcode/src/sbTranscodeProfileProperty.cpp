/* vim: set sw=2 : */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbTranscodeProfileProperty.h"

#include <nsIVariant.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeProfileProperty,
                              sbITranscodeProfileProperty)

sbTranscodeProfileProperty::sbTranscodeProfileProperty()
  : mHidden(PR_FALSE),
    mScale(NS_LITERAL_CSTRING("1/1"))
{
}

sbTranscodeProfileProperty::~sbTranscodeProfileProperty()
{
}

nsresult
sbTranscodeProfileProperty::SetPropertyName(const nsAString & aPropertyName)
{
  mPropertyName = aPropertyName;
  return NS_OK;
}

nsresult
sbTranscodeProfileProperty::SetValueMin(nsIVariant * aValueMin)
{
  mValueMin = aValueMin;
  return NS_OK;
}

nsresult
sbTranscodeProfileProperty::SetValueMax(nsIVariant * aValueMax)
{
  mValueMax = aValueMax;
  return NS_OK;
}

nsresult
sbTranscodeProfileProperty::SetHidden(const PRBool aHidden)
{
  mHidden = aHidden;
  return NS_OK;
}

nsresult
sbTranscodeProfileProperty::SetMapping(const nsACString & aMapping)
{
  mMapping = aMapping;
  return NS_OK;
}

nsresult
sbTranscodeProfileProperty::SetScale(const nsACString & aScale)
{
  mScale = aScale;
  return NS_OK;
}

/* readonly attribute AString propertyName; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetPropertyName(nsAString & aPropertyName)
{
  aPropertyName.Assign(mPropertyName);
  return NS_OK;
}

/* readonly attribute nsIVariant valueMin; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetValueMin(nsIVariant * *aValueMin)
{
  NS_ENSURE_ARG_POINTER(aValueMin);
  NS_IF_ADDREF(*aValueMin = mValueMin);
  return NS_OK;
}

/* readonly attribute nsIVariant valueMax; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetValueMax(nsIVariant * *aValueMax)
{
  NS_ENSURE_ARG_POINTER(aValueMax);
  NS_IF_ADDREF(*aValueMax = mValueMax);
  return NS_OK;
}

/* attribute nsIVariant value; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetValue(nsIVariant * *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  NS_IF_ADDREF(*aValue = mValue);
  return NS_OK;
}
NS_IMETHODIMP
sbTranscodeProfileProperty::SetValue(nsIVariant * aValue)
{
  mValue = aValue;
  return NS_OK;
}

/* readonly attribute boolean hidden; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetHidden(PRBool *aHidden)
{
  NS_ENSURE_ARG_POINTER(aHidden);
  *aHidden = mHidden;
  return NS_OK;
}

/* readonly attribute ACString mapping; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetMapping(nsACString & aMapping)
{
  aMapping = mMapping;
  return NS_OK;
}

/* readonly attribute ACString scale; */
NS_IMETHODIMP
sbTranscodeProfileProperty::GetScale(nsACString & aScale)
{
  aScale = mScale;
  return NS_OK;
}
