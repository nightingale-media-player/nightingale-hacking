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

#ifndef SBDEVICEXMLCAPABILITIES_H_
#define SBDEVICEXMLCAPABILITIES_H_

#include <nsCOMPtr.h>
#include <nsStringAPI.h>

class sbIDeviceCapabilities;
class sbIDevCapVideoStream;
class sbIDevCapAudioStream;
class sbIDevCapVideoFormatType;
class nsIDOMDocument;
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
   * Initialize the XML document
   */
  sbDeviceXMLCapabilities(nsIDOMDocument* aDocument);
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
private:
  nsCOMPtr<nsIDOMDocument> mDocument;
  sbIDeviceCapabilities * mDeviceCaps;  // Non-owning reference
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
   * \param aFormat The format being associated with the content type
   */
  nsresult AddFormat(PRUint32 aContentType,
                     nsAString const & aFormat);

  /**
   * Processing the DOM document object populating the device capabilities
   * with information it finds
   */
  nsresult ProcessDocument(nsIDOMDocument * aDocument);

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
};

#endif /* SBDEVICEXMLCAPABILITIES_H_ */

