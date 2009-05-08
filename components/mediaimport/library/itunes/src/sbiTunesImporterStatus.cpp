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
    
    mJobProgress->SetStatusText(SBLocalizedString("import_library.job.status_text"));
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

void sbiTunesImporterStatus::SetProgress(PRUint32 aProgress) {
  mProgress = aProgress;
  Update();
}

void sbiTunesImporterStatus::SetProgressMax(PRUint32 aProgressMax) {
  mProgressMax = aProgressMax;
}

void sbiTunesImporterStatus::SetStatusText(nsAString const & aMsg) {
  mStatusText = aMsg;
}

nsresult sbiTunesImporterStatus::Update() {
  nsresult rv;
  
  nsString status(mStatusText);
#if WHEN_PROGRESS_WORKS  
  // Calc the 0 to 100 progress value
  double const progress = static_cast<double>(mProgress) * 100.0 / 
                              static_cast<double>(mProgressMax);
#endif                              
  // Round the value
  PRUint32 progressAsInt = mProgress; // progress + 0.5;
  if (!mLastStatusText.Equals(mStatusText) || mLastProgress + 10 < progressAsInt) {    
    if (!mDone) {
      status.AppendLiteral(" ");
      status.AppendInt(progressAsInt, 10);
      // XXX l10n TODO: If we can't do % progress internationalize
      status.AppendLiteral(" tracks processed (l10n this text)");
    }
    rv = mStatusDataRemote->SetStringValue(status);
    NS_ENSURE_SUCCESS(rv, rv);
#if WHEN_PROGRESS_WORKS    
    if (mJobProgress) {
      if (mLastProgress != progressAsInt) {
        mLastProgress = progressAsInt;
        rv = mJobProgress->SetProgress(progressAsInt);
        NS_ENSURE_SUCCESS(rv, rv);
  
        rv = mJobProgress->SetTotal(100);
        NS_ENSURE_SUCCESS(rv, rv);
      }      
    }
#endif    
    if (mDone) {
      rv = mJobProgress->SetStatus(sbIJobProgress::STATUS_SUCCEEDED);
    }
    mLastProgress = progressAsInt;
    mLastStatusText = mStatusText;
  }
  return NS_OK;
}

