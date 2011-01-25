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

#include "sbiTunesImporterStatus.h"

#include <nsIProxyObjectManager.h>

#include <sbProxiedComponentManager.h>
#include <sbStringBundle.h>

#include "sbiTunesImporterJob.h"

sbiTunesImporterStatus::sbiTunesImporterStatus(sbiTunesImporterJob * aJobProgress) :
  mDone(PR_FALSE),
  mJobProgress(aJobProgress),
  mProgress(0),
  mTotalSet(PR_FALSE) {

}

sbiTunesImporterStatus::~sbiTunesImporterStatus() {
  // TODO Auto-generated destructor stub
}

sbiTunesImporterStatus *
sbiTunesImporterStatus::New(sbiTunesImporterJob * aJobProgress) {
  return new sbiTunesImporterStatus(aJobProgress);
}

PRBool sbiTunesImporterStatus::CancelRequested() {
  // If there's no job progress, then we've been canceled
  PRBool canceled = PR_TRUE;
  if (mJobProgress) {
    canceled = mJobProgress->CancelRequested();
  }
  return canceled;
}

nsresult sbiTunesImporterStatus::Initialize() {
  
  nsresult rv;
  
  mProgress = 0;
  // Create and proxy the status data remote object
  mStatusDataRemote = do_CreateInstance(SB_DATAREMOTE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mStatusDataRemote->Init(NS_LITERAL_STRING("faceplate.status.text"), nsString()); 
  NS_ENSURE_SUCCESS(rv, rv);


  if (mJobProgress) {
    sbStringBundle bundle;
    nsTArray<nsString> params;
    
    nsString * newElement = params.AppendElement(NS_LITERAL_STRING("iTunes"));
    NS_ENSURE_TRUE(newElement, NS_ERROR_OUT_OF_MEMORY);
    
    nsString titleText = bundle.Format(NS_LITERAL_STRING("import_library.job.title_text"),
                                  params);
    rv = mJobProgress->SetTitleText(titleText);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mJobProgress->SetStatusText(SBLocalizedString("import_library.job.status_text"));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mJobProgress->SetStatus(sbIJobProgress::STATUS_RUNNING);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

void sbiTunesImporterStatus::Finalize() {
  mStatusDataRemote = nsnull;
  mJobProgress = nsnull;
}

nsresult sbiTunesImporterStatus::Reset() {
  mStatusText.Truncate();
  mDone = PR_FALSE;
  return NS_OK;
}

void sbiTunesImporterStatus::SetProgress(PRInt64 aProgress) {
  mProgress = (PRUint32)((aProgress * 100L) / mProgressMax);
  Update();
}

void sbiTunesImporterStatus::SetProgressMax(PRInt64 aProgressMax) {
  mProgressMax = aProgressMax;
}

void sbiTunesImporterStatus::SetStatusText(nsAString const & aMsg) {
  mStatusText = aMsg;
}

nsresult sbiTunesImporterStatus::Update() {
  nsresult rv;
  
  // Check if finalized so just return an error
  if (!mStatusDataRemote || !mJobProgress) { 
    return NS_ERROR_FAILURE;
  }
  nsString status(mStatusText);
  // Round the value
  if (!mLastStatusText.Equals(mStatusText) || mLastProgress != mProgress) {    
    if (!mDone) {
      status.AppendLiteral(" ");
      status.AppendInt(mProgress, 10);
      status.AppendLiteral("%");
    }
    rv = mStatusDataRemote->SetStringValue(status);
    NS_ENSURE_SUCCESS(rv, rv);
    if (mJobProgress) {
      if (mLastProgress != mProgress) {
        rv = mJobProgress->SetProgress(mProgress);
        NS_ENSURE_SUCCESS(rv, rv);
  
        rv = mJobProgress->SetTotal(100);
        NS_ENSURE_SUCCESS(rv, rv);
      }      
    }
    if (mDone) {
      rv = mJobProgress->SetStatus(sbIJobProgress::STATUS_SUCCEEDED);
    }
    mLastProgress = mProgress;
    mLastStatusText = mStatusText;
  }
  return NS_OK;
}

