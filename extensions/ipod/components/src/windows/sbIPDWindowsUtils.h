/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

#ifndef __SB_IPD_WINDOWS_UTILS_H__
#define __SB_IPD_WINDOWS_UTILS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device Windows utility services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDWindowsUtils.h
 * \brief Songbird iPod Device Windows Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device marshall imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDUtils.h"


//------------------------------------------------------------------------------
//
// iPod device Windows utility services classes.
//
//------------------------------------------------------------------------------

//
// Auto-disposal class wrappers.
//
//   sbAutoHANDLE               Wrapper to auto-close Windows handles.
//

SB_AUTO_CLASS(sbAutoHANDLE,
              HANDLE,
              mValue != INVALID_HANDLE_VALUE,
              CloseHandle(mValue),
              mValue = INVALID_HANDLE_VALUE);


#endif /* __SB_IPD_WINDOWS_UTILS_H__ */

