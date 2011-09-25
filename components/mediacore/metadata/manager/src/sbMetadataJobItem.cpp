/* vim: set sw=2 :miv */
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
 * \file sbMetadataJobItem.cpp
 * \brief Implementation of sbMetadataJobItem.h
 */

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsComponentManagerUtils.h>
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

#define SB_MUTABLEPROPERTYARRAY_CONTRACTID "@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1"

// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS0(sbMetadataJobItem);

sbMetadataJobItem::sbMetadataJobItem(sbMetadataJob::JobType aJobType, 
                                     sbIMediaItem* aMediaItem,
                                     nsStringArray* aRequiredProperties, 
                                     sbMetadataJob* aOwningJob) :
  mJobType(aJobType),
  mMediaItem(aMediaItem),
  mHandler(nsnull),
  mOwningJob(aOwningJob),
  mPropertyList(aRequiredProperties),
  mProcessingStarted(PR_FALSE),
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

nsresult sbMetadataJobItem::GetProcessingStarted(PRBool* aProcessingStarted)
{
  NS_ENSURE_ARG_POINTER(aProcessingStarted);
  *aProcessingStarted = mProcessingStarted;
  return NS_OK;
}

nsresult sbMetadataJobItem::SetProcessingStarted(PRBool aProcessingStarted)
{
  mProcessingStarted = aProcessingStarted;
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

nsresult sbMetadataJobItem::GetProperties(sbIMutablePropertyArray** aPropertyArray)
{
  NS_ENSURE_ARG_POINTER(aPropertyArray);
  NS_ENSURE_STATE(mMediaItem);
  nsresult rv;

  // Create a property array we can put the properties we want into.
  nsCOMPtr<sbIMutablePropertyArray> writeProps =
                    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now grab all the properties from the item.
  nsCOMPtr<sbIPropertyArray> propArray;
  rv = mMediaItem->GetProperties(nsnull, getter_AddRefs(propArray));
  NS_ENSURE_SUCCESS(rv, rv);

  // We have to make sure that any properties we wanted that are missing from
  // propArray get added with empty strings to ensure we can erase values.
  nsCOMPtr<sbIProperty> property;
  nsString propertyId;
  nsString propertyValue;
  for (PRInt32 current = 0; current < mPropertyList->Count(); ++current) {
    // Get the wanted property
    mPropertyList->StringAt(current, propertyId);
    
    // Get the value for this property and if null make an empty string
    rv = propArray->GetPropertyValue(propertyId, propertyValue);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      // This is a null value or does not exist so we should write out an
      // empty string
      propertyValue = EmptyString();
      propertyValue.SetIsVoid(PR_TRUE);
    } else {
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    rv = writeProps->AppendProperty(propertyId, propertyValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  NS_ADDREF(*aPropertyArray = writeProps);
  return NS_OK;
}
