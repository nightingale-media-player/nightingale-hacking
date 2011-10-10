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
 * \file sbBackgroundThreadMetadataProcessor.h
 * \brief Runs sbIMetadataHandlers on a background thread
 */

#ifndef SBBACKGROUNDTHREADMETADATAPROCESSOR_H_
#define SBBACKGROUNDTHREADMETADATAPROCESSOR_H_

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsISupports.h>
#include <nsIRunnable.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsTArray.h>

// CLASSES ====================================================================

class sbFileMetadataService;
class sbMetadataJobItem;

/**
 * \class sbBackgroundThreadMetadataProcessor
 * Used by sbFileMetadataService to process sbMetadataJobItem handlers
 * on a background thread.
 */
class sbBackgroundThreadMetadataProcessor : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  sbBackgroundThreadMetadataProcessor(sbFileMetadataService* aManager);
  virtual ~sbBackgroundThreadMetadataProcessor();

  /**
   * Make sure that the background thread is running.  
   * Note that this method should be called any time
   * a new job is added, since the thread may go to sleep
   * if it runs out of things to do.
   */
  nsresult Start();
  
  /**
   * Shut down the background thread.  
   * Must be called from the main thread.
   */
  nsresult Stop();

protected:
  
  // The job manager that owns this processor
  nsRefPtr<sbFileMetadataService>         mJobManager;
  
  // Thread to call nsIRunnable.Run()  
  nsCOMPtr<nsIThread>                     mThread;
    
  // Flag to indicate that the thread should stop processing
  PRBool                                  mShouldShutdown;
  
  // Monitor used to wake up the thread when more items
  // are added, or when it is time to shut down
  PRMonitor*                              mMonitor;
};

#endif // SBBACKGROUNDTHREADMETADATAPROCESSOR_H_
