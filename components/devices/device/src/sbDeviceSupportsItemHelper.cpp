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

#include "sbDeviceSupportsItemHelper.h"

#include <sbIDevice.h>
#include <sbIMediaInspector.h>
#include <sbIMediaItem.h>

#include <nsNetError.h> // for NS_ERROR_IN_PROGRESS

#include "sbDeviceUtils.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceSupportsItemHelper,
                              sbIJobProgressListener)

sbDeviceSupportsItemHelper::sbDeviceSupportsItemHelper()
  : mDevice(nsnull)
{
  /* member initializers and constructor code */
}

sbDeviceSupportsItemHelper::~sbDeviceSupportsItemHelper()
{
  if (mDevice) {
    NS_ISUPPORTS_CAST(sbIDevice*, mDevice)->Release();
    mDevice = nsnull;
  }
}

/***** sbIJobProgressListener *****/
/* void onJobProgress (in sbIJobProgress aJobProgress); */
NS_IMETHODIMP
sbDeviceSupportsItemHelper::OnJobProgress(sbIJobProgress *aJobProgress)
{
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aJobProgress);
  NS_ENSURE_STATE(mCallback);

  PRUint16 status;
  rv = aJobProgress->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);
  switch (status) {
  case sbIJobProgress::STATUS_SUCCEEDED: {
    nsCOMPtr<sbIMediaFormat> mediaFormat;
    rv = mInspector->GetMediaFormat(getter_AddRefs(mediaFormat));
    NS_ENSURE_SUCCESS(rv, rv);
    bool needsTranscoding;
    rv = sbDeviceUtils::DoesItemNeedTranscoding(mTranscodeType,
                                                mediaFormat,
                                                mDevice,
                                                needsTranscoding);

    PRBool supported = (NS_SUCCEEDED(rv) && !needsTranscoding);
    mCallback->OnSupportsMediaItem(mItem, supported); // ignore results
    break;
  }
  case sbIJobProgress::STATUS_FAILED:
    // failed to get the format for the item; assume not supported
    mCallback->OnSupportsMediaItem(mItem, PR_FALSE); // ignore results
    break;
  default:
    // incomplete, keep going
    return NS_OK;
  }

  // cleanup

  rv = aJobProgress->RemoveJobProgressListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/***** sbDeviceSupportsItemHelper *****/
nsresult
sbDeviceSupportsItemHelper::Init(sbIMediaItem* aItem,
                                 sbBaseDevice* aDevice,
                                 sbIDeviceSupportsItemCallback* aCallback)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aCallback);

  mItem = aItem;
  mCallback = aCallback;
  mDevice = aDevice;
  NS_ISUPPORTS_CAST(sbIDevice*, mDevice)->AddRef();
  return NS_OK;
}


nsresult
sbDeviceSupportsItemHelper::InitJobProgress(sbIMediaInspector* aInspector,
                                            PRUint32 aTranscodeType)
{
  NS_ENSURE_ARG_POINTER(aInspector);

  nsresult rv;

  mInspector = aInspector;
  mTranscodeType = aTranscodeType;

  nsCOMPtr<sbIJobProgress> progress = do_QueryInterface(aInspector, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = progress->AddJobProgressListener(this);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

void
sbDeviceSupportsItemHelper::RunSupportsMediaItem()
{
  nsresult rv;
  PRBool result;
  rv = mDevice->SupportsMediaItem(mItem, this, PR_FALSE, &result);
  if (rv != NS_ERROR_IN_PROGRESS) {
    // this was synchronous, notify the callback
    if (NS_FAILED(rv)) {
      result = PR_FALSE;
    }
    mCallback->OnSupportsMediaItem(mItem, result);
  }
}

