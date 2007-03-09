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

#ifndef __SBLOCALDATABASEMEDIALISTBASE_H__
#define __SBLOCALDATABASEMEDIALISTBASE_H__

#include <nsClassHashTable.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <sbILibrary.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbIMediaList.h>


class sbLocalDatabaseMediaListBase : public sbIMediaList,
                                     public nsIClassInfo
{
  typedef nsTArray<nsString> sbStringArray;
  typedef nsClassHashtable<nsStringHashKey, sbStringArray> sbStringArrayHash;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYRESOURCE
  NS_DECL_SBIMEDIAITEM
  NS_DECL_SBIMEDIALIST
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaListBase(sbILibrary* aLibrary, const nsAString& aGuid);

  NS_IMETHODIMP Init();

private:

  static PLDHashOperator PR_CALLBACK
    AddFilterToGUIDArrayEnumerator(nsStringHashKey::KeyType aKey,
                                   sbStringArray* aEntry,
                                   void* aUserData);

protected:
  /*
   * The library this media list instance belogs to
   */
  nsCOMPtr<sbILibrary> mLibrary;

  /*
   * The guid of this media list
   */
  nsString mGuid;

  /*
   * The mViewArray is the guid array that represents the contents of the
   * media list when viewed in the UI.  This is the array that gets updated
   * from calls through the sbI*ableMediaList interfaces.  The contents of this
   * array does not affect the API level list manupulation methods
   */
  nsCOMPtr<sbILocalDatabaseGUIDArray> mViewArray;

  /*
   * The mFullArray is a cached version of the full contents of the media
   * list this instance represents.
   */
  nsCOMPtr<sbILocalDatabaseGUIDArray> mFullArray;

};

#endif /* __SBLOCALDATABASEMEDIALISTBASE_H__ */
