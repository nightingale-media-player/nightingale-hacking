/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbTranscodeProfileAttribute.h"

#include <nsIVariant.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeProfileAttribute,
                              sbITranscodeProfileAttribute)

sbTranscodeProfileAttribute::sbTranscodeProfileAttribute()
{
}

sbTranscodeProfileAttribute::~sbTranscodeProfileAttribute()
{
}

nsresult
sbTranscodeProfileAttribute::SetName(const nsAString & aName)
{
  mName = aName;
  return NS_OK;
}

nsresult
sbTranscodeProfileAttribute::SetValue(nsIVariant * aValue)
{
  mValue = aValue;
  return NS_OK;
}

/* readonly attribute AString name; */
NS_IMETHODIMP
sbTranscodeProfileAttribute::GetName(nsAString & aName)
{
  aName.Assign(mName);
  return NS_OK;
}

/* readonly attribute nsIVariant value; */
NS_IMETHODIMP
sbTranscodeProfileAttribute::GetValue(nsIVariant * *aValue)
{
  NS_ENSURE_ARG_POINTER(aValue);
  NS_IF_ADDREF(*aValue = mValue);
  return NS_OK;
}

