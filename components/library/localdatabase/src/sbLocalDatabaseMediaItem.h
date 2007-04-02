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
#include <sbILocalDatabaseResourceProperty.h>
#include <sbILocalDatabaseMediaItem.h>
#include <sbIMediaItem.h>
#include <nsIClassInfo.h>
#include <nsWeakReference.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prlock.h>

class sbILocalDatabaseLibrary;
class sbILocalDatabasePropertyCache;
class sbILocalDatabaseResourcePropertyBag;

class sbLocalDatabaseMediaItem : public nsSupportsWeakReference,
                                 public nsIClassInfo,
                                 public sbILocalDatabaseResourceProperty,
                                 public sbILocalDatabaseMediaItem,
                                 public sbIMediaItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBILIBRARYRESOURCE
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTY
  NS_DECL_SBILOCALDATABASEMEDIAITEM
  NS_DECL_SBIMEDIAITEM

  sbLocalDatabaseMediaItem();

  virtual ~sbLocalDatabaseMediaItem();

  nsresult Init(sbILocalDatabaseLibrary* aLibrary,
                const nsAString& aGuid);

private:
  nsresult GetPropertyBag();

protected:
  PRUint32 mMediaItemId;

  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;

private:
  PRLock* mPropertyCacheLock;
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  PRLock* mPropertyBagLock;
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> mPropertyBag;

  PRLock*   mGuidLock;
  nsString  mGuid;

  PRPackedBool mWriteThrough;
  PRPackedBool mWritePending;
};

#endif /* __SBLOCALDATABASEMEDIAITEM_H__ */
