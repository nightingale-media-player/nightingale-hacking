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
 * \file sbMetadataJobItem.h
 * \brief A metadata job task
 */

#ifndef SBMETADATAJOBITEM_H_
#define SBMETADATAJOBITEM_H_

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsISupports.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <sbIMetadataHandler.h>
#include <sbIPropertyArray.h>
#include "sbMetadataJob.h"

// CLASSES ====================================================================

class sbIMediaItem;

/**
 * \class sbMetadataJobItem
 *
 * Data container representing a single task in an sbFileMetdataService request
 *
 * Held by an sbMetadataJob in a waiting queue, then taken by a job processor 
 * which runs the sbIMetadataHandler and then returns the item to the job.
 * 
 * Instances of this class are not to be shared between threads.
 * Ownership should be transfered between objects at various
 * stages of the job lifecycle.
 */
class sbMetadataJobItem : public nsISupports
{
public:

  NS_DECL_ISUPPORTS

  sbMetadataJobItem(sbMetadataJob::JobType aJobType, 
                    sbIMediaItem* aMediaItem,
                    nsTArray<nsString>* aRequiredProperties,
                    sbMetadataJob* aOwningJob);
  virtual ~sbMetadataJobItem();
  
  nsresult GetHandler(sbIMetadataHandler** aHandler);
  nsresult SetHandler(sbIMetadataHandler* aHandler);
  nsresult GetMediaItem(sbIMediaItem** aMediaItem);
  nsresult GetOwningJob(sbMetadataJob** aJob);
  nsresult GetJobType(sbMetadataJob::JobType* aJobType);  
  nsresult GetProcessingStarted(PRBool* aProcessingStarted);
  nsresult SetProcessingStarted(PRBool aProcessingStarted);
  nsresult GetProcessed(PRBool* aProcessed);
  nsresult SetProcessed(PRBool aProcessed);
  nsresult GetURL(nsACString& aURL);
  nsresult SetURL(const nsACString& aURL);
  nsresult GetProperties(sbIMutablePropertyArray** aPropertyArray);

protected:
  sbMetadataJob::JobType             mJobType;
  nsCOMPtr<sbIMediaItem>             mMediaItem;
  nsCOMPtr<sbIMetadataHandler>       mHandler;
  nsRefPtr<sbMetadataJob>            mOwningJob;
  
  nsCString                          mURL;
  nsTArray<nsString>*                mPropertyList;

  // Flag to indicate that a handler was started for this item
  PRBool                             mProcessingStarted;

  // Flag to indicate that a handler was run for this item
  PRBool                             mProcessingComplete;
};

#endif // SBMETADATAJOBITEM_H_
