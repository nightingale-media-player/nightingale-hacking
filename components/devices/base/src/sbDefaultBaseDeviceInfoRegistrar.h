/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef SBIDEFAULTBASEDEVICEINFOREGISTRAR_H_
#define SBIDEFAULTBASEDEVICEINFOREGISTRAR_H_

// Mozilla includes
#include <nsAutoPtr.h>
#include <nsCOMArray.h>

// Songibrd includes
#include <sbDeviceXMLInfo.h>
#include <sbIDeviceInfoRegistrar.h>

class sbIDevice;

/**
 *   This is a base class for implementing a default info registrar for a
 * device.
 */
class sbDefaultBaseDeviceInfoRegistrar:
        public sbIDeviceInfoRegistrar
{
public:
  NS_DECL_SBIDEVICEINFOREGISTRAR

  sbDefaultBaseDeviceInfoRegistrar();

protected:
  //
  // mDevice                    Device info device.
  // mDeviceXMLInfo             Device XML info data record.
  // mDeviceXMLInfoPresent      If true, device info is present in the device
  //                            XML info data record.
  //

  sbIDevice*                 mDevice;
  nsAutoPtr<sbDeviceXMLInfo> mDeviceXMLInfo;
  PRBool                     mDeviceXMLInfoPresent;

  /**
   * Return in aDeviceXMLInfo the device XML info for the device specified by
   * aDevice.  If no device XML info is available, return null in
   * aDeviceXMLInfo.
   *
   * \param aDevice             Device for which to get info.
   * \param aDeviceXMLInfo      Returned device XML info.
   */
  nsresult GetDeviceXMLInfo(sbIDevice*        aDevice,
                            sbDeviceXMLInfo** aDeviceXMLInfo);

  /**
   * Read the device XML info document with the URI spec specified by
   * aDeviceXMLInfoSpec for the device specified by aDevice.
   *
   * \param aDeviceXMLInfoSpec  Device XML info document URI spec.
   * \param aDevice             Device for which to get info.
   */
  nsresult GetDeviceXMLInfo(const nsACString& aDeviceXMLInfoSpec,
                            sbIDevice*        aDevice);

  /**
   * Return in aDeviceXMLInfoSpec the URI spec for the device XML info document.
   *
   * \param aDeviceXMLInfoSpec  Device XML info document URI spec.
   */
  virtual nsresult GetDeviceXMLInfoSpec(nsACString& aDeviceXMLInfoSpec);

  /**
   * Return in aDeviceXMLInfoExtensions a space-delimited list of file
   * extensions (without dots) to scan for when searching a directory
   * for device XML info files.  Directories are not scanned if this
   * string is empty
   *
   * \param aDeviceXMLInfoExtensions  Returns a space-delimited list of
   *                                  extensions
   */
  virtual nsresult GetDeviceXMLInfoExtensions(
                     nsACString& aDeviceXMLInfoExtensions);

  /**
   * Return in aDeviceXMLInfoSpec the URI spec for the default device XML info
   * document.  If no matching device info is found in the main device XML info
   * document, the device info in the default document is used.  Typically, the
   * default device XML info document matches all devices.
   *
   * \param aDeviceXMLInfoSpec  Default device XML info document URI spec.
   */
  virtual nsresult GetDefaultDeviceXMLInfoSpec(nsACString& aDeviceXMLInfoSpec);

  virtual ~sbDefaultBaseDeviceInfoRegistrar();
};

#endif /* SBIDEFAULTBASEDEVICEINFOREGISTRAR_H_ */

