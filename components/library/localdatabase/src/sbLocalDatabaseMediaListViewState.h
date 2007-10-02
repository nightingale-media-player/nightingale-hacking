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

#ifndef __SBLOCALDATABASEMEDIALISTVIEWSTATE_H__
#define __SBLOCALDATABASEMEDIALISTVIEWSTATE_H__

#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseCascadeFilterSet.h"
#include "sbLocalDatabaseTreeView.h"

#include <sbIMediaListView.h>
#include <sbILibraryConstraints.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsTArray.h>
#include <nsStringGlue.h>
#include <nsIClassInfo.h>
#include <nsISerializable.h>

#define SB_ILOCALDATABASEMEDIALISTVIEWSTATE_IID \
{ 0xd04ebdd9, 0x9c9c, 0x4acc, \
  { 0xb5, 0x4c, 0x7d, 0x30, 0x3b, 0x76, 0xee, 0xac } }

class sbILocalDatabaseMediaListViewState : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(SB_ILOCALDATABASEMEDIALISTVIEWSTATE_IID)

  NS_IMETHOD GetSort(sbIMutablePropertyArray** aSort) = 0;
  NS_IMETHOD GetSearch(sbIMutablePropertyArray** aSearch) = 0;
  NS_IMETHOD GetFilter(sbIMutablePropertyArray** aFilter) = 0;
  NS_IMETHOD GetFilterSet(sbLocalDatabaseCascadeFilterSetState** aFilterSet) = 0;
  NS_IMETHOD GetTreeViewState(sbLocalDatabaseTreeViewState** aTreeViewState) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(sbILocalDatabaseMediaListViewState,
                              SB_ILOCALDATABASEMEDIALISTVIEWSTATE_IID)

class sbLocalDatabaseMediaListViewState : public sbILocalDatabaseMediaListViewState,
                                          public sbIMediaListViewState,
                                          public nsISerializable,
                                          public nsIClassInfo
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTVIEWSTATE
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  // sbILocalDatabaseMediaListViewState
  NS_IMETHOD GetSort(sbIMutablePropertyArray** aSort);
  NS_IMETHOD GetSearch(sbIMutablePropertyArray** aSearch);
  NS_IMETHOD GetFilter(sbIMutablePropertyArray** aFilter);
  NS_IMETHOD GetFilterSet(sbLocalDatabaseCascadeFilterSetState** aFilterSet);
  NS_IMETHOD GetTreeViewState(sbLocalDatabaseTreeViewState** aTreeViewState);

  sbLocalDatabaseMediaListViewState();
  sbLocalDatabaseMediaListViewState(sbIMutablePropertyArray* aSort,
                                    sbIMutablePropertyArray* aSearch,
                                    sbIMutablePropertyArray* aFilter,
                                    sbLocalDatabaseCascadeFilterSetState* aFilterSet,
                                    sbLocalDatabaseTreeViewState* aTreeViewState);
private:
  PRBool mInitialized;
  nsCOMPtr<sbIMutablePropertyArray> mSort;
  nsCOMPtr<sbIMutablePropertyArray> mSearch;
  nsCOMPtr<sbIMutablePropertyArray> mFilter;
  nsRefPtr<sbLocalDatabaseCascadeFilterSetState> mFilterSet;
  nsRefPtr<sbLocalDatabaseTreeViewState> mTreeViewState;
};

#endif /* __SBLOCALDATABASEMEDIALISTVIEWSTATE_H__ */

