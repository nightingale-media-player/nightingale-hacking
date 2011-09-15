/* vim: set sw=2 :miv */
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

#ifndef SBDEVICEENSURESPACEFORWRITE_H_
#define SBDEVICEENSURESPACEFORWRITE_H_

#include <map>
#include <vector>

#include "sbBaseDevice.h"

/**
 * This is a helper class that looks at the changeset and determines how many
 * items will fit in the free space of the device. The changeset passed to the
 * constructor is modified, removing changes that dont fit.
 */
class sbDeviceEnsureSpaceForWrite
{
public:

  /**
   * Initializes the object with the device and changeset to be analyzed
   * \param aDevice     The device the changeset belongs to
   * \param aDevLibrary The device's library
   * \param aChangeset  The changeset that we're checking for available space
   */
  sbDeviceEnsureSpaceForWrite(sbBaseDevice * aDevice,
                              sbIDeviceLibrary * aDevLibrary,
                              sbILibraryChangeset * aChangeset);
  /**
   * Cleanup data
   */
  ~sbDeviceEnsureSpaceForWrite();

  /**
   * Ensures there's enough space for items in the changeset and removes any items
   * that don't fit.
   */
  nsresult EnsureSpace();
private:

  /**
   * non-owning reference back to our device
   */
  sbBaseDevice * mDevice;

  /**
   * The device library
   */
  nsCOMPtr<sbIDeviceLibrary> mDevLibrary;

  /**
   * The changeset we're operating on
   */
  nsCOMPtr<sbILibraryChangeset> mChangeset;
  /**
   * Total length in bytes of the items
   */
  PRInt64 mTotalLength;
  /**
   * Free space on the device
   */
  PRInt64 mFreeSpace;

  /**
   * Gets the free space for the device
   */
  nsresult GetFreeSpace();

  /**
   * Removes the extra items from the changeset
   */
  nsresult RemoveExtraItems();
};

#endif /* SBDEVICEENSURESPACEFORWRITE_H_ */
