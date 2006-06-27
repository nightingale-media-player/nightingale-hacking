/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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

#pragma warning(push)
#pragma warning(disable:4800)

// INCLUDES ===================================================================
#include <nscore.h>
#include "MetadataValues.h"

#include <string/nsStringAPI.h>

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataValues, sbIMetadataValues)

//-----------------------------------------------------------------------------
sbMetadataValues::sbMetadataValues()
{
}

//-----------------------------------------------------------------------------
sbMetadataValues::~sbMetadataValues()
{
}

/* void setValue (in wstring key, in wstring value, in PRInt32 type); */
NS_IMETHODIMP sbMetadataValues::SetValue(const PRUnichar *key, const PRUnichar *value, PRInt32 type)
{
  // Check the inputs
  if ( ! ( key ) )
  {
    return NS_OK;
  }
  if ( ! ( value ) )
  {
    value = NS_LITERAL_STRING("").get(); // This should work for our purposes.
  }
  // Put it into the map
  m_Map[ nsString( key ) ] = sbMetadataValue( nsString( value ), type );
  return NS_OK;
}

/* wstring getValue (in wstring key); */
NS_IMETHODIMP sbMetadataValues::GetValue(const PRUnichar *key, PRUnichar **_retval)
{
  // Bad key value is "".
  nsString retval( NS_LITERAL_STRING("") ); // Formally a blank string.  We'd better get it back in the get.
  // Check the inputs
  if ( ! ( key ) || ! ( _retval ) )
  {
    return NS_OK;
  }
  // Pull it out of the map
  t_map::iterator it = m_Map.find( nsString( key ) );
  if ( it != m_Map.end() )
  {
    retval = (*it).second.m_Value;
  }
  size_t nLen = retval.Length() + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(retval.get(), nLen * sizeof(PRUnichar));
  return NS_OK;
}

/* PRInt32 getType (in wstring key); */
NS_IMETHODIMP sbMetadataValues::GetType(const PRUnichar *key, PRInt32 *_retval)
{
  // Bad key value
  *_retval = -1; 
  // Check the inputs
  if ( ! ( key ) || ! ( _retval ) )
  {
    return NS_OK;
  }
  // Pull it out of the map
  t_map::iterator it = m_Map.find( nsString( key ) );
  if ( it != m_Map.end() )
  {
    *_retval = (*it).second.m_Type;
  }
  return NS_OK;
}

/* PRInt32 Clear (); */
NS_IMETHODIMP sbMetadataValues::Clear()
{
  m_Map.clear();
  return NS_OK;
}

#pragma warning(pop)
