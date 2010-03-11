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

#ifndef SBDEVICEXMLCAPABILITIES_H_
#define SBDEVICEXMLCAPABILITIES_H_

#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <sbIDevice.h>

class sbIDeviceCapabilities;
class sbIDevCapVideoStream;
class sbIDevCapAudioStream;
class sbIDevCapVideoFormatType;
class nsIDOMDocument;
class nsIDOMElement;
class nsIDOMNode;
class nsIMutableArray;

/**
 * This class reads an DOM document and adds capabilities based
 * on the content of the document
 */
class sbDeviceXMLCapabilities
{
public:
  /**
   * Initialize the XML capabilities
   */
  sbDeviceXMLCapabilities(nsIDOMElement* aRootElement,
                          sbIDevice*     aDevice = nsnull);
  /**
   * Cleanup
   */
  ~sbDeviceXMLCapabilities();
  /**
   * Reads in the capabilities and sets them on aCapabilities
   * \param aCapabilities The capabilities object to update
   */
  nsresult Read(sbIDeviceCapabilities * aCapabilities);
  /**
   * Return true if DOM document contains device capabilities.
   * \return true if DOM document contains device capabilities.
   */
  PRBool HasCapabilities() { return mHasCapabilities; }
  /**
   * Read the capabilities matching the device specified by aDevice from the
   * document specified by aDocument and return them in aCapabilities.  If no
   * matching capabilities are present, return null in aCapabilities.
   * \param aCapabilities Returned device capabilities.
   * \param aDocument Document from which to get capabilities.
   * \param aDevice Device to match against capabilities.
   */
  static nsresult GetCapabilities(sbIDeviceCapabilities** aCapabilities,
                                  nsIDOMDocument*         aDocument,
                                  sbIDevice*              aDevice = nsnull);
  /**
   * Read the capabilities matching the device specified by aDevice from the
   * root DOM noded specified by aDeviceCapsRootNode and return them in
   * aCapabilities.  If no matching capabilities are present, return null in
   * aCapabilities.
   * \param aCapabilities Returned device capabilities.
   * \param aDeviceCapsRootNode Root DOM node from which to get capabilities.
   * \param aDevice Device to match against capabilities.
   */
  static nsresult GetCapabilities(sbIDeviceCapabilities** aCapabilities,
                                  nsIDOMNode*             aDeviceCapsRootNode,
                                  sbIDevice*              aDevice = nsnull);
  /**
   * Read the capabilities matching the device specified by aDevice from the
   * file with the URI spec specified by aXMLCapabilitiesSpec and add them to
   * the device capabilities object specified by aCapabilities.  If any
   * capabilities were added, return true in aAddedCapabilities; if no
   * capabilities were added (e.g., device didn't match), return false.
   * \param aCapabilities The capabilities object to which to add capabilities.
   * \param aXMLCapabilitiesSpec URI spec of capabilities file.
   * \param aAddedCapabilities Returned true if capabilities added.
   * \param aDevice Device to match against capabilities.
   */
  static nsresult AddCapabilities
                    (sbIDeviceCapabilities* aCapabilities,
                     const char*            aXMLCapabilitiesSpec,
                     PRBool*                aAddedCapabilities = nsnull,
                     sbIDevice*             aDevice = nsnull);
  /**
   * Read the capabilities matching the device specified by aDevice from the
   * root DOM node specified by aDeviceCapsRootNode and add them to the device
   * capabilities object specified by aCapabilities.  If any capabilities were
   * added, return true in aAddedCapabilities; if no capabilities were added
   * (e.g., device didn't match), return false.
   * \param aCapabilities The capabilities object to which to add capabilities.
   * \param aDeviceCapsRootNode Root DOM node from which to get capabilities.
   * \param aAddedCapabilities Returned true if capabilities added.
   * \param aDevice Device to match against capabilities.
   */
  static nsresult AddCapabilities
                    (sbIDeviceCapabilities* aCapabilities,
                     nsIDOMNode*            aDeviceCapsRootNode,
                     PRBool*                aAddedCapabilities = nsnull,
                     sbIDevice*             aDevice = nsnull);
private:
  sbIDevice* mDevice;                   // Non-owning reference
  sbIDeviceCapabilities * mDeviceCaps;  // Non-owning reference
  nsCOMPtr<nsIDOMElement> mRootElement;
  PRBool mHasCapabilities;

