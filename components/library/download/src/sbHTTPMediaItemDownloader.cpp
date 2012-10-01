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
// HTTP media item downloader.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbHTTPMediaItemDownloader.cpp
 * \brief HTTP Media Item Downloader Source.
 */

//------------------------------------------------------------------------------
//
// HTTP media item downloader imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbHTTPMediaItemDownloader.h"

// Local imports.
#include "sbHTTPMediaItemDownloadJob.h"

// Songbird imports.
#include <sbFileUtils.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsIChannel.h>
#include <nsIIOService.h>
#include <nsIPropertyBag2.h>
#include <nsIURI.h>


//------------------------------------------------------------------------------
//
// HTTP media item downloader logging services.
//
//------------------------------------------------------------------------------

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbHTTPMediaItemDownloader:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gHTTPMediaItemDownloaderLog = nsnull;
#define TRACE(args) PR_LOG(gHTTPMediaItemDownloaderLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gHTTPMediaItemDownloaderLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// HTTP media item downloader nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbHTTPMediaItemDownloader,
                              sbIMediaItemDownloader)


//------------------------------------------------------------------------------
//
// HTTP media item downloader sbIMediaItemDownloader implementation.
//
// \see sbIMediaItemDownloader
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// vote
//
// \see sbIMediaItemDownloader
//

NS_IMETHODIMP
sbHTTPMediaItemDownloader::Vote(sbIMediaItem* aMediaItem,
                                sbILibrary*   aTargetLibrary,
                                PRUint32*     retval)
{
  TRACE(("%s 0x%08x 0x%08x", __FUNCTION__, aMediaItem, aTargetLibrary));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(retval);

  // Function variables.
  nsresult rv;

  // Get the content source on the main thread.
  nsCOMPtr<nsIURI> contentSrc;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentSrc));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIURI>
    mainThreadContentSrc = do_MainThreadQueryInterface(contentSrc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the content source scheme.
  nsCAutoString scheme;
  rv = mainThreadContentSrc->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for a scheme match.
  if (scheme.Equals("http") || scheme.Equals("https"))
    *retval = sbIMediaItemDownloader::VOTE_SCHEME;
  else
    *retval = sbIMediaItemDownloader::VOTE_NONE;

  return NS_OK;
}


//-------------------------------------
//
// getDownloadSize
//
// \see sbIMediaItemDownloader
//

NS_IMETHODIMP
sbHTTPMediaItemDownloader::GetDownloadSize(sbIMediaItem* aMediaItem,
                                           sbILibrary*   aTargetLibrary,
                                           PRUint64*     retval)
{
  TRACE(("%s 0x%08x 0x%08x", __FUNCTION__, aMediaItem, aTargetLibrary));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Get the content source.
  nsCOMPtr<nsIURI> contentSrc;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentSrc));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the IO service.
  nsCOMPtr<nsIIOService>
    ioService = do_ProxiedGetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a new channel on the main thread.
  nsCOMPtr<nsIChannel> channel;
  rv = ioService->NewChannelFromURI(contentSrc, getter_AddRefs(channel));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIChannel>
    mainThreadChannel = do_MainThreadQueryInterface(channel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Open the channel.
  nsCOMPtr<nsIInputStream> inputStream;
  rv = mainThreadChannel->Open(getter_AddRefs(inputStream));
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoInputStream autoInputStream(inputStream);

  // Get the content length.
  PRUint64 contentLength = 0;
  bool   gotContentLength = PR_FALSE;
  nsCOMPtr<nsIPropertyBag2>
    properties = do_MainThreadQueryInterface(mainThreadChannel, &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = properties->GetPropertyAsUint64(NS_LITERAL_STRING("content-length"),
                                         &contentLength);
    if (NS_SUCCEEDED(rv))
      gotContentLength = PR_TRUE;
  }
  if (!gotContentLength) {
    PRInt32 contentLength32;
    rv = mainThreadChannel->GetContentLength(&contentLength32);
    NS_ENSURE_SUCCESS(rv, rv);
    contentLength = contentLength32;
  }

  // Return results.
  *retval = contentLength;

  return NS_OK;
}


//-------------------------------------
//
// createDownloadJob
//
// \see sbIMediaItemDownloader
//

NS_IMETHODIMP
sbHTTPMediaItemDownloader::CreateDownloadJob
                             (sbIMediaItem*             aMediaItem,
                              sbILibrary*               aTargetLibrary,
                              sbIMediaItemDownloadJob** retval)
{
  TRACE(("%s 0x%08x 0x%08x", __FUNCTION__, aMediaItem, aTargetLibrary));

  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Create an HTTP media item download job.
  nsRefPtr<sbHTTPMediaItemDownloadJob> job;
  rv = sbHTTPMediaItemDownloadJob::New(getter_AddRefs(job),
                                       aMediaItem,
                                       aTargetLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  job.forget(retval);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// HTTP media item downloader public services.
//
//------------------------------------------------------------------------------

// Component registration implementation.
SB_MEDIA_ITEM_DOWNLOADER_REGISTERSELF_IMPL(sbHTTPMediaItemDownloader)


//-------------------------------------
//
// sbHTTPMediaItemDownloader
//

sbHTTPMediaItemDownloader::sbHTTPMediaItemDownloader()
{
  // Initialize logging.
#ifdef PR_LOGGING
  if (!gHTTPMediaItemDownloaderLog) {
    gHTTPMediaItemDownloaderLog =
      PR_NewLogModule("sbHTTPMediaItemDownloader");
  }
#endif

  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


//-------------------------------------
//
// ~sbHTTPMediaItemDownloader
//

sbHTTPMediaItemDownloader::~sbHTTPMediaItemDownloader()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
}

