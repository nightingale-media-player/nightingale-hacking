/*
 //
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbiTunesImporterJob.h"

#include <nsThreadUtils.h>

#include <sbTArrayStringEnumerator.h>
#include <sbDebugUtils.h>

NS_IMPL_THREADSAFE_ISUPPORTS2(sbiTunesImporterJob, 
                              sbIJobProgress,
                              sbIJobCancelable)

sbiTunesImporterJob::sbiTunesImporterJob() : 
  mCanCancel(PR_TRUE),
  mCancelRequested(PR_FALSE),
  mStatus(0) {
}

sbiTunesImporterJob::~sbiTunesImporterJob() {
}

sbiTunesImporterJob * sbiTunesImporterJob::New() {
  return new sbiTunesImporterJob;
}

nsresult
sbiTunesImporterJob::SetStatus(PRUint32 aStatus) {
  mStatus = aStatus;

  nsresult rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult 
sbiTunesImporterJob::SetStatusText(nsAString const & aStatusText) {
  mStatusText = aStatusText;

  nsresult rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult 
sbiTunesImporterJob::SetTitleText(nsAString const & aTitleText) {
  mTitleText = aTitleText;

  nsresult rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult 
sbiTunesImporterJob::SetProgress(PRUint32 aProgress) {
  mProgress = aProgress;

  nsresult rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult 
sbiTunesImporterJob::SetTotal(PRUint32 aTotal) {
  mTotal = aTotal;

  nsresult rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult
sbiTunesImporterJob::AddErrorMessage(nsAString const & aErrorMessage) {
  mErrorMessages.AppendElement(aErrorMessage);

  nsresult rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP 
sbiTunesImporterJob::GetStatus(PRUint16 *aStatus)
{
  *aStatus = mStatus;
  return NS_OK;
}

NS_IMETHODIMP
sbiTunesImporterJob::GetBlocked(PRBool *aBlocked)
{
  NS_ENSURE_ARG_POINTER(aBlocked);
  *aBlocked = PR_FALSE;
  return NS_OK;
}

/* readonly attribute AString statusText; */
NS_IMETHODIMP 
sbiTunesImporterJob::GetStatusText(nsAString & aStatusText)
{
  aStatusText = mStatusText;
  return NS_OK;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP 
sbiTunesImporterJob::GetTitleText(nsAString & aTitleText)
{
  aTitleText = mTitleText;
  return NS_OK;
}

/* readonly attribute unsigned long progress; */
NS_IMETHODIMP 
sbiTunesImporterJob::GetProgress(PRUint32 *aProgress)
{
  *aProgress = mProgress;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP 
sbiTunesImporterJob::GetTotal(PRUint32 *aTotal)
{
  *aTotal = mTotal;
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP 
sbiTunesImporterJob::GetErrorCount(PRUint32 *aErrorCount)
{
  *aErrorCount = mErrorMessages.Length();
  return NS_OK;
}

/**
 * This is thread safe as sbTArrayStringEnumerator creates a copy in it's 
 * constructor and does not reference the original array after that
 *  nsIStringEnumerator getErrorMessages (); 
 */
NS_IMETHODIMP 
sbiTunesImporterJob::GetErrorMessages(nsIStringEnumerator ** aEnumerator)
{
  *aEnumerator = sbTArrayStringEnumerator::New(mErrorMessages);
  NS_ENSURE_TRUE(*aEnumerator, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aEnumerator);
  return NS_OK;
}

/* void addJobProgressListener (in sbIJobProgressListener aListener); */
NS_IMETHODIMP 
sbiTunesImporterJob::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  if (mListeners.IndexOfObject(aListener) == -1) {
    PRBool const inserted = mListeners.AppendObject(aListener);
    NS_ENSURE_TRUE(inserted, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_WARNING("sbiTunesImporterJob::AddJobProgressListener listener already added");
  }
  return NS_OK;
}

/* void removeJobProgressListener (in sbIJobProgressListener aListener); */
NS_IMETHODIMP 
sbiTunesImporterJob::RemoveJobProgressListener(sbIJobProgressListener *aListener)
{
  PRInt32 index = mListeners.IndexOfObject(aListener);
  mListeners.RemoveObjectAt(index);
  return NS_OK;
}

NS_IMETHODIMP 
sbiTunesImporterJob::GetCanCancel(PRBool *aCanCancel)
{
  *aCanCancel = mCanCancel; 
  return NS_OK;
}

/* void cancel (); */
NS_IMETHODIMP
sbiTunesImporterJob::Cancel()
{
  mCancelRequested = PR_TRUE;
  return NS_OK;
}

nsresult
sbiTunesImporterJob::UpdateProgress() {
  PRUint32 const listenerCount = mListeners.Count();
  for (PRUint32 index = 0; index < listenerCount; ++index) {
    nsresult SB_UNUSED_IN_RELEASE(rv) = mListeners[index]->OnJobProgress(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "iTunes Import listener reported error");
  }
  return NS_OK;
}
