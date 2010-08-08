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

#include "sbTrackTypeTextPropertyInfo.h"

#include <sbStringUtils.h>

#define SB_TRACKTYPE_PROPERTY_KEY "property.track_type.local"

sbTrackTypeTextPropertyInfo::sbTrackTypeTextPropertyInfo()
  : sbTextPropertyInfo()
{
  mType = NS_LITERAL_STRING("text");
}

NS_IMETHODIMP sbTrackTypeTextPropertyInfo::Format(const nsAString & aValue,
                                                  nsAString & _retval)
{
  nsresult rv;

  // If this is a track type column and the value is empty, return local type
  // by default.
  if (aValue.IsEmpty()) {
    if (mTrackType.IsEmpty()) {
      mTrackType = SBLocalizedString(SB_TRACKTYPE_PROPERTY_KEY);
    }
    _retval.Assign(mTrackType);
    return NS_OK;
  }

  rv = sbTextPropertyInfo::Format(aValue, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}
