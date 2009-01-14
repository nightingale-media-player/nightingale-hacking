/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include <nsAutoLock.h>

/**
 * Keeps track of the used statistics on the device
 */
class sbDeviceStatistics
{
public:
  /**
   * Initialize the stats to 0 and create the loc
   */
	sbDeviceStatistics() : mAudioUsed(0),
	                       mVideoUsed(0),
	                       mOtherUsed(0),
	                       mStatLock(nsAutoLock::NewLock(__FILE__ "::mStatLock"))
  {
	}
	/**
	 * Free the lock
	 */
	~sbDeviceStatistics()
	{
	  nsAutoLock::DestroyLock(mStatLock);
	  mStatLock = nsnull;
	}
	/**
	 * Return the audio used in bytes
	 */
	PRUint64 AudioUsed() const
	{
	  nsAutoLock lock(mStatLock);
	  return mAudioUsed;
	}
	/**
	 * Return the video used in bytes
	 */
	PRUint64 VideoUsed() const
	{
    nsAutoLock lock(mStatLock);
	  return mVideoUsed;
	}
	/**
	 * Return content that is not video or audio in bytes
	 */
	PRUint64 OtherUsed() const
	{
    nsAutoLock lock(mStatLock);
	  return mOtherUsed;
	}
  /**
   * Returns the number of bytes used by all content
   */
  PRUint64 TotalUsed()
  {
    nsAutoLock lock(mStatLock);
    return mAudioUsed + mVideoUsed + mOtherUsed;
  }
	/**
	 * Set the amount of audio used in bytes
	 */
  void SetAudioUsed(PRUint64 audio)
  {
    nsAutoLock lock(mStatLock);
    mAudioUsed = audio;
  }
  /**
   * Set the amount of video used in bytes
   */
  void SetVideoUsed(PRUint64 video)
  {
    nsAutoLock lock(mStatLock);
    mVideoUsed = video;
  }
  /**
   * Set the amount of other content in bytes
   */
  void SetOtherUsed(PRUint64 other)
  {
    nsAutoLock lock(mStatLock);
    mOtherUsed = other;
  }
  /**
   * Adds bytes to the audio used stat
   */
  void AddAudioUsed(PRUint64 audio)
	{
    nsAutoLock lock(mStatLock);
	  mAudioUsed += audio;
	}
  /**
   * Adds bytes to the video used stat
   */
  void AddVideoUsed(PRUint64 video)
  {
    nsAutoLock lock(mStatLock);
    mVideoUsed += video;
  }
  /**
   * Adds bytes to the other content used stat
   */
  void AddOtherUsed(PRUint64 other)
  {
    nsAutoLock lock(mStatLock);
    mOtherUsed += other;
  }
  /**
   * Subtracts bytes from the audio used stat
   */
  void SubAudioUsed(PRUint64 audio)
  {
    nsAutoLock lock(mStatLock);
    mAudioUsed -= audio;
  }
  /**
   * Subtracts bytes from the video used stat
   */
  void SubVideoUsed(PRUint64 video)
  {
    nsAutoLock lock(mStatLock);
    mVideoUsed -= video;
  }
  /**
   * Subtracts bytes from the other content used stat
   */
  void SubOtherUsed(PRUint64 other)
  {
    nsAutoLock lock(mStatLock);
    mOtherUsed -= other;
  }
private:
  PRUint64 mAudioUsed;
  PRUint64 mVideoUsed;
  PRUint64 mOtherUsed;
  PRLock * mStatLock;

  /**
   * Prevent derivation
   */
  sbDeviceStatistics(sbDeviceStatistics const &) {}
};

#endif /*SBDEVICESTATISTICS_H_*/
