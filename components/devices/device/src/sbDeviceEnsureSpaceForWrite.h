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
 * This is a helper class that looks at the batch and determines how many
 * items will fit in the free space of the device. The batch passed to the
 * constructor is modified, with items that fail to fit being removed.
 */
class sbDeviceEnsureSpaceForWrite
{
public:
  // Typedefs for types from sbBaseDevice
  typedef sbBaseDevice::TransferRequest TransferRequest;
  typedef sbBaseDevice::Batch Batch;
  /**
   * Holds the length and indexes back to the batch for the item
   */
  struct BatchLink {
    BatchLink() : mLength(0) {}
    BatchLink(PRInt32 aOrder,
              PRInt64 aLength,
              PRUint32 aIndex) : mOrder(aOrder), mLength(aLength) {
      mBatchIndexes.push_back(aIndex);
    }
    PRInt32 mOrder;
    PRInt64 mLength;
    typedef std::vector<PRUint32> BatchIndexes;
    BatchIndexes mBatchIndexes;
  };

  typedef std::map<sbIMediaItem*, BatchLink> ItemsToWrite;

  /**
   * Initializes the object with the device and batch to be analyzed
   * \param aDevice The device the batch belongs to
   * \param aBatch The batch that we'er checking for available space
   */
  sbDeviceEnsureSpaceForWrite(sbBaseDevice * aDevice,
                              Batch & aBatch);
  /**
   * Cleanup data
   */
  ~sbDeviceEnsureSpaceForWrite();

  /**
   * Ensures there's enough space for items in the batch and removes any items
   * that don't fit.
   */
  nsresult EnsureSpace();
private:
  /**
   * Our write mode
   */
  enum WriteMode {
    NOP,
    SHUFFLE,
    MANUAL,
    ABORT
  };

  struct RemoveItemInfo
  {
    RemoveItemInfo(sbIMediaItem * aItem, sbIMediaList * aList) : mItem(aItem),
                                                                 mList(aList) {}
    nsRefPtr<sbIMediaItem> mItem;
    nsRefPtr<sbIMediaList> mList;
  };

  /**
   * Simple list of non-owning media item pointers
   */
  typedef std::vector<sbIMediaItem*> ItemList;

  /**
   * Collection items to be removed and their list
   */
  typedef std::list<RemoveItemInfo> RemoveItems;

  /**
   * non-owning reference back to our device
   */
  sbBaseDevice * mDevice;
  /**
   * Collection of media item pointers
   */
  ItemsToWrite mItemsToWrite;
  /**
   * The batch we're operating on
   */
  Batch & mBatch;
  /**
   * The owning library of the items
   */
  nsCOMPtr<sbIDeviceLibrary> mOwnerLibrary;
  /**
   * Total length in bytes of the items
   */
  PRInt64 mTotalLength;
  /**
   * Free space on the device
   */
  PRInt64 mFreeSpace;

  /**
   * Builds a list of sbIMediaItem pointers mItemsToWrite
   */
  nsresult BuildItemsToWrite();

  /**
   * Adds an item to mItemsToWrite
   */
  nsresult AddItemToWrite(TransferRequest * aRequest,
                          PRUint32 aIndex,
                          PRUint32 & aOrder);

  /**
   * Creates an item list
   */
  void CreateItemList(ItemList & aItems);

  /**
   * Gets the free space for the device
   */
  nsresult GetFreeSpace();

  /**
   * Determines the write mode for our device
   */
  nsresult GetWriteMode(WriteMode & aWriteMode);

  /**
   * Removes the extra items from the batch
   */
  nsresult RemoveExtraItems();

  /**
   * Removes the items picked to be removed from the device library
   */
  nsresult RemoveItemsFromLibrary(RemoveItems::iterator iter,
                                  RemoveItems::iterator end);
};

#endif /* SBDEVICEENSURESPACEFORWRITE_H_ */