  /**
   * Adds the function type to the device capabilities
   * \param aFunctionType the function type to add
   */
  nsresult AddFunctionType(PRUint32 aFunctionType);

  /**
   * Adds the content type to the device capabilities
   * \param aFunctionType The function type associated with the content type
   * \param aContentType The content type to add
   */
  nsresult AddContentType(PRUint32 aFunctionType,
                          PRUint32 aContentType);

  /**
   * Associates a given format with a content type
   * \param aContentType the content type being associated to the format
   * \param aMimeType The mime type being associated with the content type
   */
  nsresult AddMimeType(PRUint32 aContentType,
                       nsAString const & aMimeType);

  /**
   * Processing the DOM node populating the device capabilities with information
   * it finds
   */
  nsresult ProcessCapabilities(nsIDOMNode* aRootNode);

  /**
   * Process the devicecaps node in the DOM document
   * \param aDevCapNode The devicecaps DOM node
   */
  nsresult ProcessDeviceCaps(nsIDOMNode * aDevCapNode);

  /**
   * Processes audio capabilities specified under the audio node
   * \param aAudioNode The audio DOM node to process
   */
  nsresult ProcessAudio(nsIDOMNode * aAudioNode);

  /**
   * Processes explicit-sizes node
   * \param aImageSizeNode the explicit-sizes node
   * \param aImageSizes The array to receive the image sizes
   */
  nsresult ProcessImageSizes(nsIDOMNode * aImageSizeNode,
                             nsIMutableArray * aImageSizes);

  /**
   * Processes image capabilities specified under the image node
   * \param aImageNode The image DOM node to process
   */
  nsresult ProcessImage(nsIDOMNode * aImageNode);

  /**
   * Processes video capabilities specified under the video node
   * \param aVideoNode The video DOM node to process
   */
  nsresult ProcessVideo(nsIDOMNode * aVideoNode);

  /**
   * Process the video stream portion of the video format
   * \param aVideoStreamNode The DOM node containing the video
   *                         stream info
   * \param aVideoStream The newly built video stream object
   */
  nsresult
  ProcessVideoStream(nsIDOMNode* aVideoStreamNode,
                     sbIDevCapVideoStream** aVideoStream);

  /**
   * Process the audio stream portion of the video format
   * \param aAudioStreamNode The DOM node containing the audio
   *                         stream info
   * \param aAudioStream The newly built audio stream object
   */
  nsresult
  ProcessAudioStream(nsIDOMNode* aAudioStreamNode,
                     sbIDevCapAudioStream** aAudioStream);

  /**
   * Process the video format element
   * \param aDOMNode the DOM node to start processing
   */
  nsresult
  ProcessVideoFormat(nsIDOMNode* aDOMNode);

    /**
   * Processes playlist capabilities specified under the playlist node
   * \param aPlaylistNode The playlist DOM node to process
   */
  nsresult ProcessPlaylist(nsIDOMNode * aPlaylistNode);

  /**
   * Check if the device matches the capabilities node specified by
   * aCapabilitiesNode.  If it matches, return true in aDeviceMatches;
   * otherwise, return false.
   * \param aCapabilitiesNode Capabilities DOM node to check.
   * \param aDeviceMatches Returned true if device matches.
   */
  nsresult DeviceMatchesCapabilitiesNode(nsIDOMNode * aCapabilitiesNode,
                                         PRBool * aDeviceMatches);

  /**
   * Check if the device with the properties specified by aDeviceProperties
   * matches the device node specified by aDeviceNode.  If it matchces, return
   * true in aDeviceMatches; otherwise, return false.
   * \param aDeviceNode Device DOM node to check.
   * \param aDeviceProperties Device properties.
   * \param aDeviceMatches Returned true if device matches.
   */
  nsresult DeviceMatchesDeviceNode(nsIDOMNode * aDeviceNode,
                                   nsIPropertyBag2 * aDeviceProperties,
                                   PRBool * aDeviceMatches);

  /**
   * Search for the first child of the node specified by aNode with the tag name
   * specified by aTagName.  Return the first child node in aChildNode.  If no
   * child node matches, return null in aChildNode.
   * \param aNode Node to search.
   * \param aTagName Child node tag name for which to search.
   * \param aChildNode Returned child node.
   */
  nsresult GetFirstChildByTagName(nsIDOMNode*  aNode,
                                  char*        aTagName,
                                  nsIDOMNode** aChildNode);
};

#endif /* SBDEVICEXMLCAPABILITIES_H_ */

