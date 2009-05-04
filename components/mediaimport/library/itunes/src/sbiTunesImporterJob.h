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

#ifndef SBITUNESIMPORTERJOB_H_
#define SBITUNESIMPORTERJOB_H_

#include <nsStringAPI.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsTArray.h>

#include <sbIJobProgress.h>
#include <sbIJobCancelable.h>

class nsIThread;

/**
 * This class provides Job status information and notification.
  */
class sbiTunesImporterJob : public sbIJobProgress,
                            public sbIJobCancelable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIJOBCANCELABLE

  /**
   * Allocator
   */
  static sbiTunesImporterJob * New();
  /**
   * Request cancel
   */
  PRBool CancelRequested() const {
    return mCancelRequested;
  }
  /**
   * Sets the progress status
   */
  nsresult SetStatus(PRUint32 aStatus);
  /**
   * Sets the status mesage of the the status window
   */
  nsresult SetStatusText(nsAString const & aStatusText);
  /**
   * Sets the title text of the status window
   */ 
  nsresult SetTitleText(nsAString const & aTitleText);
  /**
   * Sets the progress
   * XXX TODO Not sure how this is different from SetStatus
   */
  nsresult SetProgress(PRUint32 aProgress);
  /**
   * Sets the total items being processed
   */
  nsresult SetTotal(PRUint32 aTotal);
  /**
   * Adds an error message
   */
  nsresult AddErrorMessage(nsAString const & aErrorMessage);
protected:  
  /**
   * Initializes the locks and state
   */
  sbiTunesImporterJob();
  /**
   * Needed so forward declares aren't referenced here
   */
  virtual ~sbiTunesImporterJob();
private:
  // Useful typedefs
  typedef nsTArray<nsString> StringArray;
  typedef nsCOMArray<sbIJobProgressListener> ListenerArray;

  /**
   * Holds whether we can cancel or not
   */
  PRBool mCanCancel;
  /**
   * Cancel requested indicator
   */
  PRBool mCancelRequested;
  /**
   * Collection of error messages
   */
  StringArray mErrorMessages;
  /**
   * Collection of listeners
   */
  ListenerArray mListeners;
  /**
   * Current status?
   */
  PRUint32 mStatus;
  /**
   * Current status message
   */
  nsString mStatusText;
  /**
   * Current title
   */
  nsString mTitleText;
  /**
   * The progress?
   */
  PRUint32 mProgress;
  /**
   * The total items being processed
   */
  PRUint32 mTotal;
  /**
   * Sends update notifications
   */
  nsresult UpdateProgress();
  
  // Prevent copying and assignment
  sbiTunesImporterJob(sbiTunesImporterJob const &);
  sbiTunesImporterJob & operator = (sbiTunesImporterJob const);
};

#endif /* SBITUNESIMPORTERJOB_H_ */
