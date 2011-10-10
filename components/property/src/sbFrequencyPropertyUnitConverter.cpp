/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

// Property Unit Converter for samplerate values, ie: Hz, kHz

#include "sbFrequencyPropertyUnitConverter.h"
#include <math.h>
#include <stdlib.h>

// ctor - register all units
sbFrequencyPropertyUnitConverter::sbFrequencyPropertyUnitConverter() 
{
  SetStringBundle(NS_LITERAL_STRING("chrome://nightingale/locale/nightingale.properties"));
  RegisterUnit(FREQUENCY_UNIT_HZ,
    NS_LITERAL_STRING("hz"),
    NS_LITERAL_STRING("&frequency.unit.hertz"), 
    NS_LITERAL_STRING("&frequency.unit.hertz.short"), 
    PR_TRUE);
  RegisterUnit(FREQUENCY_UNIT_KHZ,
    NS_LITERAL_STRING("khz"),
    NS_LITERAL_STRING("&frequency.unit.kilohertz"),
    NS_LITERAL_STRING("&frequency.unit.kilohertz.short"));
}

// dtor
sbFrequencyPropertyUnitConverter::~sbFrequencyPropertyUnitConverter() 
{
}

// convert from the native unit to any of the exposed units
NS_IMETHODIMP
sbFrequencyPropertyUnitConverter::ConvertFromNativeToUnit(PRFloat64 aValue, 
                                                          PRUint32 aUnitID, 
                                                          PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case FREQUENCY_UNIT_HZ:
      // native unit, nothing to do
      break;
    case FREQUENCY_UNIT_KHZ:
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
sbFrequencyPropertyUnitConverter::ConvertFromUnitToNative(PRFloat64 aValue,
                                                          PRUint32 aUnitID, 
                                                          PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case FREQUENCY_UNIT_HZ:
      // native unit, nothing to do
      break;
    case FREQUENCY_UNIT_KHZ:
      aValue *= 1000.0;
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

PRInt32 sbFrequencyPropertyUnitConverter::GetAutoUnit(const PRFloat64 aValue) {
  // get number of digits
  PRUint32 d;
  if (aValue == 0) d = 1;
  else d = (PRUint32)(log10(abs(aValue)) + 1);

  // and pick most suitable unit
  if (d < 4) return FREQUENCY_UNIT_HZ;
  return FREQUENCY_UNIT_KHZ;
}
