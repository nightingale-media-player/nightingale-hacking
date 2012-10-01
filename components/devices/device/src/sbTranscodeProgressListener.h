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

#ifndef sbTranscodeProgressListener_H_
#define sbTranscodeProgressListener_H_

// Songbird includes
#include "sbBaseDevice.h"
#include <sbIJobProgress.h>
#include <sbIMediacoreEventListener.h>
// Forward class declarations

class sbIJobCancelable;
class sbDeviceStatusAutoOperationComplete;
class sbDeviceStatusHelper;
class sbStatusPropertyValue;

/**
 * Provides a listener to indicate when the progress listener has completed
 */
class sbTranscodeProgressListener : public sbIJobProgressListener,
                                    public sbIMediacoreEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIJOBPROGRESSLISTENER
  NS_DECL_SBIMEDIACOREEVENTLISTENER

  /**
   * Holds the status property information if any
   */
  class StatusProperty
  {
  public:
    /**
     * Initialize the status property with the source item and property name.
     * Default values will cause the status not to be updated.
     */
    StatusProperty(sbIMediaItem * aSourceItem = nsnull,
                   nsAString const & aPropertyName = nsString()) :
      mSourceItem(aSourceItem),
      mPropertyName(aPropertyName)
    {
    }
    /**
     * Sets the status property to aValue if the property name was given
     */
    nsresult SetValue(nsAString const & aValue)
    {
      if (!mPropertyName.IsEmpty() && mSourceItem) {
        nsresult rv = mSourceItem->SetProperty(mPropertyName, aValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      return NS_OK;
    }
  private:
    nsCOMPtr<sbIMediaItem> mSourceItem;
    nsString mPropertyName;
  };
  static sbTranscodeProgressListener *
  New(sbBaseDevice * aDeviceBase,
      sbDeviceStatusHelper * aDeviceStatusHelper,
      sbIMediaItem *aItem,
      PRMonitor * aCompleteNotifyMonitor = nsnull,
      StatusProperty const & aStatusProperty = StatusProperty(),
      sbIJobCancelable * aCancel = nsnull);

  bool IsComplete() const { return mIsComplete; };
  bool IsAborted() const { return mAborted; }
private:
  inline
  sbTranscodeProgressListener(sbBaseDevice * aDeviceBase,
                              sbDeviceStatusHelper * aDeviceStatusHelper,
                              sbIMediaItem *aItem,
                              PRMonitor * aCompleteNotifyMonitor,
                              StatusProperty const & aStatusProperty,
                              sbIJobCancelable * aCancel);

  ~sbTranscodeProgressListener();
  /**
   * Sets the device status as completed either failed or succeeded
   * \param aJobProgress The progress data of the job
   */
  nsresult Completed(sbIJobProgress * aJobProgress);

  /**
   * Sets the current progress of the transcoding job
   * \param aJobProgress The progress data of the job
   */
  nsresult SetProgress(sbIJobProgress * aJobProgress);

  /**
   * Sets the status property of the item if given
   */
  inline
  nsresult SetStatusProperty(sbStatusPropertyValue const & aValue);

  // Raw manually ref counted pointer because nsISupports is ambiguous
  sbBaseDevice * mBaseDevice;

  // Non-owning reference, reference to mBaseDevice will keep this alive
  sbDeviceStatusHelper * mStatus;

  nsCOMPtr<sbIMediaItem> mItem;
  PRMonitor *mCompleteNotifyMonitor;
  PRInt32 mIsComplete;
  PRUint32 mTotal;
  StatusProperty mStatusProperty;
  nsCOMPtr<sbIJobCancelable> mCancel;
  bool mAborted;
};


#endif /* sbTranscodeProgressListener_H_ */
