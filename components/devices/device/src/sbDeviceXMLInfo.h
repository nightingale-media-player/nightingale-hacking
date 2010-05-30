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
#include <nsIArray.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsStringAPI.h>
#include <nsTArray.h>


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
   * Return in aFolderURL the URL for the device folder of the content type
   * specified by aContentType.  If no folder of the specified type is present
   * in the device info, return a void URL.  The returned URL is relative to the
   * device mount path.
   *
   * \param aContentType        Folder content type from sbIDeviceCapabilities.
   * \param aFolderURL          Returned folder URL.
   */
  nsresult GetDeviceFolder(PRUint32   aContentType,
                           nsAString& aFolderURL);

  /**
   * Return in aMountTimeout the mount timeout value in seconds.  If no mount
   * timeout value is available, return NS_ERROR_NOT_AVAILABLE.
   *
   * \param aMountTimeout       Returned mount timeout value.
   *
   * \return NS_ERROR_NOT_AVAILABLE
   *                            Mount timeout value not available.
   */
  nsresult GetMountTimeout(PRUint32* aMountTimeout);

  /**
   * Return in aOutSupportsReformat if the device supports reformat. This value
   * will default to PR_TRUE if the format tag is not specified in the XML
   * document.
   *
   * \param aOutSupportsReformat Returned supported reformat boolean value.
   */
  nsresult GetDoesDeviceSupportReformat(PRBool *aOutSupportsReformat);

  /**
   * Return true in aOnlyMountMediaFolders if only the media folders should be
   * mounted rather than the entire storage volume.
   *
   * \param aOnlyMountMediaFolders
   *                            True if only the media folders should be
   *                            mounted.
   */
  nsresult GetOnlyMountMediaFolders(PRBool* aOnlyMountMediaFolders);

  /**
   * Returns a list of excluded folders folders in the array passed in. Each
   * array entry may be folder name or a path to a specific folder.
   * \param aFolders This is an array that the excluded folders will be added
   */
  nsresult GetExcludedFolders(nsAString & aExcludedFolders);

  /**
   * Return in aStorageDeviceInfoList a list of information for storage devices.
   * Each element in the list is an nsIPropertyBag of storage device properties.
   * See sbIDeviceInfoRegistrar.getStorageDeviceInfoList for storage device info
   * examples.
   *
   * \param aStorageDeviceInfoList  Returned storage device info list.
   */
  nsresult GetStorageDeviceInfoList(nsIArray** aStorageDeviceInfoList);

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

