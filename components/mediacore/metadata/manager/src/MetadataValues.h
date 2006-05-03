/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/**
* \file MetadataValues.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include <string/nsString.h>
#include "sbIMetadataValues.h"
#include <map>

// DEFINES ====================================================================
#define SONGBIRD_METADATAVALUES_CONTRACTID  "@songbird.org/Songbird/MetadataValues;1"
#define SONGBIRD_METADATAVALUES_CLASSNAME   "Songbird Metadata Values Container"

// {EC39DE03-8918-4369-951E-0A04BD9CF663}
#define SONGBIRD_METADATAVALUES_CID { 0xec39de03, 0x8918, 0x4369, { 0x95, 0x1e, 0xa, 0x4, 0xbd, 0x9c, 0xf6, 0x63 } }

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbMetadataValues : public sbIMetadataValues
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAVALUES

  sbMetadataValues();
  virtual ~sbMetadataValues();

  struct sbMetadataValue
  {
    sbMetadataValue( void ) : m_Value( ), m_Type( 0 ) {}
    sbMetadataValue( nsString value, PRInt32 type = 0 ) : m_Value( value ), m_Type( type ) {}
    nsString m_Value;
    PRInt32 m_Type;
  };

  class t_map : public std::map< nsString, sbMetadataValue > {};
  t_map m_Map;
};
