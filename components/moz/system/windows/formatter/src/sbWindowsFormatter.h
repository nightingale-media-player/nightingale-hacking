/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SB_WINDOWS_FORMATTER_H__
#define __SB_WINDOWS_FORMATTER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows formatter services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsFormatter.h
 * \brief Songbird Windows Formatter Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows formatter imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIWindowsFormatter.h"

// Songbird imports.
#include <sbMemoryUtils.h>


//------------------------------------------------------------------------------
//
// Songbird Windows formatter classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements an sbIWindowsFormatter interface for formatting
 * volumes.
 */

class sbWindowsFormatter : public sbIWindowsFormatter
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // IUnknown implementation.
  //

  STDMETHOD_(ULONG, AddRef)();

  STDMETHOD_(ULONG, Release)();

  STDMETHOD(QueryInterface)(REFIID aIID,
                            void** aInterface);


  //
  // sbIWindowsFormatter
  //

  STDMETHOD(Format)(BSTR                 aVolumeName,
                    unsigned long        aType,
                    BSTR                 aLabel,
                    DWORD                aUnitAllocationSize,
                    long                 aForce,
                    long                 aQuickFormat,
                    long                 aEnableCompression);


  //
  // Public services.
  //

  sbWindowsFormatter();

  virtual ~sbWindowsFormatter();


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  //
  // mRefCount                  Object reference count.
  //

  ULONG                         mRefCount;
};


//
// Auto-disposal class wrappers.
//
//   sbAutoRegKey               Wrapper to automatically close a registry key.
//   sbAutoIUnknown             Wrapper to automatically release an IUnknown
//                              object.
//

SB_AUTO_NULL_CLASS(sbAutoRegKey, HKEY, RegCloseKey(mValue));
SB_AUTO_NULL_CLASS(sbAutoIUnknown, IUnknown*, mValue->Release());


#endif // __SB_WINDOWS_FORMATTER_H__

