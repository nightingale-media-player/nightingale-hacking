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
 * \file  sbWindowsFormatter.cpp
 * \brief Songbird Windows Formatter Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows formatter imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbWindowsFormatter.h"

// Local imports.
#include "sbIWindowsFormatter_i.c"
#include "sbWindowsEventLog.h"


//------------------------------------------------------------------------------
//
// Songbird Windows formatter IUnknown implementation.
//
//------------------------------------------------------------------------------

/**
 * Add a reference to this object and return the resulting reference count.
 *
 * \return                      New reference count.
 */

STDMETHODIMP_(ULONG)
sbWindowsFormatter::AddRef()
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
sbWindowsFormatter::Release()
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
sbWindowsFormatter::QueryInterface(REFIID aIID,
                                   void** aInterface)
{
  // Validate arguments.
  if (IsBadWritePtr(aInterface, sizeof(void*)))
    return E_POINTER;

  // Initialize interface to null.
  *aInterface = NULL;

  // Return requested interface.
  if (InlineIsEqualGUID(aIID, IID_IUnknown))
    *aInterface = static_cast<IUnknown*>(this);
  else if (InlineIsEqualGUID(aIID, IID_sbIWindowsFormatter))
    *aInterface = static_cast<sbIWindowsFormatter*>(this);
  else
    return E_NOINTERFACE;

  // Add a reference to the interface.
  (static_cast<IUnknown*>(*aInterface))->AddRef();

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows formatter sbIWindowsFormat implementation.
//
//------------------------------------------------------------------------------

/**
 *   Format the volume with the volume name specified by aVolumeName.  The
 * volume name is the volume GUID path without the trainling slash.
 *   The formatted file system type is specified by aType.  The volume label
 * is specified by aLabel.  The file system unit allocation size is specified
 * by aUnitAllocationSize; a value of 0 specifies that a default value should
 * be used.
 *   If aForce is true, the file system is formatted even if the volume is in
 * use.  If aQuickFormat is true, a quick format is performed.  If
 * aEnableCompression is true, compression is enabled for NTFS file systems.
 *
 * \param aVolumeName         Name of volume to format.
 * \param aType               Type of file system to use.
 * \param aLabel              Volume label.
 * \param aUnitAllocationSize File system unit allocation size.
 * \param aForce              If true, force format even if volume is in use.
 * \param aQuickFormat        If true, perform a quick format.
 * \param aEnableCompression  If true, enable compression for NTFS file
 *                            systems.
 */

STDMETHODIMP
sbWindowsFormatter::Format(BSTR          aVolumeName,
                           unsigned long aType,
                           BSTR          aLabel,
                           DWORD         aUnitAllocationSize,
                           long          aForce,
                           long          aQuickFormat,
                           long          aEnableCompression)
{
  // Log format request.
  sbWindowsEventLog("Formatting volume: %S\n", aVolumeName);

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Public Songbird Windows formatter services.
//
//------------------------------------------------------------------------------

/**
 * Create an instance of a Songbird Windows formatter object.
 */

sbWindowsFormatter::sbWindowsFormatter() :
  mRefCount(0)
{
}


/**
 * Destroy an instance of a Songbird Windows formatter object.
 */

sbWindowsFormatter::~sbWindowsFormatter()
{
}


