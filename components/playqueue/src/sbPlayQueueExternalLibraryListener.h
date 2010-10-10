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

#ifndef __SBPLAYQUEUEEXTERNALLIBRARYLISTENER_H__
#define __SBPLAYQUEUEEXTERNALLIBRARYLISTENER_H__

/**
 * \file sbPlayQueueExternalLibraryListener.h
 * \brief Helper class to listen to external libraries and sync properties.
 */

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsVoidArray.h>

#include <prlock.h>

#include <sbILibrary.h>
#include <sbIMediaListListener.h>
#include <sbIPropertyArray.h>

#include <list>

/** 
 * \brief Class to hold onto property updates to make sure we're not applying
 * them more than once.
 */

class sbPropertyUpdate {
public:
  sbPropertyUpdate(sbIMediaItem* aItem, sbIPropertyArray* aUpdate)
  : mItem(aItem),
    mUpdate(aUpdate)
  {}

  bool operator==(sbPropertyUpdate rhs);

  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIPropertyArray> mUpdate;
};

/** 
 * \brief Class to allow items in a "master" internal library to stay in sync
 * with duplicates in other libraries.
 */

class sbPlayQueueExternalLibraryListener : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbPlayQueueExternalLibraryListener();

  /** 
   * \brief Set the master library. Items that are in this library with
   * duplicates in external libraries, and items in external libraries with
   * duplicates in this library, will be kept in sync.
   */
  nsresult SetMasterLibrary(sbILibrary* aLibrary);

  /** 
   * \brief Add an external library to listen to.
   */
  nsresult AddExternalLibrary(sbILibrary* aLibrary);

  /** 
   * \brief Set properties on an item, but don't synchronize to duplicates in
   * other libraries.
   */
  nsresult SetPropertiesNoSync(sbIMediaItem* aMediaItem,
                               sbIPropertyArray* aProperties);

  /** 
   * \brief Removes all listeners so the libraries can shut down.
   */
  nsresult RemoveListeners();

private:
  virtual ~sbPlayQueueExternalLibraryListener();

  /** 
   * \brief Updates that are currently being handled, and therefore should be
   * ignored.
   */
  typedef std::list<sbPropertyUpdate> Updates;
  typedef std::list<sbPropertyUpdate>::iterator UpdateIter;
  Updates mUpdates;

  /** 
   * \brief Internal method to generate updates that need to be applied. Called
   * from OnItemUpdated.
   */
  nsresult GenerateUpdates(sbIMediaItem* aMediaItem,
                           sbIPropertyArray* aProperties,
                           Updates& updates);

  /** 
   * \brief Lock to make sure two libraries aren't modifying our update list.
   */
  PRLock* mUpdateLock;

  /** 
   * \brief The master library.
   */
  nsCOMPtr<sbILibrary> mMasterLibrary;

  /** 
   * \brief Array of external libraries to listen to.
   */
  nsCOMArray<sbILibrary> mExternalLibraries;
};

#endif /* __SBPLAYQUEUEEXTERNALLIBRARYLISTENER_H__ */
