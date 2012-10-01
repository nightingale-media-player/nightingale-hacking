/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#include "sbDeviceEnsureSpaceForWrite.h"

// Local includes
#include "sbDeviceUtils.h"

// Standard includes
#include <algorithm>
#include <set>
#include <ctime>

// Mozilla interfaces
#include <nsIPropertyBag2.h>
#include <nsIVariant.h>

// Mozilla includes
#include <nsArrayUtils.h>

// Songbird interfaces
#include <sbIDeviceEvent.h>
#include <sbIDeviceProperties.h>

// Songbird includes
#include <sbLibraryUtils.h>
#include <sbStandardDeviceProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>
#include <sbILibraryChangeset.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseDevice:5
 */
#ifdef PR_LOGGING
extern PRLogModuleInfo* gBaseDeviceLog;
#endif

#undef LOG
#define LOG(args) PR_LOG(gBaseDeviceLog, PR_LOG_WARN, args)

sbDeviceEnsureSpaceForWrite::sbDeviceEnsureSpaceForWrite(
  sbBaseDevice * aDevice,
  sbIDeviceLibrary * aDevLibrary,
  sbILibraryChangeset * aChangeset) :
    mDevice(aDevice),
    mDevLibrary(aDevLibrary),
    mChangeset(aChangeset),
    mTotalLength(0),
    mFreeSpace(0) {
}

sbDeviceEnsureSpaceForWrite::~sbDeviceEnsureSpaceForWrite() {
}

nsresult
sbDeviceEnsureSpaceForWrite::GetFreeSpace() {
  nsresult rv;
  nsAutoString freeSpaceStr;
  rv = mDevLibrary->GetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_FREE_SPACE),
                                freeSpaceStr);
  NS_ENSURE_SUCCESS(rv, rv);
  mFreeSpace = nsString_ToInt64(freeSpaceStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // apply limit to the total space available for music
  PRInt64 freeMusicSpace;
  rv = mDevice->GetMusicFreeSpace(mDevLibrary, &freeMusicSpace);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mFreeSpace >= freeMusicSpace)
    mFreeSpace = freeMusicSpace;

  return NS_OK;
}

nsresult
sbDeviceEnsureSpaceForWrite::RemoveExtraItems() {
  nsresult rv;

  /* We'll go through the changes and separate into ADDEDs and 'others', which
   * should be MODIFIEDs.  We'll handle the 'others' first to ensure that
   * things like modifications are all done before additions to the device */
  nsCOMPtr<nsIMutableArray> addChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> otherChanges =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get all of the changes in our changeset
  nsCOMPtr<nsIArray> changes;
  rv = mChangeset->GetChanges(getter_AddRefs(changes));
  NS_ENSURE_SUCCESS(rv, rv);

  /* Loop over the changes in the changeset, separate into ADDED and 'other',
   * and calculate how much space all of the changes will take */
  PRUint32 length;
  rv = changes->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt64 totalChangeSize = 0;
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbILibraryChange> change = do_QueryElementAt(changes, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 operation;
    rv = change->GetOperation(&operation);
    NS_ENSURE_SUCCESS(rv, 0);

    // Separate changes into ADDED and other
    if (operation == sbIChangeOperation::ADDED) {
      addChanges->AppendElement(change, PR_FALSE);
    }
    else {
      otherChanges->AppendElement(change, PR_FALSE);
    }

    // Keep track of how much space all of these changes will take
    PRInt64 changeSize = mDevice->GetChangeSize(mDevLibrary, change);
    totalChangeSize += changeSize;
  }

  // Check if the changes will fit on the device as is
  if (totalChangeSize >= mFreeSpace) {
    // not everything fits, see what the user wants to do
    if (!mDevice->GetEnsureSpaceChecked()) {
      bool abort;
      rv = sbDeviceUtils::QueryUserSpaceExceeded(mDevice,
                                                 mDevLibrary,
                                                 totalChangeSize,
                                                 mFreeSpace,
                                                 &abort);
      NS_ENSURE_SUCCESS(rv, rv);
      if (abort) {
        // If the user aborts, we bail
        return NS_ERROR_ABORT;
      }

      /* If the user does not abort, we log that he/she has agreed to the
       * EnsureSpaceCheck and continue to find out which changes will fit */
      mDevice->SetEnsureSpaceChecked(true);
    }
  }
  else {
    // everything fits, we're good
    return NS_OK;
  }

  // an array of the changes we'll actually complete
  nsCOMPtr<nsIMutableArray> changesToTake =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  PRInt64 changesToTakeSize = 0;

  /* We attempt to add the 'other', non-ADDED, changes first because they will
   * likely not take much space or, in the case of DELETEs, add free space to
   * the device whereas all ADDED changes will reduce the free space
   * significantly */
  PRUint32 otherChangesLength;
  rv = otherChanges->GetLength(&otherChangesLength);
  for (PRUint32 i = 0; i < otherChangesLength; i++) {
    nsCOMPtr<sbILibraryChange> change = do_QueryElementAt(otherChanges, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* See how big the change is, if it will fit, add it to the list of changes
     * that we'll actually perform */
    PRInt64 changeSize = mDevice->GetChangeSize(mDevLibrary, change);
    if ((changesToTakeSize + changeSize) < mFreeSpace) {
      changesToTake->AppendElement(change, PR_FALSE);
      changesToTakeSize += changeSize;
    }
  }

  // Seed our random number generator
  time_t t;
  time(&t);
  srand((unsigned int) t);

  // Now we consider our ADDED changes in random order
  PRUint32 addChangesLength;
  rv = addChanges->GetLength(&addChangesLength);
  while (addChangesLength != 0) {
    // Get a semi-random number between 0 and addChangesLength
    PRUint32 random = (unsigned int) ((rand() / (RAND_MAX + 1.0)) * addChangesLength);

    nsCOMPtr<sbILibraryChange> change = do_QueryElementAt(addChanges, random, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    /* We are considering this change currently, so remove it from our list
     * of ADDED changes to be considered. */
    addChanges->RemoveElementAt(random);
    addChangesLength--;

    /* See how big the change is, if it will fit, add it to the list of changes
     * that we'll actually perform */
    PRInt64 changeSize = mDevice->GetChangeSize(mDevLibrary, change);
    if ((changesToTakeSize + changeSize) < mFreeSpace) {
      changesToTake->AppendElement(change, PR_FALSE);
      changesToTakeSize += changeSize;
    }
  }

  /* Set the changes of the changeset we are considering to the ones we selected
   * that will fit on the device. */
  rv = mChangeset->SetChanges(changesToTake);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbDeviceEnsureSpaceForWrite::EnsureSpace() {
  // First find out how much room we have to use
  nsresult rv = GetFreeSpace();
  NS_ENSURE_SUCCESS(rv, rv);

  // Then find out which of the changes we are considering will fit
  rv = RemoveExtraItems();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
