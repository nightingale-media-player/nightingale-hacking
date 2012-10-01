/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

/**
 * \file sbMetadataCrashTracker.h
 * \brief Logs file access and maintain a blacklist of crash-causing files
 */

#ifndef SBMETADATACRASHTRACKER_H_
#define SBMETADATACRASHTRACKER_H_

// INCLUDES ===================================================================

// TODO cleanup
#include <nscore.h>
#include <prlock.h>
#include <prmon.h>
#include <nsAutoLock.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsISupports.h>
#include <nsIOutputStream.h>
#include <nsIFile.h>
#include <nsDataHashtable.h>



// CLASSES ====================================================================

/**
 * \class sbMetadataCrashTracker
 *
 * Used by sbFileMetadataService to keep a persistent log of all 
 * file activity, and avoid processing files that may have caused
 * the app to crash in the past. This is necessary because we don't
 * completely trust the sbIMetadataHandler implementations, and could
 * otherwise repeatedly crash trying to read/write the same file.
 *
 * To improve performance we don't bother flushing the log on write.
 * Instead we trust the OS to flush and close on app crash. 
 *
 * The unit test framework can't handle crashes, so to verify this
 * functionality, set a "songbird.metadata.simulate.crash.url" preference
 * to a partial filename, then run a media import.
 */
class sbMetadataCrashTracker : public nsISupports
{  
public:
  NS_DECL_ISUPPORTS

  sbMetadataCrashTracker();
  virtual ~sbMetadataCrashTracker();

  /**
   * Initialize the tracker.  Must be called once
   * at startup before first use.
   */
  nsresult Init();

  /**
   * Opens a new log file.  Called implicitly by LogURLBegin.
   */
  nsresult StartLog();

  /**
   * Close and destroy the log file. To be called when
   * all jobs have completed successfully.
   *
   * Note that this does not finalize the class, and tracking
   * can be started again by calling StartLog or LogURLBegin 
   */
  nsresult ResetLog();

  /**
   * Record that the given URL is about to be processed.
   * If we crash, this URL may be the cause. 
   */
  nsresult LogURLBegin(const nsACString& aURL);

  /**
   * Record that the given URL has been successfully processed.
   * If we crash, this URL is not to blame.
   */
  nsresult LogURLEnd(const nsACString& aURL);

  /**
   * Sets aIsBlackListed true if the given URL is
   * suspected of causing a crash on a previous run.
   */
  nsresult IsURLBlacklisted(const nsACString& aURL,
                            PRBool* aIsBlackListed);

  /**
   * Adds a URL to the blacklist
   * @note The blacklist is not threadsafe, there must be nothing else
   * attempting to use it at the same time.  This only exists for unit testing
   * purposes (see bug 22806).
   */
  nsresult AddBlacklistURL(const nsACString& aURL);
  
private:

  /**
   * Check for an existing log file, and if found,
   * add the incomplete files to the blacklist.
   */  
  nsresult ProcessExistingLog();
  
  /**
   * Hash enumeration callback used by ProcessExistingLog
   * to add to the blacklist.
   */
  static PLDHashOperator PR_CALLBACK
  AddURLsToBlacklist(nsCStringHashKey::KeyType aKey,
                     nsCString aEntry,
                     void* aUserData);

  /**
   * Hash enumeration callback used by WriteBlacklist
   * Expects an nsIOutputStream as aUserData.
   */                 
  static PLDHashOperator PR_CALLBACK
  WriteBlacklistURLToFile(nsCStringHashKey::KeyType aKey,
                          PRBool aEntry,
                          void* aUserData);

  /**
   * Check for a blacklist file and load it into mURLBlacklist
   */
  nsresult ReadBlacklist();
  
  /**
   * Write out mURLBlacklist into mBlacklistFile
   */
  nsresult WriteBlacklist();
  
  /**
   * Get an nsIFile with the given name under the user's profile directory
   */
  nsresult GetProfileFile(const nsAString& aName, nsIFile** aFile);



  nsCOMPtr<nsIFile>                           mBlacklistFile;
  nsDataHashtable<nsCStringHashKey, PRBool>   mURLBlacklist;

  // Rather than log the URL for both begin and complete we
  // assign each URL a number.  This cuts the log file size in half.
  PRUint32                                    mCounter;
  nsDataHashtable<nsCStringHashKey, PRUint32> mURLToIndexMap;
  
  nsCOMPtr<nsIFile>                           mLogFile;
  nsCOMPtr<nsIOutputStream>                   mOutputStream;
  PRLock*                                     mLock;
  
  // Value of songbird.metadata.simulate.crash.url, if set
  nsCString                                   mSimulateCrashURL;
};

#endif // SBMETADATACRASHTRACKER_H_
