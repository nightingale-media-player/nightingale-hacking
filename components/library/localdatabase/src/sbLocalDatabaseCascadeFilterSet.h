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

#ifndef __SBLOCALDATABASECASCADEFILTERSET_H__
#define __SBLOCALDATABASECASCADEFILTERSET_H__

#include <nsCOMPtr.h>
#include <nsAutoPtr.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <sbICascadeFilterSet.h>
#include <nsTHashtable.h>
#include <nsHashKeys.h>

class sbLocalDatabaseTreeView;
class sbILocalDatabaseAsyncGUIDArray;
class sbILocalDatabaseLibrary;
class sbIMediaListView;
class sbIPropertyArray;

class sbLocalDatabaseCascadeFilterSet : public sbICascadeFilterSet
{
  typedef nsTArray<nsString> sbStringArray;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICASCADEFILTERSET

  sbLocalDatabaseCascadeFilterSet();
  ~sbLocalDatabaseCascadeFilterSet();

  nsresult Init(sbILocalDatabaseLibrary* aLibrary,
                sbIMediaListView* aMediaListView,
                sbILocalDatabaseAsyncGUIDArray* aProtoArray);

private:
  nsresult ConfigureArray(PRUint32 aIndex);

  struct sbFilterSpec {
    PRBool isSearch;
    nsString property;
    sbStringArray propertyList;
    sbStringArray values;
    nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> array;
    nsRefPtr<sbLocalDatabaseTreeView> treeView;
  };

  // This callback is meant to be used with mListeners.
  // aUserData should be a sbICascadeFilterSetListener pointer.
  static PLDHashOperator PR_CALLBACK
    OnValuesChangedCallback(nsISupportsHashKey* aKey,
                            void* aUserData);

  // The library this filter set is associated with
  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;

  // The media list view this filter set is associated with
  nsCOMPtr<sbIMediaListView> mMediaListView;

  // Prototypical array that is cloned to provide each filter's data
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mProtoArray;

  // Current filter configuration
  nsTArray<sbFilterSpec> mFilters;

  // Change listeners
  nsTHashtable<nsISupportsHashKey> mListeners;
};

class sbGUIDArrayPrimarySortEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbGUIDArrayPrimarySortEnumerator(sbILocalDatabaseAsyncGUIDArray* aArray);

private:
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mArray;
  PRUint32 mNextIndex;
};

#endif /* __SBLOCALDATABASECASCADEFILTERSET_H__ */

