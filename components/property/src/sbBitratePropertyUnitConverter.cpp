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

// Property Unit Converter for bitrate values, ie: bps, kbps, Mbps

#include "sbBitratePropertyUnitConverter.h"
#include <math.h>
#include <stdlib.h>

// ctor - register all units
sbBitratePropertyUnitConverter::sbBitratePropertyUnitConverter() 
{
  SetStringBundle(NS_LITERAL_STRING("chrome://songbird/locale/songbird.properties"));
  RegisterUnit(BITRATE_UNIT_BPS,
    NS_LITERAL_STRING("bps"),
    NS_LITERAL_STRING("&bitrate.unit.bitspersecond"), 
    NS_LITERAL_STRING("&bitrate.unit.bitspersecond.short"));
  RegisterUnit(BITRATE_UNIT_KBPS,
    NS_LITERAL_STRING("kbps"),
    NS_LITERAL_STRING("&bitrate.unit.kilobitspersecond"),
    NS_LITERAL_STRING("&bitrate.unit.kilobitspersecond.short"),
    PR_TRUE);
  RegisterUnit(BITRATE_UNIT_MBPS,
    NS_LITERAL_STRING("mbps"),
    NS_LITERAL_STRING("&bitrate.unit.megabitspersecond"),
    NS_LITERAL_STRING("&bitrate.unit.megabitspersecond.short"));
}

// dtor
sbBitratePropertyUnitConverter::~sbBitratePropertyUnitConverter() 
{
}

// convert from the native unit to any of the exposed units
NS_IMETHODIMP
sbBitratePropertyUnitConverter::ConvertFromNativeToUnit(PRFloat64 aValue, PRUint32 aUnitID, PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case BITRATE_UNIT_BPS:
      aValue *= 1000.0;
      break;
    case BITRATE_UNIT_KBPS:
      // native unit, nothing to do
      break;
    case BITRATE_UNIT_MBPS:
      aValue /= 1000.0;
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

// convert from any of the exposed units back to the native unit
NS_IMETHODIMP
sbBitratePropertyUnitConverter::ConvertFromUnitToNative(PRFloat64 aValue, PRUint32 aUnitID, PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case BITRATE_UNIT_BPS:
      aValue /= 1000.0;
      break;
    case BITRATE_UNIT_KBPS:
      // native unit, nothing to do
      break;
    case BITRATE_UNIT_MBPS:
      aValue *= 1000.0;
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

PRInt32 sbBitratePropertyUnitConverter::GetAutoUnit(const PRFloat64 aValue) {
  // get number of digits
  PRUint32 d;
  if (aValue == 0) d = 1;
  else d = (PRUint32)(log10(fabs(aValue)) + 1);

  // and pick most suitable unit
  if (d <= 1) return BITRATE_UNIT_BPS;
  if (d < 4) return BITRATE_UNIT_KBPS;
  return BITRATE_UNIT_MBPS;
}
