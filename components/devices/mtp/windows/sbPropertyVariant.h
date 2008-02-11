/* vim: set sw=2 :miv */
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

#include "nsIVariant.h"
#include "propidl.h"
#include <nsIClassInfo.h>

/**
 * This class wraps the Microsoft PROPVARIANT and provides
 * an nsIVariant interface to it.
 */
class sbPropertyVariant : public nsIWritableVariant,
                          public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIVARIANT
  NS_DECL_NSIWRITABLEVARIANT
  NS_DECL_NSICLASSINFO
  
  /* shared */ PROPVARIANT* GetPropVariant();
  static sbPropertyVariant * New();
  static sbPropertyVariant * New(PROPVARIANT const & propVar);
protected:
  /**
   * Initializes the PROPVARIANT
   */
  sbPropertyVariant()
  {
    PropVariantInit(&mPropVariant);
  }
  /**
   * Initializes the object with the given PROPVARIANT
   */
  sbPropertyVariant(PROPVARIANT const & propVar)
  {
    PropVariantCopy(&mPropVariant, &propVar);
  }
  /**
   * Cleans up the variant. Only allow destruction from within
   */
  virtual ~sbPropertyVariant();
private:
  PROPVARIANT mPropVariant;
  
  /**
   * Denotes whether the type is a value, by ref, or unsupported
   */
  typedef enum TypeType
  {
    ByVal,
    ByRef,
    Unsupported
  };
  /**
   * Helper function that asserts we can use the type identify by
   * dataType with this variant.
   */
  inline
  TypeType AssertSupportedType(PRUint16 dataType);
};

#define SB_PROPERTYVARIANT_CONTRACTID \
  "@songbirdnest.com/Songbird/PropertyVariant;1"
#define SB_PROPERTYVARIANT_CLASSNAME \
  "Songbird Property Variant"
#define SB_PROPERTYVARIANT_CID                          \
{ /* {e351eace-4ed2-4f19-8937-ed350a04a4a9} */          \
  0xe351eace, 0x4ed2, 0x4f19,                           \
  { 0x89, 0x37, 0xed, 0x35, 0x0a, 0x04, 0xa4, 0xa9 } }

#define SB_PROPERTYVARIANT_DESCRIPTION \
  "Songbird Property Variant Implementation"