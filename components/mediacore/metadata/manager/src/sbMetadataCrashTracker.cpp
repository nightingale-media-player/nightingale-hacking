/* vim: set sw=2 :miv */
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
 * \file sbMetadataCrashTracker.cpp
 * \brief Implementation for sbMetadataCrashTracker.h
 */

// INCLUDES ===================================================================

// TODO clean up
#include <nspr.h>
#include <nscore.h>
#include <mozilla/Mutex.h>
#include <nsServiceManagerUtils.h>
#include <nsCRTGlue.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>
#include <nsNetUtil.h>
#include <nsIURI.h>
#include <nsIIOService.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch2.h>
#include <nsIInputStream.h>
#include <nsILineInputStream.h>

#include "sbMetadataCrashTracker.h"

// DEFINES ====================================================================

// Initial hashtable capacity for mURLToIndexMap
#define DEFAULT_MAP_SIZE 20

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#define TRACE(args) PR_LOG(gMetadataLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS0(sbMetadataCrashTracker);

sbMetadataCrashTracker::sbMetadataCrashTracker() :
  mBlacklistFile(nsnull),
  mCounter(0),
  mLogFile(nsnull),
  mOutputStream(nsnull),
  mLock(nsnull)
{
  TRACE(("sbMetadataCrashTracker[0x%.8x] - ctor", this));
  MOZ_COUNT_CTOR(sbMetadataCrashTracker);
}


sbMetadataCrashTracker::~sbMetadataCrashTracker()
{
  TRACE(("sbMetadataCrashTracker[0x%.8x] - dtor", this));
  MOZ_COUNT_DTOR(sbMetadataCrashTracker);
  ResetLog();
}


