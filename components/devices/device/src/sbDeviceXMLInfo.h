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
   * Search one or more XML files specified by the space-delimited list
   * of URI strings and load the newest device info element--according
   * to its version attribute--that matches this device.
   *
   * \param aDeviceXMLInfoSpecList  A space-delimited list of URI strings
   *                                pointing to device XML info files or
   *                                directories containing such files to be
   *                                searched recursively
   *
   * \param aExtensionsList         A space-delimited list of file extensions
   *                                (without dots) to scan for when searching
   *                                a directory for device XML info files.
   *                                If nsnull, then directories are not
   *                                searched
   */
  nsresult Read(const char* aDeviceXMLInfoSpecList,
                const char* aExtensionsList = nsnull);

  /**
   * Search the specified XML file or, if the URI points to a directory,
   * all files in that directory recursively that have one of the specified
   * file name extensions and load the newest device info element--according
   * to its version attribute--that matches this device.
   *
   * \param aDeviceXMLInfoURI       A URI pointing either to a device XML
   *                                info file or to a directory to be
   *                                searched recursively for such files
   *
   * \param aExtensionsList         A space-delimited list of file extensions
   *                                (without dots) to scan for when searching
   *                                a directory for device XML info files
   */
  nsresult Read(nsIURI *           aDeviceXMLInfoURI,
                const nsAString &  aExtensionsList);

  /**
   * Search the specified XML file or, if the nsIFile is a directory,
   * all files in that directory recursively that have one of the specified
   * file name extensions and load the newest device info element--according
   * to its version attribute--that matches this device.
   *
   * \param aDeviceXMLInfoFile      Either a device XML info file or a
   *                                directory to be searched recursively
   *                                for such files
   *
   * \param aExtensionsList         A space-delimited list of file extensions
   *                                (without dots) to scan for when searching
   *                                a directory for device XML info files
   */
  nsresult Read(nsIFile *          aDeviceXMLInfoFile,
                const nsAString &  aExtensionsList);

  /**
   * Read the device info from the XML stream specified by
   * aDeviceXMLInfoStream.
   *
   * \param aDeviceXMLInfoStream    Device XML info stream.
   */
  nsresult Read(nsIInputStream *    aDeviceXMLInfoStream);

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
   * Return in aDefaultName the default name for the device. If no name is
   * present in the device info, return a void string.
   *
   * \param aDefaultName       Returned default name value.
   */
  nsresult GetDefaultName(nsAString& aDefaultName);

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
   * Return in aImportRules a list of "rules" to apply to files imported from
   * the device.  Each rule is itself an nsIArray of two nsISupportsStrings:
   * a path and an "import type" of files residing (recursively) within that
   * path.  The import type can determine how to set the media item properties
   * when importing items of that type from the device.
   *
   * \param aImportRules  an array to populate with the list of import rules.
   *                      This array can be stored with an sbIDevice as
   *                      SB_DEVICE_PROPERTY_IMPORT_RULES.
   */
  nsresult GetImportRules(nsIArray ** aImportRules);

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
   * Return in aDeviceIconURL the URL for the device icon file.  The returned
   * URL is relative to the device mount path.
   *
   * \param aDeviceIconURL      Returned device icon file URL.
   */
  nsresult GetDeviceIcon(nsAString& aDeviceIconURL);

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

  /**
   * Returns the device identifier for logging
   */
  static nsCString GetDeviceIdentifier(sbIDevice * aDevice);

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
  nsString                      mDeviceInfoVersion;
  nsCOMPtr<nsIDOMElement>       mDeviceInfoElement;
  nsCOMPtr<nsIDOMElement>       mDeviceElement;
  bool                          mLogDeviceInfo;


  /**
   * Check if the device matches the device info node specified by
   * aDeviceInfoNode.  If it matches, return true in aDeviceMatches; otherwise,
   * return false.  If the device matches a <device> node, return the matching
   * device node in aDeviceNode if aDeviceNode is not null.
   *
   * \param aDeviceInfoNode     Device info node to check.
   * \param aFoundVersion       Returns the version of the device info node
   *                            if the device matches, or an empty string
   *                            otherwise.  The version is the value of the
   *                            version attribute of the <deviceinfo> element,
   *                            if defined, or of its parent element, if
   *                            defined, or "0" otherwise.
   * \param aDeviceNode         Optional returned matching device node.
   */
  nsresult DeviceMatchesDeviceInfoNode(nsIDOMNode*  aDeviceInfoNode,
                                       nsAString &  aFoundVersion,
                                       nsIDOMNode** aDeviceNode);

  /**
   * Return the version of a <deviceinfo> element
   *
   * \param aDeviceInfoElement  A <deviceinfo> element
   * \param aVersion            Returns the value of the version attribute
   *                            of aDeviceInfoElement, if defined, or of its
   *                            parent element, if defined, or "0" otherwise.
   */
  nsresult GetDeviceInfoVersion(nsIDOMElement * aDeviceInfoElement,
                                nsAString &     aVersion);

  /**
   * Check if the device with the properties specified by aDeviceProperties
   * matches the device node specified by aDeviceNode.  If it matches, return
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

  /**
   * Logging function for device info. Controlled by mLogDeviceInfo. Inline to
   * avoid unnecessary function calls. Takes a variable number of arguments.
   * \param aFmt a standard C printf style format string
   */
  inline
  void Log(const char * aFmt, ...);

  /**
   * Logging function that does the actual work, called by Log
   */
  void LogArgs(const char * aFmt, va_list aArgs);
};


#endif // _SB_DEVICE_XML_INFO_H_

