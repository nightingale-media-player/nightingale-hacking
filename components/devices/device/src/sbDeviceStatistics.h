/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
*/

#ifndef SBDEVICESTATISTICS_H_
#define SBDEVICESTATISTICS_H_

/**
 * \file  sbDeviceStatistics.h
 * \brief Songbird Device Statistics Definitions.
 */

//------------------------------------------------------------------------------
//
// Device statistics imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIDeviceLibrary.h>
#include <sbIMediaListListener.h>

// Mozilla imports.
//#include <nsAutoLock.h>
#include <nsCOMPtr.h>


//------------------------------------------------------------------------------
//
// Device statistics services classes.
//
//------------------------------------------------------------------------------

/**
 * Keeps track of the used statistics on the device.
 */

class sbDeviceStatistics : public sbIMediaListEnumerationListener
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Implemented interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER


  //
  // Public device statistics services.
  //

  static nsresult New(class sbBaseDevice*  aDevice,
                      sbDeviceStatistics** aDeviceStatistics);

  nsresult AddLibrary(sbIDeviceLibrary* aLibrary);

  nsresult RemoveLibrary(sbIDeviceLibrary* aLibrary);

  nsresult AddItem(sbIMediaItem* aMediaItem);

  nsresult RemoveItem(sbIMediaItem* aMediaItem);

  nsresult RemoveAllItems(sbIDeviceLibrary* aLibrary);


  //
  // Setter/getter services.
  //

  // Audio count.
  PRUint32 AudioCount();

  void SetAudioCount(PRUint32 aAudioCount);

  void AddAudioCount(PRInt32 aAddAudioCount);

  // Audio used.
  PRUint64 AudioUsed();

  void SetAudioUsed(PRUint64 aAudioUsed);

  void AddAudioUsed(PRInt64 aAddAudioUsed);

  // Audio play time.
  PRUint64 AudioPlayTime();

  void SetAudioPlayTime(PRUint64 aAudioPlayTime);

  void AddAudioPlayTime(PRInt64 aAddAudioPlayTime);

  // Video count.
  PRUint32 VideoCount();

  void SetVideoCount(PRUint32 aVideoCount);

  void AddVideoCount(PRInt32 aAddVideoCount);

  // Video used.
  PRUint64 VideoUsed();

  void SetVideoUsed(PRUint64 aVideoUsed);

  void AddVideoUsed(PRInt64 aAddVideoUsed);

  // Video play time.
  PRUint64 VideoPlayTime();

  void SetVideoPlayTime(PRUint64 aVideoPlayTime);

  void AddVideoPlayTime(PRInt64 aAddVideoPlayTime);

  // Image count.
  PRUint32 ImageCount();

  void SetImageCount(PRUint32 aImageCount);

  void AddImageCount(PRInt32 aAddImageCount);

  // Image used.
  PRUint64 ImageUsed();

  void SetImageUsed(PRUint64 aImageUsed);

  void AddImageUsed(PRInt64 aAddImageUsed);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mBaseDevice                Base device for statistics.  Used to interface
  //                            to base device class.  Don't keep a reference to
  //                            the device to avoid a cycle.
  //
  // mStatLock                  Lock used for serializing access to statistics.
  // mAudioCount                Count of the number of audio items.
  // mAudioUsed                 Count of the number of bytes used for audio.
  // mAudioPlayTime             Total audio playback time in microseconds.
  // mVideoCount                Count of the number of video items.
  // mVideoUsed                 Count of the number of bytes used for video.
  // mVideoPlayTime             Total video playback time in microseconds.
  // mVideoCount                Count of the number of image items.
  // mVideoUsed                 Count of the number of bytes used for images.
  //

  class sbBaseDevice*           mBaseDevice;

  PRLock *                      mStatLock;
  PRUint32                      mAudioCount;
  PRUint64                      mAudioUsed;
  PRUint64                      mAudioPlayTime;
  PRUint32                      mVideoCount;
  PRUint64                      mVideoUsed;
  PRUint64                      mVideoPlayTime;
  PRUint32                      mImageCount;
  PRUint64                      mImageUsed;


  //
  // Private device statistics services.
  //

  sbDeviceStatistics();

  virtual ~sbDeviceStatistics();

  nsresult Initialize(class sbBaseDevice* aDevice);

  nsresult ClearLibraryStatistics(sbIDeviceLibrary* aLibrary);

  nsresult UpdateForItem(sbIMediaItem* aMediaItem,
                         PRBool        aItemAdded);


  // Prevent derivation.
  sbDeviceStatistics(sbDeviceStatistics const &) {}
};

#endif /* SBDEVICESTATISTICS_H_ */

