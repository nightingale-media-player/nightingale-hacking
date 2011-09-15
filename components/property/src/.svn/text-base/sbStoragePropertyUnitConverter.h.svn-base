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

#ifndef __sbStoragePropertyUnitConverter_H__
#define __sbStoragePropertyUnitConverter_H__

#include "sbPropertyUnitConverter.h"

class sbStoragePropertyUnitConverter : public sbPropertyUnitConverter {
public:
  sbStoragePropertyUnitConverter();
  virtual ~sbStoragePropertyUnitConverter();

  enum {
    STORAGE_UNIT_BYTE,
    STORAGE_UNIT_KILOBYTE,
    STORAGE_UNIT_MEGABYTE,
    STORAGE_UNIT_GIGABYTE,
    STORAGE_UNIT_TERABYTE,
    STORAGE_UNIT_PETABYTE,
    STORAGE_UNIT_EXABYTE
  };

  virtual PRInt32 GetAutoUnit(PRFloat64 aValue);

protected:

  NS_IMETHOD ConvertFromNativeToUnit(PRFloat64 aValue,
                                     PRUint32 aUnitID,
                                     PRFloat64 &_retVal);
  NS_IMETHOD ConvertFromUnitToNative(PRFloat64 aValue,
                                     PRUint32 aUnitID,
                                     PRFloat64 &_retVal);
};

#endif /* __sbStoragePropertyUnitConverter_H__ */
