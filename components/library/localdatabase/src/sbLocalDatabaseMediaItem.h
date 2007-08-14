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

#ifndef __SBLOCALDATABASEMEDIAITEM_H__
#define __SBLOCALDATABASEMEDIAITEM_H__

#include <sbILibraryResource.h>
#include <sbILocalDatabaseMediaItem.h>
#include <sbILocalDatabaseResourceProperty.h>
#include <sbIMediaItem.h>
#include <nsIClassInfo.h>
#include <nsIRequestObserver.h>
#include <nsWeakReference.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prlock.h>

class sbILocalDatabasePropertyCache;
class sbILocalDatabaseResourcePropertyBag;
class sbLocalDatabaseLibrary;

class sbLocalDatabaseMediaItem : public nsSupportsWeakReference,
                                 public nsIClassInfo,
                                 public nsIRequestObserver,
                                 public sbILocalDatabaseResourceProperty,
                                 public sbILocalDatabaseMediaItem,
                                 public sbIMediaItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSICLASSINFO
  NS_DECL_SBILIBRARYRESOURCE
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTY
  NS_DECL_SBILOCALDATABASEMEDIAITEM
  NS_DECL_SBIMEDIAITEM

  sbLocalDatabaseMediaItem();

  virtual ~sbLocalDatabaseMediaItem();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary,
                const nsAString& aGuid,
                PRBool aOwnsLibrary = PR_TRUE);

private:
  nsresult EnsurePropertyBag();

protected:
  PRUint32 mMediaItemId;

  // Determines what kind of reference will be kept for mLibrary.  If true,
  // it will be an owning reference, if false, it will be a non-owning
  // reference.
  PRBool mOwnsLibrary;

  // This reference is either owning or non-owning depending on how this class
  // was initalized.  We do manual reference counting if this is to be an
  // owning reference
  sbLocalDatabaseLibrary* mLibrary;

  nsString mGuid;

private:
  PRLock* mPropertyCacheLock;
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  PRLock* mPropertyBagLock;
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> mPropertyBag;
};

class sbLocalDatabaseIndexedMediaItem : public nsIClassInfo,
                                        public sbIIndexedMediaItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIINDEXEDMEDIAITEM

  sbLocalDatabaseIndexedMediaItem(PRUint32 aIndex, sbIMediaItem* aMediaItem) :
    mIndex(aIndex),
    mMediaItem(aMediaItem)
  {
    NS_ASSERTION(aMediaItem, "Null value passed to ctor");
  }

private:
  PRUint32 mIndex;
  nsCOMPtr<sbIMediaItem> mMediaItem;
};

#endif /* __SBLOCALDATABASEMEDIAITEM_H__ */
