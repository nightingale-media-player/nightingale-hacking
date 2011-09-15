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

// Property Unit Converter for duration values, ie: m, s, min, hours, days, 
// weeks, months, years

#include "sbDurationPropertyUnitConverter.h"

// ctor - register all units
sbDurationPropertyUnitConverter::sbDurationPropertyUnitConverter() 
{
  SetStringBundle(NS_LITERAL_STRING("chrome://songbird/locale/songbird.properties"));
  RegisterUnit(DURATION_UNIT_MICROSECONDS,
    NS_LITERAL_STRING("us"),
    NS_LITERAL_STRING("&duration.unit.microseconds"), 
    NS_LITERAL_STRING("&duration.unit.microseconds.short"), 
    PR_TRUE);
  RegisterUnit(DURATION_UNIT_MILLISECONDS,
    NS_LITERAL_STRING("ms"),
    NS_LITERAL_STRING("&duration.unit.milliseconds"), 
    NS_LITERAL_STRING("&duration.unit.milliseconds.short"));
  RegisterUnit(DURATION_UNIT_SECONDS,
    NS_LITERAL_STRING("sec"),
    NS_LITERAL_STRING("&duration.unit.seconds"),
    NS_LITERAL_STRING("&duration.unit.seconds.short"));
  RegisterUnit(DURATION_UNIT_MINUTES,
    NS_LITERAL_STRING("min"),
    NS_LITERAL_STRING("&duration.unit.minutes"),
    NS_LITERAL_STRING("&duration.unit.minutes.short"));
  RegisterUnit(DURATION_UNIT_HOURS,
    NS_LITERAL_STRING("h"),
    NS_LITERAL_STRING("&duration.unit.hours"),
    NS_LITERAL_STRING("&duration.unit.hours.short"));
  RegisterUnit(DURATION_UNIT_DAYS,
    NS_LITERAL_STRING("d"),
    NS_LITERAL_STRING("&duration.unit.days"),
    NS_LITERAL_STRING("&duration.unit.days.short"));
  RegisterUnit(DURATION_UNIT_WEEKS,
    NS_LITERAL_STRING("w"),
    NS_LITERAL_STRING("&duration.unit.weeks"),
    NS_LITERAL_STRING("&duration.unit.weeks.short"));
  RegisterUnit(DURATION_UNIT_MONTHS,
    NS_LITERAL_STRING("m"),
    NS_LITERAL_STRING("&duration.unit.months"),
    NS_LITERAL_STRING("&duration.unit.months.short"));
  RegisterUnit(DURATION_UNIT_YEARS,
    NS_LITERAL_STRING("y"),
    NS_LITERAL_STRING("&duration.unit.years"),
    NS_LITERAL_STRING("&duration.unit.years.short"));
}

// dtor
sbDurationPropertyUnitConverter::~sbDurationPropertyUnitConverter() 
{
}

// convert from the native unit to any of the exposed units
NS_IMETHODIMP
sbDurationPropertyUnitConverter::ConvertFromNativeToUnit(PRFloat64 aValue, 
                                                         PRUint32 aUnitID, 
                                                         PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case DURATION_UNIT_MICROSECONDS:
      // native unit, nothing to do
      break;
    case DURATION_UNIT_MILLISECONDS:
      aValue /= 1000.0;
      break;
    case DURATION_UNIT_SECONDS:
      aValue /= (1000.0*1000.0);
      break;
    case DURATION_UNIT_MINUTES:
      aValue /= (1000.0*1000.0*60.0);
      break;
    case DURATION_UNIT_HOURS:
      aValue /= (1000.0*1000.0*60.0*60.0);
      break;
    case DURATION_UNIT_DAYS:
      aValue /= (1000.0*1000.0*60.0*60.0*24.0);
      break;
    case DURATION_UNIT_WEEKS:
      aValue /= (1000.0*1000.0*60.0*60.0*24.0*7.0);
      break;
    case DURATION_UNIT_MONTHS:
      aValue /= (1000.0*1000.0*60.0*60.0*24.0*30.0);
      break;
    case DURATION_UNIT_YEARS:
      aValue /= (1000.0*1000.0*60.0*60.0*24.0*365.0);
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

// convert from any of the exposed units back to the native unit
NS_IMETHODIMP
sbDurationPropertyUnitConverter::ConvertFromUnitToNative(PRFloat64 aValue,
                                                         PRUint32 aUnitID, 
                                                         PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case DURATION_UNIT_MICROSECONDS:
      // native unit, nothing to do
      break;
    case DURATION_UNIT_MILLISECONDS:
      aValue *= 1000.0;
      break;
    case DURATION_UNIT_SECONDS:
      aValue *= (1000.0*1000.0);
      break;
    case DURATION_UNIT_MINUTES:
      aValue *= (1000.0*1000.0*60.0);
      break;
    case DURATION_UNIT_HOURS:
      aValue *= (1000.0*1000.0*60.0*60.0);
      break;
    case DURATION_UNIT_DAYS:
      aValue *= (1000.0*1000.0*60*60.0*24.0);
      break;
    case DURATION_UNIT_WEEKS:
      aValue *= (1000.0*1000.0*60.0*60.0*24.0*7.0);
      break;
    case DURATION_UNIT_MONTHS:
      aValue *= (1000.0*1000.0*60.0*60.0*24.0*30.0);
      break;
    case DURATION_UNIT_YEARS:
      aValue *= (1000.0*1000.0*60.0*60.0*24.0*365.0);
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

