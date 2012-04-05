/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird media item download services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMediaItemDownloadService.cpp
 * \brief Songbird Media Item Download Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird media item download service imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbMediaItemDownloadService.h"

// Songbird imports.
#include <sbIMediaItemDownloader.h>
#include <sbXPCOMUtils.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>

#include <nsIObserverService.h>


//------------------------------------------------------------------------------
//
// Songbird media item download service logging services.
//
//------------------------------------------------------------------------------

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaItemDownloadService:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaItemDownloadServiceLog = nsnull;
#define TRACE(args) PR_LOG(gMediaItemDownloadServiceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaItemDownloadServiceLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// Songbird media item download service nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMediaItemDownloadService,
                              sbIMediaItemDownloadService,
                              nsIObserver)


//------------------------------------------------------------------------------
//
// Songbird media item download service sbIMediaItemDownloadService
// implementation.
//
// \see sbIMediaItemDownloadService
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// getDownloader
//
// \see sbIMediaItemDownloadService
//

NS_IMETHODIMP
sbMediaItemDownloadService::GetDownloader
                              (sbIMediaItem*            aMediaItem,
                               sbILibrary*              aTargetLibrary,
                               sbIMediaItemDownloader** retval)
{
  TRACE(("%s 0x%08x 0x%08x", __FUNCTION__, aMediaItem, aTargetLibrary));

  // Validate state and arguments.
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(retval);

  // Function variables.
  nsresult rv;

  // Search for the downloader with the highest vote.
  nsCOMPtr<sbIMediaItemDownloader> highestVoteDownloader;
  PRUint32 highestVote = sbIMediaItemDownloader::VOTE_NONE;
  PRUint32 downloaderCount = mDownloaderList.Length();
  for (PRUint32 i = 0; i < downloaderCount; ++i) {
    // Get the next downloader's vote.
    sbIMediaItemDownloader* downloader = mDownloaderList[i];
    PRUint32 vote;
    rv = downloader->Vote(aMediaItem, aTargetLibrary, &vote);
    NS_ENSURE_SUCCESS(rv, rv);

    // Update the highest vote downloader.
    if (vote > highestVote) {
      highestVoteDownloader = downloader;
      highestVote = vote;
    }
  }

  // Return downloader.
  highestVoteDownloader.forget(retval);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird media item download service nsIObserver implementation.
//
// \see nsIObserver
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Observer
//
// \see nsIObserver
//

NS_IMETHODIMP
sbMediaItemDownloadService::Observe(nsISupports*     aSubject,
                                    const char*      aTopic,
                                    const PRUnichar* aData)
{
  if (aData) {
    TRACE(("%s[%.8x] \"%s\" \"%s\"", __FUNCTION__,
                                     this,
                                     aTopic,
                                     NS_ConvertUTF16toUTF8(aData).get()));
  }
  else {
    TRACE(("%s[%.8x] \"%s\"", __FUNCTION__, this, aTopic));
  }

  nsresult rv;

  // Dispatch processing of event.
  if (!strcmp(aTopic, "app-startup")) {
    nsCOMPtr<nsIObserverService> obsSvc = do_GetService(
            "@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(this,
                             "profile-after-change",
                             PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!strcmp(aTopic, "profile-after-change")) {
    // Initialize the component.
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);

    // Now remove the listener, since it's no longer needed.
    nsCOMPtr<nsIObserverService> obsSvc = do_GetService(
            "@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(this,
                                "profile-after-change");
    NS_ENSURE_SUCCESS(rv, rv);

  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird media item download public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbMediaItemDownloadService
//

sbMediaItemDownloadService::sbMediaItemDownloadService() :
  mInitialized(PR_FALSE)
{
  // Initialize logging.
#ifdef PR_LOGGING
  if (!gMediaItemDownloadServiceLog) {
    gMediaItemDownloadServiceLog =
      PR_NewLogModule("sbMediaItemDownloadService");
  }
#endif

  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


//-------------------------------------
//
// ~sbMediaItemDownloadService
//

sbMediaItemDownloadService::~sbMediaItemDownloadService()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Finalize the media item download services.
  Finalize();
}


//------------------------------------------------------------------------------
//
// Songbird media item download private services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Initialize
//

nsresult
sbMediaItemDownloadService::Initialize()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Function variables.
  nsresult rv;

  // Do nothing if already initialized.
  if (mInitialized)
    return NS_OK;

  // Get the service manager.
  mServiceManager = do_GetService(SB_SERVICE_MANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the list of media item downloader components.
  nsTArray<nsCString> entryValueList;
  rv = SB_GetCategoryEntryValues(SB_MEDIA_ITEM_DOWNLOADER_CATEGORY,
                                 entryValueList);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add all of the media item downloaders.
  for (PRUint32 i = 0; i < entryValueList.Length(); ++i) {
    // Get the next downloader.
    nsCOMPtr<sbIMediaItemDownloader>
      downloader = do_CreateInstance(entryValueList[i].get(), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add the downloader.
    nsCOMPtr<sbIMediaItemDownloader>*
      appendedElement = mDownloaderList.AppendElement(downloader);
    NS_ENSURE_TRUE(appendedElement, NS_ERROR_OUT_OF_MEMORY);
  }

  // Services are now initialized.
  mInitialized = PR_TRUE;

  // Indicate that the media item download services are ready.
  rv = mServiceManager->SetServiceReady
                          (SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CONTRACTID, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// Finalize
//

void
sbMediaItemDownloadService::Finalize()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  if (mInitialized) {
    // Indicate that the media item download services are no longer ready.
    mServiceManager->SetServiceReady(SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CONTRACTID,
                                     PR_FALSE);

    // Clear the media item downloader list.
    mDownloaderList.Clear();

    // No longer initialized.
    mInitialized = PR_FALSE;
  }
}
