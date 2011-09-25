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

#ifndef SBDEVICETRANSCODING_H_
#define SBDEVICETRANSCODING_H_

// Standard includes
#include <list>

// Mozilla includes
#include <nsIArray.h>

// Nightingale interfaces
#include <sbIMediaItem.h>
#include <sbITranscodeManager.h>
#include <sbITranscodeJob.h>
#include <sbITranscodeProfile.h>

// Nightingale local includes
#include "sbBaseDevice.h"

class sbIMediaInspector;
class sbITranscodeVideoJob;
class sbDeviceStatusHelper;

class sbDeviceTranscoding
{
public:
  friend class sbBaseDevice;

  /**
   *  Types brought in from sbBaseDevice
   */
  typedef sbBaseDevice::Batch Batch;
  typedef sbBaseDevice::TransferRequest TransferRequest;
  typedef sbBaseDevice::TransferRequest::CompatibilityType CompatibilityType;


  /* Get an array of all the sbITranscodeProfile instances supported for this
   * The default implementation filters all available profiles by the
   * capabilities of the device.
   */
  virtual nsresult GetSupportedTranscodeProfiles(PRUint32 aType,
                                                 nsIArray **aSupportedProfiles);

  /**
   * Returns the profile for the given media item
   * \param aMediaItem The media item to find the profile for
   * \param aProfile the profile found or may be null if no transcoding
   *                 is needed.
   * \param aDeviceCompatibility Whether this item is supported directly by the
   *                             device, or if transcoding is needed.  Will be
   *                             set even if no profile is found.
   * \note NS_ERROR_NOT_AVAILABLE is returned if no suitable profile is found
   *       but transcoding is needed
   */
  nsresult FindTranscodeProfile(sbIMediaItem * aMediaItem,
                                sbITranscodeProfile ** aProfile,
                                CompatibilityType * aDeviceCompatibility);

  /**
   * \brief Select a transcode profile to use when transcoding to this device.
   * \param aTranscodeType The type of transcoding profile to look for.
   * \param aProfile       The profile found or may be null if no transcoding
   *                       is needed.
   * \note This selects the best available transcoding profile for this device
   *       for arbitrary input - even if the thing to be transcoded is directly
   *       supported by the device.
   * \note NS_ERROR_NOT_AVAILABLE is returned if no suitable profile is found
   */
  nsresult SelectTranscodeProfile(PRUint32 aTranscodeType,
                                  sbITranscodeProfile **aProfile);

  /**
   * Prepare a batch for transcoding. This processes items in the batch and
   * determines which of them require transcoding and/or album-art transcoding.
   *
   * \brief aBatch The batch to process
   */
  nsresult PrepareBatchForTranscoding(Batch & aBatch);

  /**
   * Returns the transcode type for the item
   */
  static PRUint32 GetTranscodeType(sbIMediaItem * aMediaItem);

  /**
   * Returns the media format for a media item appropriate for a particular
   * type of transcoding.
   */
  nsresult GetMediaFormat(PRUint32 aTranscodeType,
                          sbIMediaItem* aMediaItem,
                          sbIMediaFormat** aMediaFormat);

  /**
   * Returns a media format for an audio media item based on the file extension
   * and properties on the item (not based on file content!)
   */
  nsresult GetAudioFormatFromMediaItem(sbIMediaItem* aMediaItem,
                                       sbIMediaFormat** aMediaFormat);
  /**
   * Get a media inspector
   */
  nsresult GetMediaInspector(sbIMediaInspector** _retval);

  /**
   * Transcode a media item to the destination specified
   * by aDestinationURI.  If aTranscodedDestinationURI is not null, return the
   * final destination URI with transcoded file extension in
   * aTranscodedDestinationURI.
   */
  nsresult TranscodeMediaItem(sbIMediaItem *aItem,
                              sbDeviceStatusHelper * aDeviceStatusHelper,
                              nsIURI * aDestinationURI,
                              nsIURI ** aTranscodedDestinationURI = nsnull);
private:
  sbDeviceTranscoding(sbBaseDevice * aBaseDevice);
  nsresult GetTranscodeManager(sbITranscodeManager ** aTranscodeManager);

  sbBaseDevice * mBaseDevice;
  nsCOMPtr<nsIArray> mTranscodeProfiles;
  nsCOMPtr<sbIMediaInspector> mMediaInspector;
  nsCOMPtr<sbITranscodeManager> mTranscodeManager;
};

#endif
