/* vim: set sw=2 :miv */
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
#include "sbTranscodeProgressListener.h"

// Mozilla includes
#include <nsIWritablePropertyBag2.h>

// Songbird includes
#include <sbIJobCancelable.h>
#include <sbIJobProgress.h>
#include <sbIDeviceEvent.h>
#include <sbIMediacoreEventListener.h>
#include <sbIMediacoreEvent.h>
#include <sbIMediacoreError.h>
#include <sbITranscodeError.h>
#include <sbStandardProperties.h>
#include <sbStatusPropertyValue.h>
#include <sbTranscodeUtils.h>
#include <sbVariantUtils.h>
// Local includes
#include "sbDeviceStatusHelper.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(sbTranscodeProgressListener,
                              sbIJobProgressListener,
                              sbIMediacoreEventListener)

sbTranscodeProgressListener *
sbTranscodeProgressListener::New(sbBaseDevice * aDeviceBase,
                                 sbDeviceStatusHelper * aDeviceStatusHelper,
                                 sbIMediaItem *aItem,
                                 PRMonitor * aCompleteNotifyMonitor,
                                 StatusProperty const & aStatusProperty,
                                 sbIJobCancelable * aCancel) {
  return new sbTranscodeProgressListener(aDeviceBase,
                                         aDeviceStatusHelper,
                                         aItem,
                                         aCompleteNotifyMonitor,
                                         aStatusProperty,
                                         aCancel);
}

sbTranscodeProgressListener::sbTranscodeProgressListener(
  sbBaseDevice * aDeviceBase,
  sbDeviceStatusHelper * aDeviceStatusHelper,
  sbIMediaItem *aItem,
  PRMonitor * aCompleteNotifyMonitor,
  StatusProperty const & aStatusProperty,
  sbIJobCancelable * aCancel) :
    mBaseDevice(aDeviceBase),
    mStatus(aDeviceStatusHelper),
    mItem(aItem),
    mCompleteNotifyMonitor(aCompleteNotifyMonitor),
    mIsComplete(0),
    mTotal(0),
    mStatusProperty(aStatusProperty),
    mCancel(aCancel),
    mAborted(PR_FALSE) {

  NS_ASSERTION(mBaseDevice,
               "sbTranscodeProgressListener mBaseDevice can't be null");
  NS_ASSERTION(mStatus,
               "sbTranscodeProgressListener mStatus can't be null");
  NS_ASSERTION(mItem,
               "sbTranscodeProgressListener mItem can't be null");
  NS_ASSERTION(mCompleteNotifyMonitor,
               "sbTranscodeProgressListener mCompleteNotifyMonitor "
               "can't be null");

  NS_IF_ADDREF(static_cast<sbIDevice*>(mBaseDevice));
}

sbTranscodeProgressListener::~sbTranscodeProgressListener() {
  sbIDevice * disambiguate = mBaseDevice;
  NS_IF_RELEASE(disambiguate);
}

nsresult
sbTranscodeProgressListener::Completed(sbIJobProgress * aJobProgress) {
  nsresult rv;

  // Indicate that the transcode operation is complete.  If a complete notify
  // monitor was provided, operate under the monitor and send completion
  // notification.
  if (mCompleteNotifyMonitor) {
    nsAutoMonitor monitor(mCompleteNotifyMonitor);
    PR_AtomicSet(&mIsComplete, 1);
    monitor.Notify();
  } else {
    PR_AtomicSet(&mIsComplete, 1);
  }

  // Disconnect us now that we're done.
  // This should be where we die as well, as this will be the last reference
  rv = aJobProgress->RemoveJobProgressListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mCancel = nsnull;

  return NS_OK;
}

