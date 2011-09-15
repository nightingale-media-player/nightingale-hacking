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

// Property Unit Converter for information storage quantities,
// ie: byte, kB, MB, GB

#include "sbStoragePropertyUnitConverter.h"
#include <math.h>
#include <stdlib.h>

// ctor - register all units
sbStoragePropertyUnitConverter::sbStoragePropertyUnitConverter()
{
  SetStringBundle(NS_LITERAL_STRING("chrome://songbird/locale/songbird.properties"));
  RegisterUnit(STORAGE_UNIT_BYTE,
    NS_LITERAL_STRING("b"),
    NS_LITERAL_STRING("&storage.unit.bytes"),
    NS_LITERAL_STRING("&storage.unit.bytes.short"),
    PR_TRUE);
  RegisterUnit(STORAGE_UNIT_KILOBYTE,
    NS_LITERAL_STRING("kb"),
    NS_LITERAL_STRING("&storage.unit.kilobytes"),
    NS_LITERAL_STRING("&storage.unit.kilobytes.short"));
  RegisterUnit(STORAGE_UNIT_MEGABYTE,
    NS_LITERAL_STRING("mb"),
    NS_LITERAL_STRING("&storage.unit.megabytes"),
    NS_LITERAL_STRING("&storage.unit.megabytes.short"));
  RegisterUnit(STORAGE_UNIT_GIGABYTE,
    NS_LITERAL_STRING("gb"),
    NS_LITERAL_STRING("&storage.unit.gigabytes"),
    NS_LITERAL_STRING("&storage.unit.gigabytes.short"));
  RegisterUnit(STORAGE_UNIT_TERABYTE,
    NS_LITERAL_STRING("tb"),
    NS_LITERAL_STRING("&storage.unit.terabytes"),
    NS_LITERAL_STRING("&storage.unit.terabytes.short"));
  RegisterUnit(STORAGE_UNIT_PETABYTE,
    NS_LITERAL_STRING("pb"),
    NS_LITERAL_STRING("&storage.unit.petabytes"),
    NS_LITERAL_STRING("&storage.unit.petabytes.short"));
  RegisterUnit(STORAGE_UNIT_EXABYTE,
    NS_LITERAL_STRING("eb"),
    NS_LITERAL_STRING("&storage.unit.exabytes"),
    NS_LITERAL_STRING("&storage.unit.exabytes.short"));
}

// dtor
sbStoragePropertyUnitConverter::~sbStoragePropertyUnitConverter()
{
}

// convert from the native unit to any of the exposed units
NS_IMETHODIMP
sbStoragePropertyUnitConverter::ConvertFromNativeToUnit(PRFloat64 aValue,
                                                        PRUint32 aUnitID,
                                                        PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case STORAGE_UNIT_BYTE:
      // native unit, nothing to do
      break;
    case STORAGE_UNIT_KILOBYTE:
      aValue /= 1024.0;
      break;
    case STORAGE_UNIT_MEGABYTE:
      aValue /= (1024.0*1024.0);
      break;
    case STORAGE_UNIT_GIGABYTE:
      aValue /= (1024.0*1024.0*1024.0);
      break;
    case STORAGE_UNIT_TERABYTE:
      aValue /= (1024.0*1024.0*1024.0*1024.0);
      break;
    case STORAGE_UNIT_PETABYTE:
      aValue /= (1024.0*1024.0*1024.0*1024.0*1024.0);
      break;
    case STORAGE_UNIT_EXABYTE:
      aValue /= (1024.0*1024.0*1024.0*1024.0*1024.0*1024.0);
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

// convert from any of the exposed units back to the native unit
NS_IMETHODIMP
sbStoragePropertyUnitConverter::ConvertFromUnitToNative(PRFloat64 aValue,
                                                        PRUint32 aUnitID,
                                                        PRFloat64 &_retVal)
{
  switch (aUnitID) {
    case STORAGE_UNIT_BYTE:
      // native unit, nothing to do
      break;
    case STORAGE_UNIT_KILOBYTE:
      aValue *= 1024.0;
      break;
    case STORAGE_UNIT_MEGABYTE:
      aValue *= (1024.0*1024.0);
      break;
    case STORAGE_UNIT_GIGABYTE:
      aValue *= (1024.0*1024.0*1024.0);
      break;
    case STORAGE_UNIT_TERABYTE:
      aValue *= (1024.0*1024.0*1024.0*1024.0);
      break;
    case STORAGE_UNIT_PETABYTE:
      aValue *= (1024.0*1024.0*1024.0*1024.0*1024.0);
      break;
    case STORAGE_UNIT_EXABYTE:
      aValue *= (1024.0*1024.0*1024.0*1024.0*1024.0*1024.0);
      break;
    default:
      return NS_ERROR_INVALID_ARG;
  }
  _retVal = aValue;
  return NS_OK;
}

PRInt32 sbStoragePropertyUnitConverter::GetAutoUnit(const PRFloat64 aValue) {
  // get number of digits
  PRUint32 d;
  if (aValue == 0) d = 1;
  else d = (PRUint32)(log10(fabs(aValue)) + 1);

  // and pick most suitable unit
  if (d < 4) return STORAGE_UNIT_BYTE;
  if (d < 7) return STORAGE_UNIT_KILOBYTE;
  if (d < 10) return STORAGE_UNIT_MEGABYTE;
  if (d < 13) return STORAGE_UNIT_GIGABYTE;
  if (d < 16) return STORAGE_UNIT_TERABYTE;
  if (d < 19) return STORAGE_UNIT_PETABYTE;
  return STORAGE_UNIT_EXABYTE;
}
