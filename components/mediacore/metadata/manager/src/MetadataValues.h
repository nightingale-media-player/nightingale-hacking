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

/**
* \file MetadataValues.h
* \brief 
*/

#ifndef __METADATA_VALUES_H__
#define __METADATA_VALUES_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <nsStringGlue.h>
#include "sbIMetadataValues.h"
#include <map>

// DEFINES ====================================================================
#define SONGBIRD_METADATAVALUES_CONTRACTID                \
  "@songbirdnest.com/Songbird/MetadataValues;1"
#define SONGBIRD_METADATAVALUES_CLASSNAME                 \
  "Songbird Metadata Values Container"
#define SONGBIRD_METADATAVALUES_CID                       \
{ /* 105e3af3-eef3-4b1b-900e-ccc1a9259ceb */              \
  0x105e3af3,                                             \
  0xeef3,                                                 \
  0x4b1b,                                                 \
  {0x90, 0xe, 0xcc, 0xc1, 0xa9, 0x25, 0x9c, 0xeb}         \
}
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

#endif // __METADATA_VALUES_H__

