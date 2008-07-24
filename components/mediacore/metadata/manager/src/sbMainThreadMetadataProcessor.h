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

/**
 * \file sbMainThreadMetadataProcessor.h
 * \brief Runs sbIMetadataHandlers with a timer on the main thread
 */

#ifndef SBMAINTHREADMETADATAPROCESSOR_H_
#define SBMAINTHREADMETADATAPROCESSOR_H_

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsISupports.h>
#include <nsITimer.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsTArray.h>

// CLASSES ====================================================================

class sbFileMetadataService;
class sbMetadataJobItem;

/**
 * \class sbMainThreadMetadataProcessor
 * Used by sbFileMetadataService to process sbMetadataJobItem handlers
 * on the main thread thread.
 * Runs multiple handlers at once (if async) and polls them for completion.
 */
class sbMainThreadMetadataProcessor : public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  sbMainThreadMetadataProcessor(sbFileMetadataService* aManager);
  virtual ~sbMainThreadMetadataProcessor();

  /**
   * Make sure that the timer is running.  
   * Note that this method should be called any time
   * a new job is added, since the timer will shut down
   * if it runs out of things to do.
   */
  nsresult Start();
  
  /**
   * Shut down the timer.
   * Must be called from the main thread.
   */
  nsresult Stop();

protected:
  
  // The job manager that owns this processor
  nsRefPtr<sbFileMetadataService>         mJobManager;
  
  // Items that are currently being processed 
  // (have async sbIMetadataHandlers that need polling)
  nsTArray<nsRefPtr<sbMetadataJobItem> >  mCurrentJobItems;
  
  // Timer used to start and poll mCurrentJobItems
  nsCOMPtr<nsITimer>                      mTimer;
  
  // Flag to indicate that the timer is running
  PRBool                                  mRunning;
};

#endif // SBMAINTHREADMETADATAPROCESSOR_H_
