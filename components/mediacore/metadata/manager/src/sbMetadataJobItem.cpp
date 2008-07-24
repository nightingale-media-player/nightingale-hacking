/* vim: set sw=2 :miv */
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
 * \file sbMetadataJobItem.cpp
 * \brief Implementation of sbMetadataJobItem.h
 */

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include "prlog.h"
#include "sbMetadataJobItem.h"

// DEFINES ====================================================================
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#define TRACE(args) PR_LOG(gMetadataLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS0(sbMetadataJobItem);

sbMetadataJobItem::sbMetadataJobItem(sbMetadataJob::JobType aJobType, 
                                     sbIMediaItem* aMediaItem, 
                                     sbMetadataJob* aOwningJob) :
  mJobType(aJobType),
  mMediaItem(aMediaItem),
  mHandler(nsnull),
  mOwningJob(aOwningJob),
  mProcessingComplete(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbMetadataJobItem);
  TRACE(("sbMetadataJobItem[0x%.8x] - ctor", this));
}

sbMetadataJobItem::~sbMetadataJobItem()
{
  MOZ_COUNT_DTOR(sbMetadataJobItem);
  TRACE(("sbMetadataJobItem[0x%.8x] - dtor", this));
}

nsresult sbMetadataJobItem::GetHandler(sbIMetadataHandler** aHandler) 
{
  NS_ENSURE_ARG_POINTER(aHandler);
  if (!mHandler) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  NS_ADDREF(*aHandler = mHandler);
  return NS_OK;
}

nsresult sbMetadataJobItem::SetHandler(sbIMetadataHandler* aHandler) 
{
  mHandler = aHandler;
  return NS_OK;
}

nsresult sbMetadataJobItem::GetMediaItem(sbIMediaItem** aMediaItem) 
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_STATE(mMediaItem);
  NS_ADDREF(*aMediaItem = mMediaItem);
  return NS_OK;
}

nsresult sbMetadataJobItem::GetOwningJob(sbMetadataJob** aJob) 
{
  NS_ENSURE_ARG_POINTER(aJob);
  NS_ENSURE_STATE(mOwningJob);
  NS_ADDREF(*aJob = mOwningJob); 
  return NS_OK;
}

nsresult sbMetadataJobItem::GetJobType(sbMetadataJob::JobType* aJobType)
{
  NS_ENSURE_ARG_POINTER(aJobType);
  *aJobType = mJobType;
  return NS_OK;
}

nsresult sbMetadataJobItem::GetProcessed(PRBool* aProcessed)
{
  NS_ENSURE_ARG_POINTER(aProcessed);
  *aProcessed = mProcessingComplete;
  return NS_OK;
}

nsresult sbMetadataJobItem::SetProcessed(PRBool aProcessed)
{
  mProcessingComplete = aProcessed;
  return NS_OK;
}

nsresult sbMetadataJobItem::GetURL(nsACString& aURL) 
{
  aURL.Assign(mURL);
  return NS_OK;
}
nsresult sbMetadataJobItem::SetURL(const nsACString& aURL) 
{
  mURL.Assign(aURL);
  return NS_OK;
}

