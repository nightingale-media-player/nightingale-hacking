/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END NIGHTINGALE GPL
//
*/

/**
* \file MetadataHandlerWMA.h
* \brief 
*/

#ifndef __METADATA_HANDLER_WMA_H__
#define __METADATA_HANDLER_WMA_H__

// INCLUDES ===================================================================
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include "sbIMetadataHandler.h"
#include "sbIMetadataHandlerWMA.h"
#include "sbIMetadataChannel.h"

// DEFINES ====================================================================
#define NIGHTINGALE_METADATAHANDLERWMA_CONTRACTID  "@getnightingale.com/Nightingale/MetadataHandler/WMA;1"
#define NIGHTINGALE_METADATAHANDLERWMA_CLASSNAME   "Nightingale WMA Metadata Handler Interface"

// {2D42C52D-3B4E-4085-99F6-53DB14C5996C}
#define NIGHTINGALE_METADATAHANDLERWMA_CID { 0x2d42c52d, 0x3b4e, 0x4085, { 0x99, 0xf6, 0x53, 0xdb, 0x14, 0xc5, 0x99, 0x6c } }

// FUNCTIONS ==================================================================

// CLASSES ====================================================================

class nsIChannel;
struct IWMHeaderInfo3;
struct IWMPMedia3;

class sbMetadataHandlerWMA : public sbIMetadataHandler,
                             public sbIMetadataHandlerWMA
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAHANDLER
  NS_DECL_SBIMETADATAHANDLERWMA

  sbMetadataHandlerWMA();

protected:
  ~sbMetadataHandlerWMA();

  nsCOMPtr<sbIMutablePropertyArray>  m_PropertyArray;
  nsCOMPtr<sbIMetadataChannel>       m_ChannelHandler;
  nsCOMPtr<nsIChannel>               m_Channel;
  nsString                           m_FilePath;
  PRBool                             m_Completed;

  /**
   * Read a single string-based metadata (for the WMF API methods)
   * @param aHeaderInfo the header data to read from
   * @param aKey the (WMF) metadata key name to read
   * @return the value of the metadata found in the file, or a void string
   *         if no values are found
   */
  nsString ReadHeaderValue(IWMHeaderInfo3 *aHeaderInfo, const nsAString &aKey);
  
  /**
   * Utility function for the WMP metadata reading paths
   * Given a (absolute native) file path, return a IWMPMedia3 that can be used
   * to read metadata
   * @param aFilePath the path to the file to read
   * @param aMedia [out] the media object
   */
  NS_METHOD CreateWMPMediaItem(const nsAString& aFilePath, IWMPMedia3** aMedia);

  /**
   * Read generic metadata from a file, using the WMFSDK APIs
   * @param aFilePath the path of the file to read
   * @param _retval the number of metadata values read
   * @see sbIMetadataHandler::read()
   */
  NS_METHOD ReadMetadataWMFSDK(const nsAString& aFilePath, PRInt32* _retval);
  /**
   * Read generic metadata from a file, using the WMP APIs
   * @param aFilePath the path of the file to read
   * @param _retval the number of metadata values read
   * @see sbIMetadataHandler::read()
   */
  NS_METHOD ReadMetadataWMP(const nsAString& aFilePath, PRInt32* _retval);

  /**
   * Read album art meadata from a file, using the WMFSDK APIs
   * @param aFilePath the path of the file to read
   * @param aMimeType [out] the mime type of the metadata
   * @param aDataLen [out] the length of the image data stream
   * @param aData [out] the raw image data
   */
  NS_METHOD ReadAlbumArtWMFSDK(const nsAString &aFilePath,
                               nsACString      &aMimeType,
                               PRUint32        *aDataLen,
                               PRUint8        **aData);
  /**
   * Read album art meadata from a file, using the WMP APIs
   * @param aFilePath the path of the file to read
   * @param aMimeType [out] the mime type of the metadata
   * @param aDataLen [out] the length of the image data stream
   * @param aData [out] the raw image data
   */
  NS_METHOD ReadAlbumArtWMP(const nsAString &aFilePath,
                            nsACString      &aMimeType,
                            PRUint32        *aDataLen,
                            PRUint8        **aData);
  
  /**
   * Internal helper for writing the image data
   * This is used because both SetImageData(0 and Write() may want to do this,
   * but we can't have two IWMMetadataEditor instances open on the same file
   * @param aType the image type
   * @param aURL the url of the image file (must be local file)
   * @param aHeader a pre-opened IWMHeaderInfo3 to write to
   * @param aSuccess [out] whether data was written
   * @see sbIMetadataHandler::SetImageData
   */
  NS_METHOD SetImageDataInternal(PRInt32          aType,
                                 const nsAString &aURL,
                                 IWMHeaderInfo3  *aHeader,
                                 PRBool          &aSuccess);

};

#endif // __METADATA_HANDLER_WMA_H__
