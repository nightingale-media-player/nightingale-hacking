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

#include "sbPlaybackHistoryEntry.h"

#include <nsIClassInfoImpl.h>
#include <nsIMutableArray.h>
#include <nsIProgrammingLanguage.h>
#include <nsIStringEnumerator.h>

#include <nsArrayUtils.h>
#include <mozilla/Mutex.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>

#include <sbIPlaybackHistoryService.h>

#include <sbPropertiesCID.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbPlaybackHistoryEntry, 
                              sbIPlaybackHistoryEntry)

sbPlaybackHistoryEntry::sbPlaybackHistoryEntry()
: mLock("sbPlaybackHistoryEntry::mLock")
, mEntryId(-1)
, mTimestamp(0)
, mDuration(0)
{
  MOZ_COUNT_CTOR(sbPlaybackHistoryEntry);
}

sbPlaybackHistoryEntry::~sbPlaybackHistoryEntry()
{
  MOZ_COUNT_DTOR(sbPlaybackHistoryEntry);
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::GetEntryId(PRInt64 *aEntryId)
{
  NS_ENSURE_ARG_POINTER(aEntryId);
  
  mozilla::MutexAutoLock lock(mLock);
  *aEntryId = mEntryId;

  return NS_OK;
}

void
sbPlaybackHistoryEntry::SetEntryId(PRInt64 aEntryId)
{
	mozilla::MutexAutoLock lock(mLock);
  
  if(mEntryId != -1)
    return;

  mEntryId = aEntryId;

  return;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::GetItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  mozilla::MutexAutoLock lock(mLock);
  NS_IF_ADDREF(*aItem = mItem);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::GetTimestamp(PRInt64 *aTimestamp)
{
  NS_ENSURE_ARG_POINTER(aTimestamp);

  mozilla::MutexAutoLock lock(mLock);
  *aTimestamp = mTimestamp;

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::GetDuration(PRInt64 *aDuration)
{
  NS_ENSURE_ARG_POINTER(aDuration);

  mozilla::MutexAutoLock lock(mLock);
  *aDuration = mDuration;

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::GetAnnotations(sbIPropertyArray * *aAnnotations)
{
  NS_ENSURE_ARG_POINTER(aAnnotations);

  mozilla::MutexAutoLock lock(mLock);
  NS_IF_ADDREF(*aAnnotations = mAnnotations);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::GetAnnotation(const nsAString & aAnnotationId, 
                                      nsAString & _retval)
{
  _retval.Truncate();

  mozilla::MutexAutoLock lock(mLock);

  if(!mAnnotations)
    return NS_OK;

  nsresult rv = mAnnotations->GetPropertyValue(aAnnotationId, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::HasAnnotation(const nsAString & aAnnotationId, 
                                      PRBool *_retval)
{
  *_retval = PR_FALSE;

  mozilla::MutexAutoLock lock(mLock);

  if(!mAnnotations)
    return NS_OK;

  nsString value;
  nsresult rv = mAnnotations->GetPropertyValue(aAnnotationId, value);
  
  if(NS_SUCCEEDED(rv))
    *_retval = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::SetAnnotation(const nsAString & aAnnotationId, 
                                      const nsAString & aAnnotationValue)
{
  mozilla::MutexAutoLock lock(mLock);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIMutablePropertyArray> annotations;

  if(!mAnnotations) {
    annotations = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mAnnotations = do_QueryInterface(annotations, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    annotations = do_QueryInterface(mAnnotations, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = annotations->AppendProperty(aAnnotationId, aAnnotationValue);
  NS_ENSURE_SUCCESS(rv, rv);

  // The entry exists in the history service so we can actually set the
  // annotation. If the entry doesn't exist yet, the annotations will 
  // automatically be set when the entry is added to the history.
  if(mEntryId != -1) {
    nsCOMPtr<sbIPlaybackHistoryService> history = 
      do_GetService(SB_PLAYBACKHISTORYSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = history->AddOrUpdateAnnotation(mEntryId, 
                                        aAnnotationId, 
                                        aAnnotationValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaybackHistoryEntry::RemoveAnnotation(const nsAString &aAnnotationId)
{
  mozilla::MutexAutoLock lock(mLock);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> annotations;

  if(!mAnnotations) {
    annotations = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mAnnotations = do_QueryInterface(annotations, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    annotations = do_QueryInterface(mAnnotations, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 length = 0;
  rv = annotations->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIProperty> property = 
      do_QueryElementAt(annotations, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString id;
    rv = property->GetId(id);
    NS_ENSURE_SUCCESS(rv, rv);

    if(aAnnotationId.Equals(id)) {
      rv = annotations->RemoveElementAt(current);
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }
  }

  if(mEntryId != -1) {
    nsCOMPtr<sbIPlaybackHistoryService> history = 
      do_GetService(SB_PLAYBACKHISTORYSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = history->RemoveAnnotation(mEntryId, aAnnotationId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryEntry::Init(sbIMediaItem *aItem, 
                             PRInt64 aTimestamp, 
                             PRInt64 aDuration, 
                             sbIPropertyArray *aAnnotations)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_MIN(aTimestamp, 0);
  NS_ENSURE_ARG_MIN(aDuration, 0);

  mozilla::MutexAutoLock lock(mLock);

  mItem = aItem;
  mTimestamp = aTimestamp;
  mDuration = aDuration;
  mAnnotations = aAnnotations;

  return NS_OK;
}
