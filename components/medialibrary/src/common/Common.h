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
 * \file Common.h
 * \brief Common things to include for various platforms.
 */

#if !defined(___COMMON__H__)
#define ___COMMON__H__

// DEFINES ====================================================================
#define SAFE_DELETE(x) if(x != NULL) { delete x; x = NULL; }
#define SAFE_DELETE_ARRAY(x) if(x != NULL) { delete [] x; x = NULL; }


// INCLUDES ===================================================================
#include <string>

#include "nscore.h"

//*****************************************************************************
// PLATFORM : WIN32
//*****************************************************************************
#if defined(_WIN32)

#include <windows.h>

#else
//*****************************************************************************
// PLATFORM : ???
//*****************************************************************************
#error "PORT ME"
#endif

// TYPEDEFS ===================================================================
namespace std
{
  typedef basic_string< PRUnichar > prustring;
};



#endif //___COMMON__H__