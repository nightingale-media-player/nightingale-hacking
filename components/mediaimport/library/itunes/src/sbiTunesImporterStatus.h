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

#ifndef SBITUNESIMPORTERSTATUS_H_
#define SBITUNESIMPORTERSTATUS_H_

#include "sbiTunesImporterCommon.h"

#include <nsAutoPtr.h>
#include <nsStringAPI.h>
#include <nsThreadUtils.h>

#include <sbIDataRemote.h>

class sbIJobProgress;
class sbiTunesImporterJob;

/**
 * This class manages status updates for the iTunes importer
 */
class sbiTunesImporterStatus
{
public:
  static sbiTunesImporterStatus * New(sbiTunesImporterJob * aJobProgress);

 ~sbiTunesImporterStatus();
  
 /**
  * Returns PR_TRUE if a cancel has been requested
  */
 PRBool CancelRequested();
  /**
   * Initializes the dataremote and job progress member 
   */
  nsresult Initialize();
  /**
   * Clean up resources
   */
  void Finalize();
  /**
   * Resets the status
   */
  nsresult Reset();
  /**
   * Sets the status message
   */
  void SetStatusText(nsAString const & aMsg);
  /**
   * Sets the current progress
   */
  void SetProgress(PRUint32 aProgress);
  /**
   * Sets the max process value
   */
  void SetProgressMax(PRUint32 aMaxProgress);
  /**
   * Signals we're done
   */
  void Done() {
    mDone = PR_TRUE;
  }
  /**
   * Updates the status to the UI
   */
  nsresult Update();
private:
  typedef nsRefPtr<sbiTunesImporterJob> sbiTunesImporterJobPtr;
  
  /**
   * Flag denoting if we're done or not
   */
  PRBool mDone;
  /**
   * Our job progress object for notifications
   */
  sbiTunesImporterJobPtr mJobProgress;
  /**
   * The last reported progress of the import
   */
  PRUint32 mLastProgress;  
  /**
   * The current progress
   */
  PRUint32 mProgress;
  /**
   * The maximum progress value
   */
  PRUint32 mProgressMax;
  /**
   * Our data remote used by the UI
   */
  sbIDataRemotePtr mStatusDataRemote;
  /**
   * Initializes states and the job progress object
   */
  sbiTunesImporterStatus(sbiTunesImporterJob * aJobProgress);
  /**
   * Current status message
   */
  nsString mStatusText;
  /**
   * Denotes whether we've set the total for the job progress object
   */
  PRBool mTotalSet;
  
  // Prevent copying and assignment
  sbiTunesImporterStatus(sbiTunesImporterStatus const &);
  sbiTunesImporterStatus & operator =(sbiTunesImporterStatus const &);
};

#endif /* SBITUNESIMPORTERSTATUS_H_ */
