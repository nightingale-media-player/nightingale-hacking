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

#ifndef __SBLOCALDATABASETREEVIEW_H__
#define __SBLOCALDATABASETREEVIEW_H__

#include "sbILocalDatabaseGUIDArray.h"
#include "sbILocalDatabaseTreeView.h"
#include "sbILocalDatabasePropertyCache.h"
#include <nsITreeView.h>
#include <nsITreeSelection.h>
#include <nsITreeBoxObject.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsInterfaceHashtable.h>
#include <nsTArray.h>
#include <nsITimer.h>

class sbLocalDatabaseTreeView : public sbILocalDatabaseTreeView,
                                public nsITreeView,
                                public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASETREEVIEW
  NS_DECL_NSITREEVIEW
  NS_DECL_NSITIMERCALLBACK

  sbLocalDatabaseTreeView();

private:
  ~sbLocalDatabaseTreeView();

  NS_IMETHODIMP FetchProperties();

  NS_IMETHODIMP GetPropertyForTreeColumn(nsITreeColumn* aTreeColumn,
                                         nsAString& aProperty);
  NS_IMETHODIMP ResetFilters();

  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;
  nsCOMPtr<nsITreeSelection> mSelection;
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  nsInterfaceHashtable<nsStringHashKey, sbILocalDatabaseResourcePropertyBag> mCache;
  nsTArray<nsString> mFetchList;
  nsCOMPtr<nsITimer> mFetchTimer;
  PRBool mFetchTimerScheduled;

  nsString mSortProperty;
  PRBool   mSortDirection;
  nsString mSearch;
};

#endif /* __SBLOCALDATABASETREEVIEW_H__ */