nsresult
sbTranscodeProgressListener::SetProgress(sbIJobProgress * aJobProgress) {
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aJobProgress);

  if (!mTotal) {
    rv = aJobProgress->GetTotal(&mTotal);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 progress;
  rv = aJobProgress->GetProgress(&progress);
  NS_ENSURE_SUCCESS(rv, rv);

  double percentComplete = 0.0;
  if (mTotal > 0) {
    percentComplete = static_cast<double>(progress) /
                      static_cast<double>(mTotal);
  }
  mStatus->ItemProgress(percentComplete);

  sbStatusPropertyValue value;
  double const complete = percentComplete * 100.0;
  value.SetMode(sbStatusPropertyValue::eRipping);
  value.SetCurrent((PRUint32)complete);
  SetStatusProperty(value);

  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProgressListener::OnJobProgress(sbIJobProgress *aJobProgress)
{
  NS_ENSURE_ARG_POINTER(aJobProgress);

  // Cancel job if it needs to be aborted.  Continue processing OnJobProgress
  // calls while the cancel operation is performed.  Make sure cancel is only
  // called once.  Calling cancel can result in a reentrant call to
  // OnJobProgress.
  if (!mAborted &&
      mCancel &&
      mBaseDevice->IsRequestAborted()) {
    mAborted = PR_TRUE;
    nsCOMPtr<sbIJobCancelable> cancel = mCancel;
    mCancel = nsnull;
    cancel->Cancel();

    nsresult rv;
    sbStatusPropertyValue value;
    value.SetMode(sbStatusPropertyValue::eAborted);
    rv = SetStatusProperty(value);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = Completed(aJobProgress);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  PRUint16 status;
  nsresult rv = aJobProgress->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);
  switch (status) {
    case sbIJobProgress::STATUS_SUCCEEDED: {
      sbStatusPropertyValue value;
      value.SetMode(sbStatusPropertyValue::eComplete);
      SetStatusProperty(value);

      rv = Completed(aJobProgress);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIJobProgress::STATUS_FAILED: {
      sbStatusPropertyValue value;
      value.SetMode(sbStatusPropertyValue::eFailed);
      SetStatusProperty(value);

      rv = Completed(aJobProgress);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    case sbIJobProgress::STATUS_RUNNING: {
      rv = SetProgress(aJobProgress);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    default: {
      NS_WARNING("Unexpected progress state in "
                 "sbTranscodeProgressListener::OnJobProgress");
    }
    break;
  }

  return NS_OK;
}

inline nsresult
sbTranscodeProgressListener::SetStatusProperty(
                                           sbStatusPropertyValue const & aValue)
{
  nsresult rv = mStatusProperty.SetValue(aValue.GetValue());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbTranscodeProgressListener::OnMediacoreEvent(sbIMediacoreEvent *aEvent)
{
  NS_ENSURE_STATE(mItem);
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv;
  PRUint32 type;

  rv = aEvent->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (type == sbIMediacoreEvent::ERROR_EVENT) {
    nsCOMPtr<sbIMediacoreError> error;
    rv = aEvent->GetError(getter_AddRefs(error));
    NS_ENSURE_SUCCESS(rv, rv);

    // Dispatch the device event
    nsCOMPtr<nsIWritablePropertyBag2> bag =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString message;
    rv = error->GetMessage(message);
    if (NS_SUCCEEDED(rv)) {
      rv = bag->SetPropertyAsAString(NS_LITERAL_STRING("message"),
                                     message);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("mediacore-error"),
                                     NS_ISUPPORTS_CAST(sbIMediacoreError*, error));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString srcUri;
    rv = mItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), srcUri);
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<sbITranscodeError> transcodeError;
      rv = SB_NewTranscodeError(message, message, SBVoidString(),
                                srcUri,
                                nsnull,
                                getter_AddRefs(transcodeError));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = transcodeError->SetDestItem(mItem);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = bag->SetPropertyAsInterface(NS_LITERAL_STRING("transcode-error"),
                                       NS_ISUPPORTS_CAST(sbITranscodeError*, transcodeError));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mBaseDevice->CreateAndDispatchEvent(
          sbIDeviceEvent::EVENT_DEVICE_TRANSCODE_ERROR,
          sbNewVariant(bag));
  }

  return NS_OK;
}

