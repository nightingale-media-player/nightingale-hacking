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

#ifndef __SB_WINDOWS_FORMATTER_CLASS_FACTORY_H__
#define __SB_WINDOWS_FORMATTER_CLASS_FACTORY_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows formatter class factory services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsFormatterClassFactory.h
 * \brief Songbird Windows Formatter Class Factory Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows formatter class factory imported services.
//
//------------------------------------------------------------------------------

// Windows imports.
#include <objbase.h>


//------------------------------------------------------------------------------
//
// Songbird Windows formatter class factory classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements an IClassFactory interface for creating
 * sbWindowsFormatter instances.
 */

class sbWindowsFormatterClassFactory : public IClassFactory
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
  // IClassFactory implementation.
  //

  STDMETHOD(CreateInstance)(IUnknown* aOuter,
                            REFIID    aIID,
                            void**    aInstance);

  STDMETHOD(LockServer)(BOOL aLock);


  //
  // Public services.
  //

  sbWindowsFormatterClassFactory();

  virtual ~sbWindowsFormatterClassFactory();


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


#endif // __SB_WINDOWS_FORMATTER_CLASS_FACTORY_H__

