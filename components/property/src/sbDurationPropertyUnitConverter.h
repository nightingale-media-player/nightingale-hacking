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

#ifndef __sbDurationPropertyUnitConverter_H__
#define __sbDurationPropertyUnitConverter_H__

#include "sbPropertyUnitConverter.h"

class sbDurationPropertyUnitConverter : public sbPropertyUnitConverter {
public:
  sbDurationPropertyUnitConverter();
  virtual ~sbDurationPropertyUnitConverter();

  enum {
    DURATION_UNIT_MICROSECONDS,
    DURATION_UNIT_MILLISECONDS,
    DURATION_UNIT_SECONDS,
    DURATION_UNIT_MINUTES,
    DURATION_UNIT_HOURS,
    DURATION_UNIT_DAYS,
    DURATION_UNIT_WEEKS,
    DURATION_UNIT_MONTHS,
    DURATION_UNIT_YEARS,
  };

protected:

  NS_IMETHOD ConvertFromNativeToUnit(PRFloat64 aValue, 
                                     PRUint32 aUnitID, 
                                     PRFloat64 &_retVal);
  NS_IMETHOD ConvertFromUnitToNative(PRFloat64 aValue, 
                                     PRUint32 aUnitID, 
                                     PRFloat64 &_retVal);
};

#endif // __sbDurationPropertyUnitConverter_H__
