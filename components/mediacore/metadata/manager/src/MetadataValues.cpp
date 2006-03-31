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
  return NS_OK;
}

/* wstring getValue (in wstring key); */
NS_IMETHODIMP sbMetadataValues::GetValue(const PRUnichar *key, PRUnichar **_retval)
{
  return NS_OK;
}

/* PRInt32 getType (in wstring key); */
NS_IMETHODIMP sbMetadataValues::GetType(const PRUnichar *key, PRInt32 *_retval)
{
  return NS_OK;
}

