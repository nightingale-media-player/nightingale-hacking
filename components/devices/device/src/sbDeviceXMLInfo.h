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
 *   This class implements the Songbird device XML info object.  This class
 * reads in and processes a device XML info document or file.  It may optionally
 * be given a device object.  If it is provided a device object, it searches the
 * device XML info document for device info elements and device elements
 * matching the device object.
 *   Elements within the device XML info document provide information about a
 * device such as supported media formats, where media files are stored, and
 * whether a device has removable storage.
 *   A device XML info document can contain multiple device info elements.  Each
 * device info element can contain a list of device elements that specify what
 * devices match the device info element.  Only the information within the
 * matching device info element will be returned.
 *   Device information can also be specified within a device element.  If a
 * device object matches a device element that contains device information, only
 * that device information will be returned for the device.
 *   In the example below, the storage volume with LUN 0 of an HTC Incredible
 * will be marked as removable.  However, the storage volume with LUN 0 of an
 * HTC Magic will be marked as non-removable and primary.  However, the audio
 * formats returned will be the same for both devices.
 *
 * Example:

  <deviceinfo>
    <devices>
      <!-- HTC Incredible. -->
      <device usbVendorId="0x0bb4" usbProductId="0x0c9e">
        <storage lun="0" removable="true" />
        <storage lun="1" primary="true" />
      </device>

      <!-- HTC Magic. -->
      <device usbVendorId="0x0bb4" usbProductId="0x0c02" />
    </devices>

    <storage lun="0" primary="true" />
    <storage lun="1" removable="true" />

    <devicecaps xmlns="http://songbirdnest.com/devicecaps/1.0">
      <audio>
        <format mime="audio/mpeg" container="audio/mpeg" codec="audio/mpeg">
        ...
        </format>
      </audio>
    </devicecaps>
  </deviceinfo>

  <deviceinfo>
    <devices>
      <!-- Nokia N85 MSC. -->
      <device usbVendorId="0x0421" usbProductId="0x0091" />
    </devices>

    <devicecaps xmlns="http://songbirdnest.com/devicecaps/1.0">
      <audio>
        <format mime="audio/x-ms-wma"
                container="video/x-ms-asf"
                codec="audio/x-ms-wma">
        ...
        </format>
      </audio>
    </devicecaps>
  </deviceinfo>

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
  // mDeviceElement             Device element matching device.  May be null if
  //                            device does not match a specific device element.
  //

  sbIDevice*                    mDevice;
  nsCOMPtr<nsIDOMElement>       mDeviceInfoElement;
  nsCOMPtr<nsIDOMElement>       mDeviceElement;


  /**
   * Check if the device matches the device info node specified by
   * aDeviceInfoNode.  If it matches, return true in aDeviceMatches; otherwise,
   * return false.  If the device matches a <device> node, return the matching
   * device node in aDeviceNode if aDeviceNode is not null.
   *
   * \param aDeviceInfoNode     Device info node to check.
   * \param aDeviceMatches      Returned true if device matches.
   * \param aDeviceNode         Optional returned matching device node.
   */
  nsresult DeviceMatchesDeviceInfoNode(nsIDOMNode*  aDeviceInfoNode,
                                       PRBool*      aDeviceMatches,
                                       nsIDOMNode** aDeviceNode = nsnull);

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

  /**
   * Return in aNodeList the list of all device info nodes with the name space
   * and tag name specified by aNameSpace and aTagName.  If any nodes are found
   * within the matching <device> node, return only those nodes.  Otherwise,
   * return only nodes that descend from the matching <deviceinfo> container
   * node but do not descend from any <device> node.
   *
   * See class description above.
   *
   * \param aNameSpace          Requested device info node name space.
   * \param aTagName            Requested device info node tag name.
   * \param aNodeList           Returned node list.
   */
  nsresult GetDeviceInfoNodes(const nsAString&                  aNameSpace,
                              const nsAString&                  aTagName,
                              nsTArray< nsCOMPtr<nsIDOMNode> >& aNodeList);

  /**
   * Return in aNodeList the list of all device info nodes with the device info
   * name space and tag name specified by aTagName.  If any nodes are found
   * within the matching <device> node, return only those nodes.  Otherwise,
   * return only nodes that descend from the matching <deviceinfo> container
   * node but do not descend from any <device> node.
   *
   * See class description above.
   *
   * \param aTagName            Requested device info node tag name.
   * \param aNodeList           Returned node list.
   */
  nsresult GetDeviceInfoNodes(const nsAString&                  aTagName,
                              nsTArray< nsCOMPtr<nsIDOMNode> >& aNodeList);

  /**
   * Return true in aIsDeviceNodeDescendant if the node specified by aNode is a
   * descendant of a <device> node.
   *
   * See class description above.
   *
   * \param aNode               Node to check.
   * \param aIsDeviceNodeDescendant
   *                            Returned true if node is a descendant of a
   *                            device node.
   */
  nsresult IsDeviceNodeDescendant(nsIDOMNode* aNode,
                                  PRBool*     aIsDeviceNodeDescendant);
};


#endif // _SB_DEVICE_XML_INFO_H_

