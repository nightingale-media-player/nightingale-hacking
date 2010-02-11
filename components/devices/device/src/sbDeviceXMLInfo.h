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

#ifndef _SB_DEVICE_XML_INFO_H_
#define _SB_DEVICE_XML_INFO_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird device XML info defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceXMLInfo.h
 * \brief Songbird Device XML Info Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird device XML info imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIDevice.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>

//------------------------------------------------------------------------------
//
// Songbird device XML info defs.
//
//------------------------------------------------------------------------------

// Device info XML namespace.
#define SB_DEVICE_INFO_NS "http://songbirdnest.com/deviceinfo/1.0"


//------------------------------------------------------------------------------
//
// Songbird device XML info classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the Songbird device XML info object.
 */

class sbDeviceXMLInfo
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public :

  //
  // Public services.
  //

  /**
   * Read the device info from the XML file with the URI spec specified by
   * aDeviceXMLInfoSpec.
   *
   * \param aDeviceXMLInfoSpec  Device XML info file spec.
   */
  nsresult Read(const char* aDeviceXMLInfoSpec);

  /**
   * Read the device info from the XML document specified
   * aDeviceXMLInfoDocument.
   *
   * \param aDeviceXMLInfoDocument  Device XML info document.
   */
  nsresult Read(nsIDOMDocument* aDeviceXMLInfoDocument);

  /**
   * If device info is present, return true in aDeviceInfoPresent; otherwise,
   * return false.
   *
   * \param aDeviceInfoPresent  Returned true if device info is present.
   */
  nsresult GetDeviceInfoPresent(PRBool* aDeviceInfoPresent);

  /**
   * Return in aDeviceInfoElement the root device info DOM element.  If no
   * device info is present, return null in aDeviceInfoElement.
   *
   * \param aDeviceInfoElement  Returned device info root DOM element.
   */
  nsresult GetDeviceInfoElement(nsIDOMElement** aDeviceInfoElement);

  /**
   * Return in aFolderURL the URL for the device folder of the type specified by
   * aFolderType.  If no folder of the specified type is present in the device
   * info, return a void URL.  The returned URL is relative to the device mount
   * path.
   *
   * \param aFolderType         Type of folder (e.g., "music", "video").
   * \param aFolderURL          Returned folder URL.
   */
  nsresult GetDeviceFolder(const nsAString& aFolderType,
                           nsAString&       aFolderURL);

  /**
   * Construct a Songbird device XML info object to be used for the device
   * specified by aDevice.
   *
   * \param aDevice             Device to use with XML info.  May be null.
   */
  sbDeviceXMLInfo(sbIDevice* aDevice = nsnull);

  /**
   * Destroy the Songbird device XML info object.
   */
  virtual ~sbDeviceXMLInfo();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private :

  //
  // mDevice                    Device to use with XML info.
  // mDeviceInfoElement         Root device info element.
  //

  sbIDevice*                    mDevice;
  nsCOMPtr<nsIDOMElement>       mDeviceInfoElement;


  /**
   * Check if the device matches the device info node specified by
   * aDeviceInfoNode.  If it matches, return true in aDeviceMatches; otherwise,
   * return false.
   *
   * \param aCapabilitiesNode   Capabilities DOM node to check.
   * \param aDeviceMatches      Returned true if device matches.
   */
  nsresult DeviceMatchesDeviceInfoNode(nsIDOMNode* aDeviceInfoNode,
                                       PRBool*     aDeviceMatches);

  /**
   * Check if the device with the properties specified by aDeviceProperties
   * matches the device node specified by aDeviceNode.  If it matchces, return
   * true in aDeviceMatches; otherwise, return false.
   *
   * \param aDeviceNode         Device DOM node to check.
   * \param aDeviceProperties   Device properties.
   * \param aDeviceMatches      Returned true if device matches.
   */
  nsresult DeviceMatchesDeviceNode(nsIDOMNode*      aDeviceNode,
                                   nsIPropertyBag2* aDeviceProperties,
                                   PRBool*          aDeviceMatches);

};


#endif // _SB_DEVICE_XML_INFO_H_

