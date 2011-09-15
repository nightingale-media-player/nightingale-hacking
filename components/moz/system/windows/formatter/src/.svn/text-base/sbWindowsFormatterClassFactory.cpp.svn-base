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

/**
 * \file  sbWindowsFormatterClassFactory.cpp
 * \brief Songbird Windows Formatter Class Factory Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows formatter class factory imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbWindowsFormatterClassFactory.h"

// Local imports.
#include "sbWindowsFormatter.h"
#include "sbWindowsFormatterServer.h"

// Songbird imports.
#include <sbWindowsUtils.h>


//------------------------------------------------------------------------------
//
// Songbird Windows formatter class factory IUnknown implementation.
//
//------------------------------------------------------------------------------

/**
 * Add a reference to this object and return the resulting reference count.
 *
 * \return                      New reference count.
 */

STDMETHODIMP_(ULONG)
sbWindowsFormatterClassFactory::AddRef()
{
  // Increment the reference count and return the result.
  return ++mRefCount;
}


/**
 * Remove a reference to this object and return the resulting reference count.
 * Delete this object if the reference count reaches 0.
 *
 * \return                      New reference count.
 */

STDMETHODIMP_(ULONG)
sbWindowsFormatterClassFactory::Release()
{
  // Decrement the reference count.
  ULONG refCount = --mRefCount;

  // Delete self if reference count reaches zero.
  if (mRefCount == 0)
    delete this;

  return refCount;
}


/**
 * Return in aInterface an implementation of this object with the interface
 * specified by aIID.  If this object does not support the specified interface,
 * return E_NOINTERFACE.
 *
 * \param aIID                  Requested interface.
 * \param aInterface            Returned interface object.
 *
 * \return E_NOINTERFACE        Requested interface is not supported.
 */

STDMETHODIMP
sbWindowsFormatterClassFactory::QueryInterface(REFIID aIID,
                                               void** aInterface)
{
  // Validate return arguments.
  if (IsBadWritePtr(aInterface, sizeof(void*)))
    return E_POINTER;

  // Initialize interface to null.
  *aInterface = NULL;

  // Return requested interface.
  if (InlineIsEqualGUID(aIID, IID_IUnknown))
    *aInterface = static_cast<IUnknown*>(this);
  else if (InlineIsEqualGUID(aIID, IID_IClassFactory))
    *aInterface = static_cast<IClassFactory*>(this);
  else
    return E_NOINTERFACE;

  // Add a reference to the interface.
  (static_cast<IUnknown*>(*aInterface))->AddRef();

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows formatter class factory IClassFactory implementation.
//
//------------------------------------------------------------------------------

/**
 * Create and return in aInstance an object instance providing the interface
 * specified by aIID.  Use the object specified by aOuter with aggregation.
 * Return CLASS_E_NOAGGREGATION if aggregation is not supported.
 *
 * \param aOuter                Aggregation outer object.
 * \param aIID                  Requested interface.
 * \param aInstance             Returned created instance.
 *
 * \return CLASS_E_NOAGGREGATION Aggregation is not supported.
 */

STDMETHODIMP
sbWindowsFormatterClassFactory::CreateInstance(IUnknown* aOuter,
                                               REFIID    aIID,
                                               void**    aInstance)
{
  HRESULT result;

  // Aggregation is not supported.
  if (aOuter)
    return CLASS_E_NOAGGREGATION;

  // Validate return arguments.
  if (IsBadWritePtr(aInstance, sizeof(void*)))
    return E_POINTER;

  // Set result to NULL.
  *aInstance = NULL;

  // Create a new addref'ed Songbird Windows format object and set it up for
  // auto-release.
  sbWindowsFormatter* windowsFormatter;
  result = sbWindowsFormatter::New(&windowsFormatter);
  SB_WIN_ENSURE_SUCCESS(result, result);
  sbAutoIUnknown autoWindowsFormatter(windowsFormatter);

  // Get the requested interface.
  result = windowsFormatter->QueryInterface(aIID, aInstance);
  if (FAILED(result))
    return result;

  return S_OK;
}


/**
 * Lock or unlock the object server as specified by aLock.  If aLock is true,
 * add a server lock; otherwise, remove a server lock.
 *
 * \param aLock                 If true, add lock; otherwise, remove lock.
 */

STDMETHODIMP
sbWindowsFormatterClassFactory::LockServer(BOOL aLock)
{
  // Lock or unlock the COM server.
  if (aLock)
    COMServerLock();
  else
    COMServerUnlock();

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Public Songbird Windows formatter class factory services.
//
//------------------------------------------------------------------------------

/**
 * Create an instance of a Songbird Windows formatter class factory.
 */

sbWindowsFormatterClassFactory::sbWindowsFormatterClassFactory() :
  mRefCount(0)
{
  // Add a COM server lock.
  COMServerLock();
}


/**
 * Destroy an instance of a Songbird Windows formatter class factory.
 */

sbWindowsFormatterClassFactory::~sbWindowsFormatterClassFactory()
{
  // Remove COM server lock.
  COMServerUnlock();
}

