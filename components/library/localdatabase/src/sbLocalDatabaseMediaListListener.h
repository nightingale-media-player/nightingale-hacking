/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __SB_LOCALDATABASE_MEDIALISTLISTENER_H__
#define __SB_LOCALDATABASE_MEDIALISTLISTENER_H__

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsTArray.h>
#include <prlock.h>
#include <sbIMediaListListener.h>
#include <sbIPropertyArray.h>

class nsIEventTarget;
class nsISupportsHashKey;
class nsIWeakReference;
class sbIMediaItem;
class sbIMediaList;

class sbListenerInfo
{
friend class sbWeakMediaListListenerWrapper;
friend class sbLocalDatabaseMediaListListener;
public:
  sbListenerInfo();
  ~sbListenerInfo();

  nsresult Init(sbIMediaListListener* aListener);
  nsresult Init(nsIWeakReference* aWeakListener);
private:

  PRBool mIsGone;
  nsCOMPtr<nsISupports> mRef;
  nsCOMPtr<sbIMediaListListener> mProxy;
};

class sbWeakMediaListListenerWrapper : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbWeakMediaListListenerWrapper(sbListenerInfo* aListenerInfo);
  ~sbWeakMediaListListenerWrapper();

private:
  already_AddRefed<sbIMediaListListener> GetListener();

  sbListenerInfo* mListenerInfo;
};

class sbLocalDatabaseMediaListListener
{
  friend class sbAutoBatchHelper;

  typedef nsCOMArray<sbIMediaListListener> sbMediaListListenersArray;
  typedef nsAutoPtr<sbListenerInfo> sbListenerInfoAutoPtr;

public:
  sbLocalDatabaseMediaListListener();
  ~sbLocalDatabaseMediaListListener();

protected:
  // Initialize the instance.
  nsresult Init();

  // Add a listener to the array
  nsresult AddListener(sbIMediaListListener* aListener, PRBool aOwnsWeak);

  // Remove a listener from the array
  nsresult RemoveListener(sbIMediaListListener* aListener);

  // Return the number of listeners
  PRUint32 ListenerCount();

  // Enumerate listeners and call OnItemAdded
  void NotifyListenersItemAdded(sbIMediaList* aList,
                                sbIMediaItem* aItem);

  // Enumerate listeners and call OnBeforeItemRemoved
  void NotifyListenersBeforeItemRemoved(sbIMediaList* aList,
                                        sbIMediaItem* aItem);

  // Enumerate listeners and call OnAfterItemRemoved
  void NotifyListenersAfterItemRemoved(sbIMediaList* aList,
                                       sbIMediaItem* aItem);

  // Enumerate listeners and call OnItemRemoved
  void NotifyListenersItemUpdated(sbIMediaList* aList,
                                  sbIMediaItem* aItem,
                                  sbIPropertyArray* aProperties);

  // Enumerate listeners and call OnListCleared
  void NotifyListenersListCleared(sbIMediaList* aList);

  // Enumerate listeners and call OnBatchBegin
  void NotifyListenersBatchBegin(sbIMediaList* aList);

  // Enumerate listeners and call OnBatchEnd
  void NotifyListenersBatchEnd(sbIMediaList* aList);

private:
  nsresult SnapshotListenerArray(sbMediaListListenersArray& aArray);
  void SweepListenerArray();

  nsTArray<sbListenerInfoAutoPtr> mListenerArray;

  PRLock* mListenerArrayLock;
};

#endif /* __SB_LOCALDATABASE_MEDIALISTLISTENER_H__ */
