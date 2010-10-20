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

#ifndef __SBLIBRARYLISTENERHELPERS__H__
#define __SBLIBRARYLISTENERHELPERS__H__

#include "sbIDeviceLibrary.h"
#include "sbILocalDatabaseSimpleMediaList.h"
#include "sbIMediaListListener.h"
#include <sbStandardProperties.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsWeakReference.h>

class sbIDevice;
class sbBaseDevice;

/**
 * This class provides the common ignore logic for listener helpers
 */
class sbBaseIgnore
{
public:
  /**
   * Ignores all items. This obviously will supersede item specific ignores
   */
  nsresult SetIgnoreListener(PRBool aIgnoreListener);
  /**
   * Sets an ignore for a given item. Returns PR_FALSE if the item is already
   * being ignored
   */
  nsresult IgnoreMediaItem(sbIMediaItem * aItem);
  /**
   * Resumes listening for an item if it has been ignored
   */
  nsresult UnignoreMediaItem(sbIMediaItem * aItem);
protected:
  /**
   * Returns PR_TRUE if the item is currently being ignored
   */
  PRBool MediaItemIgnored(sbIMediaItem * aItem);
  /**
   * Initializes the lock and ignore listener count
   */
  sbBaseIgnore() : mLock(nsAutoLock::NewLock("sbBaseIgnore::mLock")),
                   mIgnoreListenerCounter(0) {
    mIgnored.Init();
    NS_ASSERTION(mLock, "Failed to allocate sbBaseIgnore::mLock");
  }
  /**
   * Destroys the lock and various other cleanup
   */
  ~sbBaseIgnore() {
    nsAutoLock::DestroyLock(mLock);
    mLock = nsnull;
  }
private:
  nsDataHashtable<nsStringHashKey,PRInt32> mIgnored;
  PRLock * mLock;
  PRInt32 mIgnoreListenerCounter;
};

class sbBaseDeviceLibraryListener : public sbIDeviceLibraryListener,
                                    public nsSupportsWeakReference,
                                    public sbBaseIgnore
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICELIBRARYLISTENER

  sbBaseDeviceLibraryListener();
  virtual ~sbBaseDeviceLibraryListener();

  nsresult Init(sbBaseDevice* aDevice);

  void Destroy();

protected:
  // The device owns the listener, so use a non-owning reference here
  sbBaseDevice* mDevice;
};

class sbDeviceBaseLibraryCopyListener : public sbILocalDatabaseMediaListCopyListener,
                                        public sbBaseIgnore
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEMEDIALISTCOPYLISTENER

  sbDeviceBaseLibraryCopyListener();
  virtual ~sbDeviceBaseLibraryCopyListener();

  nsresult Init(sbBaseDevice* aDevice);

protected:
  // The device owns the listener, so use a non-owning reference here
  sbBaseDevice* mDevice;
};

class sbBaseDeviceMediaListListener : public sbIMediaListListener,
                                      public sbBaseIgnore
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbBaseDeviceMediaListListener();

  nsresult Init(sbBaseDevice* aDevice);

protected:
  virtual ~sbBaseDeviceMediaListListener();

  // The device owns the listener, so use a non-owning reference here
  sbBaseDevice* mDevice;
};

#endif
