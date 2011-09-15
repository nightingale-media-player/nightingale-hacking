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
// HTTP media item download job.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbHTTPMediaItemDownloadJob.cpp
 * \brief HTTP Media Item Download Job Source.
 */

//------------------------------------------------------------------------------
//
// HTTP media item download job imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbHTTPMediaItemDownloadJob.h"

// Mozilla imports.
#include <nsAutoPtr.h>


//------------------------------------------------------------------------------
//
// HTTP media item download job logging services.
//
//------------------------------------------------------------------------------

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbHTTPMediaItemDownloadJob:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gHTTPMediaItemDownloadJobLog = nsnull;
#define TRACE(args) PR_LOG(gHTTPMediaItemDownloadJobLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gHTTPMediaItemDownloadJobLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// HTTP media item download job nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS_INHERITED0(sbHTTPMediaItemDownloadJob,
                             sbBaseMediaItemDownloadJob)


//------------------------------------------------------------------------------
//
// HTTP media item download job public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// New
//

/* static */ nsresult
sbHTTPMediaItemDownloadJob::New(sbHTTPMediaItemDownloadJob** aJob,
                                sbIMediaItem*                aMediaItem,
                                sbILibrary*                  aTargetLibrary)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aJob);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Function variables.
  nsresult rv;

  // Create and initialize a new HTTP media item download job.
  nsRefPtr<sbHTTPMediaItemDownloadJob>
    job = new sbHTTPMediaItemDownloadJob(aMediaItem, aTargetLibrary);
  NS_ENSURE_TRUE(job, NS_ERROR_OUT_OF_MEMORY);
  rv = job->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  job.forget(aJob);

  return NS_OK;
}


//-------------------------------------
//
// ~sbHTTPMediaItemDownloadJob
//

sbHTTPMediaItemDownloadJob::~sbHTTPMediaItemDownloadJob()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


//------------------------------------------------------------------------------
//
// HTTP media item download job private services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbHTTPMediaItemDownloadJob
//

sbHTTPMediaItemDownloadJob::sbHTTPMediaItemDownloadJob
                              (sbIMediaItem* aMediaItem,
                               sbILibrary*   aTargetLibrary) :
  sbBaseMediaItemDownloadJob(aMediaItem, aTargetLibrary)
{
  // Initialize logging.
#ifdef PR_LOGGING
  if (!gHTTPMediaItemDownloadJobLog) {
    gHTTPMediaItemDownloadJobLog =
      PR_NewLogModule("sbHTTPMediaItemDownloadJob");
  }
#endif

  TRACE(("%s[%.8x]", __FUNCTION__, this));
}

