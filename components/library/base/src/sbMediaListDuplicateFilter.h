/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#ifndef SBMEDIALISTDUPLICATEFILTER_H_
#define SBMEDIALISTDUPLICATEFILTER_H_

// Mozilla includes
#include <mozilla/Mutex.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsISimpleEnumerator.h>
#include <nsTArray.h>
#include <nsTHashtable.h>

// Songbird includes
#include <sbIMediaListDuplicateFilter.h>
#include <sbIMediaListListener.h>
#include <sbIPropertyArray.h>

// forward declarations
class sbIMediaItem;

/**
 * This class is an enumerator that filters out duplicates.
 * Initialize it with a source and destination then enumerate
 * and the class will enumerate over the source, minus the 
 * duplicates if indicated. It keeps track of the duplicates
 * and total item counts as well
 */
class sbMediaListDuplicateFilter : public sbIMediaListDuplicateFilter,
                                   public nsISimpleEnumerator,
                                   public sbIMediaListEnumerationListener
{
  NS_DECL_ISUPPORTS;
  NS_DECL_NSISIMPLEENUMERATOR;
  NS_DECL_SBIMEDIALISTDUPLICATEFILTER;
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER;

  sbMediaListDuplicateFilter();
  virtual ~sbMediaListDuplicateFilter();
private:
  /**
   * Saves the key values for aItem
   */
  nsresult SaveItemKeys(sbIMediaItem * aItem);
  /**
   * Determines if aItem is a duplicate of any item in the destination
   */
  nsresult IsDuplicate(sbIMediaItem * aItem, bool & aIsDuplicate);
  /**
   * Advances our enumerator, skipping duplicates if needed
   */
  nsresult Advance();

  // Monitor for thread-safety
  PRMonitor* mMonitor;

  // Have we run enumerate items yet?
  PRBool mInitialized;

  // Contains a combination origin ID and URL's
  nsTHashtable<nsStringHashKey> mKeys;

  // Prop Keys Length to avoid recalculating the length of the array.
  PRUint32 mSBPropKeysLength;
  // List of properties to use in duplicate identification
  nsTArray<nsString> mSBPropKeys;
  // List of properties to use when calling GetProperties
  nsCOMPtr<sbIPropertyArray> mSBPropertyArray;
  // List of properties for the current item we're processing
  nsCOMPtr<sbIPropertyArray> mItemProperties;

  // The enumerate for the source
  nsCOMPtr<nsISimpleEnumerator> mSource;

  // The destionation
  nsCOMPtr<sbIMediaList> mDest;

  // Current item in the enumeration
  nsCOMPtr<sbIMediaItem> mCurrentItem;

  // Total duplicate items found
  PRUint32 mDuplicateItems;

  // Total items enumerated
  PRUint32 mTotalItems;

  // Indicates whether to remove duplicates from the enumeration
  PRBool mRemoveDuplicates;
};

#endif