nsresult sbMetadataCrashTracker::Init()
{
  NS_ASSERTION(NS_IsMainThread(), 
    "sbMetadataCrashTracker::Init: MUST NOT INIT OFF OF MAIN THREAD!");
  nsresult rv = NS_OK;

  // Set up a map to track file URLs while logging
  PRBool success = mURLToIndexMap.Init(DEFAULT_MAP_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Set up the list of scary crashy URLs
  success = mURLBlacklist.Init(DEFAULT_MAP_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  rv = GetProfileFile(NS_LITERAL_STRING("metadata-url-io.blacklist"),
                      getter_AddRefs(mBlacklistFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // Load the existing URL blacklist if we have one
  rv = ReadBlacklist();
  if (NS_FAILED(rv)) {
    NS_ERROR("sbMetadataCrashTracker::Init failed to load blacklist!");
  }
  
  // Get the file we will use for logging
  mozilla::MutexAutoLock lock(mLock);
  
  rv = GetProfileFile(NS_LITERAL_STRING("metadata-io.log"), 
                      getter_AddRefs(mLogFile));
  NS_ENSURE_SUCCESS(rv, rv);

  // See if we crashed during the last run, and add to 
  // the blacklist if needed
  rv = ProcessExistingLog();
  if (NS_FAILED(rv)) {
    NS_ERROR("sbMetadataCrashTracker::Init failed to process existing log!");
  }  
  
  // Allow the user to set a file name in a pref that, if encountered,
  // will result in an immediate crash
  nsCOMPtr<nsIPrefBranch> prefService =
      do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS( rv, rv);
  // Get the value. We don't care if this fails.
  prefService->GetCharPref("songbird.metadata.simulate.crash.url", 
                           getter_Copies(mSimulateCrashURL));
  return NS_OK;
}


nsresult sbMetadataCrashTracker::StartLog()
{
  NS_ENSURE_STATE(mLogFile);
  if (mOutputStream) {
    ResetLog();
  }
  nsresult rv = NS_OK;

  // Open a new log file
  mozilla::MutexAutoLock lock(mLock);

  nsCOMPtr<nsIFileOutputStream> fileStream =
          do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 ioFlags = PR_TRUNCATE | PR_CREATE_FILE | PR_WRONLY; 
  rv = fileStream->Init(mLogFile, ioFlags, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  mOutputStream = do_QueryInterface(fileStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return rv;
}


nsresult sbMetadataCrashTracker::ResetLog()
{
  nsresult rv = NS_OK;

  mozilla::MutexAutoLock lock(mLock);
  
  if (mOutputStream) {
    mOutputStream->Close();
    mOutputStream = nsnull;
    mLogFile->Remove(PR_FALSE);
  }
  
  mURLToIndexMap.Clear();
  
  return rv;
}
 
   
nsresult 
sbMetadataCrashTracker::LogURLBegin(const nsACString& aURL)
{
  nsresult rv = NS_OK;
  
  // Make sure we have a log
  if (!mOutputStream) {
    rv = StartLog();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mozilla::MutexAutoLock lock(mLock);
  
  // Rather than log the URL for both begin and end we
  // assign each URL a number.  
  // This cuts the log file size in half.
  PRUint32 index = mCounter++;
  mURLToIndexMap.Put(aURL, index);

  // Record that we are Beginning this URL, and save the 
  // index so that we can match up the End record.
  nsCString output("B"); 
  output.AppendInt(index);
  output.Append(" ");
  output.Append(aURL);
  output.Append("\n");
  
  PRUint32 bytesWritten;
  rv = mOutputStream->Write(output.BeginReading(), 
                            output.Length(), 
                            &bytesWritten);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // For testing it is convenient to be able to force a crash.
  // Check to see if this URL is supposed to crash us.
  if (!mSimulateCrashURL.IsEmpty()) {
    if (output.Find(mSimulateCrashURL, PR_TRUE) != -1) {
      LOG(("LogURLBegin forcing a crash for %s", output.BeginReading()));
      PRBool* crash;
      crash = nsnull;
      *crash = PR_TRUE;
    }
  }

  return rv;
}


nsresult 
sbMetadataCrashTracker::LogURLEnd(const nsACString& aURL)
{
  NS_ENSURE_STATE(mOutputStream);
  nsresult rv = NS_OK;
  
  mozilla::MutexAutoLock lock(mLock);
  
  // Look up the index of this URL
  PRUint32 index = 0;
  PRBool success = mURLToIndexMap.Get(aURL, &index);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  mURLToIndexMap.Remove(aURL);
  
  // Write an End record
  nsCString output("E"); 
  output.AppendInt(index);
  output.Append("\n");
  
  PRUint32 bytesWritten;
  rv = mOutputStream->Write(output.BeginReading(), 
                            output.Length(), 
                            &bytesWritten);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

   
nsresult 
sbMetadataCrashTracker::IsURLBlacklisted(const nsACString& aURL,
                                         PRBool* aIsBlackListed)
{
  // Look up the URL in the hash table.
  // No need to lock, since we only update mURLBlacklist on Init.
  *aIsBlackListed = mURLBlacklist.Get(aURL, nsnull);
  return NS_OK;
}

nsresult
sbMetadataCrashTracker::AddBlacklistURL(const nsACString& aURL)
{
  /**
   * NOTE
   * This method is not threadsafe by design; it only exists to be used in the
   * unit test.  This should not be called by production code.
   */
  NS_ASSERTION(NS_IsMainThread(),
               "sbMetadataCrashTracker::AddBlacklistURL"
               " Unexpectedly called off main thread");
  mURLBlacklist.Put(aURL, PR_TRUE);
  return NS_OK;
}


// -- Private --

nsresult 
sbMetadataCrashTracker::ProcessExistingLog()
{
  NS_ENSURE_STATE(mLogFile);
  nsresult rv = NS_OK;
  
  // Did we crash on last run?
  PRBool exists = PR_FALSE;
  rv = mLogFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // No file, no crash
  if (!exists) {
    return NS_OK;
  }
  
  // Read the log file and find any URLs that didn't complete.
  nsCOMPtr<nsIInputStream> inputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), mLogFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsILineInputStream> lineStream(do_QueryInterface(inputStream, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsDataHashtable<nsCStringHashKey, nsCString> indexToURLMap;
  indexToURLMap.Init(DEFAULT_MAP_SIZE);

  PRBool more = PR_TRUE;
  nsCString line;
  nsCString url;
  PRBool hashSuccess = PR_FALSE;
  do {
    rv = lineStream->ReadLine(line, &more);
    if (NS_SUCCEEDED(rv) && line.Length() >= 2) {
      switch (line.First()) {
        // Handle Begin records
        case 'B': { 
          PRInt32 separatorIndex = line.FindChar(' ', 1);
          if (separatorIndex > 0 && separatorIndex < (PRInt32)(line.Length() - 1)) {
            // Get the URL
            url = Substring(line, separatorIndex + 1);
            // Get the index
            line = Substring(line, 1, separatorIndex - 1);
            // Save in the hash, so that we can look for a matching End record
            indexToURLMap.Put(line, url);
            
            TRACE(("sbMetadataCrashTracker::ProcessExistingLog " \
                   "- found Begin record '%s', '%s'", 
                   line.BeginReading(), url.BeginReading()));
          } else {
            NS_ERROR("Found Begin record without URL");
          }          
          break;
        }
          
        // Handle End records
        case 'E': {
          // Chop off the E. Rest is the index.
          line.Cut(0,1);
          
          // Remove the index from the map.
          // This URL was completed correctly.
          hashSuccess = indexToURLMap.Get(line, nsnull);
          if (hashSuccess) {
            TRACE(("sbMetadataCrashTracker::ProcessExistingLog " \
                   "- found End record '%s'", line.BeginReading()));
            indexToURLMap.Remove(line);
          } else {
            NS_ERROR("Found End record without matching Begin");
          }
          break;
        }
          
        default:
          NS_ERROR("Invalid sbMetadataCrashTracker log file");
      }
    }
  } while (NS_SUCCEEDED(rv) && more);
  inputStream->Close();
  
  // Anything left over in the map must have been running during
  // the crash.  Add to the blacklist!
  if (indexToURLMap.Count() > 0) {
    indexToURLMap.EnumerateRead(AddURLsToBlacklist, &mURLBlacklist);
    rv = WriteBlacklist();
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("sbMetadataCrashTracker::ProcessExistingLog - " \
            " found %d suspect URLs", indexToURLMap.Count()));
  } else {
    TRACE(("sbMetadataCrashTracker::ProcessExistingLog - no suspect URLs"));
  }
  
  // Ok, we've processed the log file, so throw it away.
  mLogFile->Remove(PR_FALSE);
  
  return rv;
}

/* static */ PLDHashOperator PR_CALLBACK
sbMetadataCrashTracker::AddURLsToBlacklist(nsCStringHashKey::KeyType aKey,
                                           nsCString aEntry,
                                           void* aUserData)
{
  if (aEntry.IsEmpty()) {
    NS_ERROR("Attempted to add an empty string to the blacklist");
    return PL_DHASH_NEXT;
  }
  nsDataHashtable<nsCStringHashKey, PRBool>* blacklistHash =
      static_cast<nsDataHashtable<nsCStringHashKey, PRBool>* >(aUserData);
  NS_ENSURE_TRUE(blacklistHash, PL_DHASH_STOP);

  blacklistHash->Put(aEntry, PR_TRUE);
  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbMetadataCrashTracker::WriteBlacklistURLToFile(nsCStringHashKey::KeyType aKey,
                                                PRBool aEntry,
                                                void* aUserData)
{
  nsresult rv = NS_OK;
  if (aKey.IsEmpty()) {
    NS_ERROR("Attempted to add an empty string to the blacklist file");
    return PL_DHASH_NEXT;
  }
  nsIOutputStream* outputStream =
      static_cast<nsIOutputStream* >(aUserData);
  NS_ENSURE_TRUE(outputStream, PL_DHASH_STOP);

  nsCString output(aKey);
  output.AppendLiteral("\n");

  PRUint32 bytesWritten;
  rv = outputStream->Write(output.BeginReading(), 
                           output.Length(), 
                           &bytesWritten);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  
  return PL_DHASH_NEXT;
}

nsresult 
sbMetadataCrashTracker::ReadBlacklist()
{
  NS_ENSURE_STATE(mBlacklistFile);
  nsresult rv = NS_OK;

  // If no blacklist, don't bother reading
  PRBool exists = PR_FALSE;
  rv = mBlacklistFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    return NS_OK;
  }

  // Open the blacklist
  nsCOMPtr<nsIInputStream> inputStream;
  rv = NS_NewLocalFileInputStream(getter_AddRefs(inputStream), mBlacklistFile);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsILineInputStream> lineStream(do_QueryInterface(inputStream, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool more = PR_TRUE;
  nsCString line;
  
  // Skip the first line, as it should be a text description
  rv = lineStream->ReadLine(line, &more);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(more, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(line.First() == '#', NS_ERROR_UNEXPECTED);
  
  // Read the URLs from the blacklist file into mURLBlacklist
  do {
    rv = lineStream->ReadLine(line, &more);
    if (NS_SUCCEEDED(rv) && !line.IsEmpty()) {
      PRBool blacklisted = PR_TRUE;
      mURLBlacklist.Put(line, blacklisted);     
      LOG(("sbMetadataCrashTracker::ReadBlacklist() - found %s", 
           line.BeginReading()));
    }
  } while (NS_SUCCEEDED(rv) && more);

  inputStream->Close();

  return rv;
}

nsresult 
sbMetadataCrashTracker::WriteBlacklist()
{
  NS_ENSURE_STATE(mBlacklistFile);
  nsresult rv = NS_OK;
    
  // Open/Create file
  nsCOMPtr<nsIFileOutputStream> fileStream =
          do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 ioFlags = PR_TRUNCATE | PR_CREATE_FILE | PR_WRONLY; 
  rv = fileStream->Init(mBlacklistFile, ioFlags, -1, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIOutputStream> outputStream = do_QueryInterface(fileStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add an explanatory message on the first line
  nsCString output("# URLs listed in this file are suspected of " \
                   "crashing Songbird, and will be ignored.\n"); 

  PRUint32 bytesWritten;
  rv = outputStream->Write(output.BeginReading(), 
                           output.Length(), 
                           &bytesWritten);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Now add all blacklisted URLs, one per line.
  mURLBlacklist.EnumerateRead(WriteBlacklistURLToFile, outputStream);
  
  outputStream->Close();  
  return rv;
}

nsresult 
sbMetadataCrashTracker::GetProfileFile(const nsAString& aName, nsIFile** aFile)
{
  NS_ENSURE_ARG_POINTER(aFile);
  nsresult rv = NS_OK;
  
  // Get the directory service
  nsCOMPtr<nsIProperties> directoryService(
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the user's profile directory
  nsCOMPtr<nsIFile> file;
  rv = directoryService->Get("ProfD", NS_GET_IID(nsIFile),
                             getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the requested filename
  rv = file->Append(aName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  file.forget(aFile);
  return NS_OK;
}
