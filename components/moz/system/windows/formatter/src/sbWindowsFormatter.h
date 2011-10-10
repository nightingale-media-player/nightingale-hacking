/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#ifndef __SB_WINDOWS_FORMATTER_H__
#define __SB_WINDOWS_FORMATTER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale Windows formatter services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsFormatter.h
 * \brief Nightingale Windows Formatter Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale Windows formatter imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIWindowsFormatter.h"

// Windows imports.
#include <rpcsal.h>

#include <vds.h>


//------------------------------------------------------------------------------
//
// Nightingale Windows formatter classes.
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

  /**
   * Create a new Nightingale Windows formatter object and return it in
   * aWindowsFormatter.
   *
   * \param aWindowsFormatter  Returned created Windows formatter object.
   */
  static HRESULT New(sbWindowsFormatter** aWindowsFormatter);

  /**
   * Destroy an instance of a Nightingale Windows formatter object.
   */
  virtual ~sbWindowsFormatter();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mRefCount                  Object reference count.
  // mVdsService                VDS service object.
  //

  ULONG                         mRefCount;
  IVdsService*                  mVdsService;


  //
  // Private services.
  //

  /**
   * Create an instance of a Nightingale Windows formatter object.
   */
  sbWindowsFormatter();

  /**
   * Initialize the Nightingale Windows formatter object.
   */
  HRESULT Initialize();

  /**
   * Return in aVolume the VDS volume object for the volume with the volume name
   * specified by aVolumeName.
   *
   * \param aVolumeName         Name of volume to get.
   * \param aVolume             Returned VDS volume.
   *
   * \return S_OK               Volume was found.
   *         S_FALSE            Volume was not found.
   */
  HRESULT GetVolume(BSTR           aVolumeName,
                    IVdsVolumeMF** aVolume);

  /**
   * Search the volumes belonging to the VDS software provider specified by
   * aSwProvider for a volume with the name specified by aVolumeName.  Return
   * any found volume in aVolume.
   *
   * \param aSwProvider         VDS software provider to search.
   * \param aVolumeName         Name of volume to get.
   * \param aVolume             Returned VDS volume.
   *
   * \return S_OK               Volume was found.
   *         S_FALSE            Volume was not found.
   */
  HRESULT GetVolume(IVdsSwProvider* aSwProvider,
                    BSTR            aVolumeName,
                    IVdsVolumeMF**  aVolume);

  /**
   * Search the volumes belonging to the VDS pack specified by aPack for a
   * volume with the name specified by aVolumeName.  Return any found volume in
   * aVolume.
   *
   * \param aPack               VDS pack to search.
   * \param aVolumeName         Name of volume to get.
   * \param aVolume             Returned VDS volume.
   *
   * \return S_OK               Volume was found.
   *         S_FALSE            Volume was not found.
   */
  HRESULT GetVolume(IVdsPack*      aPack,
                    BSTR           aVolumeName,
                    IVdsVolumeMF** aVolume);
};


#endif // __SB_WINDOWS_FORMATTER_H__

