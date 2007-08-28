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

#include "sbButtonPropertyInfo.h"

NS_IMPL_ADDREF_INHERITED(sbButtonPropertyInfo, sbPropertyInfo);
NS_IMPL_RELEASE_INHERITED(sbButtonPropertyInfo, sbPropertyInfo);

NS_INTERFACE_TABLE_HEAD(sbButtonPropertyInfo)
NS_INTERFACE_TABLE_BEGIN
NS_INTERFACE_TABLE_ENTRY(sbButtonPropertyInfo, sbIButtonPropertyInfo)
NS_INTERFACE_TABLE_ENTRY(sbButtonPropertyInfo, sbIPropertyInfo)
NS_INTERFACE_TABLE_ENTRY(sbButtonPropertyInfo, sbIRemotePropertyInfo)
NS_INTERFACE_TABLE_END
NS_INTERFACE_TABLE_TAIL_INHERITING(sbPropertyInfo)

sbButtonPropertyInfo::sbButtonPropertyInfo()
{
  MOZ_COUNT_CTOR(sbButtonPropertyInfo);
}

sbButtonPropertyInfo::~sbButtonPropertyInfo()
{
  MOZ_COUNT_DTOR(sbButtonPropertyInfo);
}

NS_IMETHODIMP
sbButtonPropertyInfo::GetLabel(nsAString& aLabel)
{
  aLabel = mLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbButtonPropertyInfo::SetLabel(const nsAString& aLabel)
{
  mLabel = aLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbButtonPropertyInfo::Format(const nsAString& aValue,
                             nsAString& _retval)
{
  _retval = mLabel;
  return NS_OK;
}

NS_IMETHODIMP
sbButtonPropertyInfo::GetDisplayPropertiesForValue(const nsAString& aValue,
                                                   nsAString& _retval)
{
  _retval.AssignLiteral("button");
  return NS_OK;
}

