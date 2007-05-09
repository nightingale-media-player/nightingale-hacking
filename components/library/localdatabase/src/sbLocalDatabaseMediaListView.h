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

#ifndef __SB_LOCALDATABASEMEDIALISTVIEW_H__
#define __SB_LOCALDATABASEMEDIALISTVIEW_H__

#include <nsCOMPtr.h>
#include <nsClassHashtable.h>
#include <nsIClassInfo.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <sbIFilterableMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISearchableMediaList.h>
#include <sbISortableMediaList.h>
#include <sbPropertiesCID.h>

class nsITreeView;
class nsIURI;
class sbICascadeFilterSet;
class sbIDatabaseQuery;
class sbIDatabaseResult;
class sbILocalDatabaseAsyncGUIDArray;
class sbILocalDatabaseLibrary;
class sbIMediaList;

class sbLocalDatabaseMediaListView : public sbIMediaListView,
                                     public sbIMediaListListener,
                                     public sbIFilterableMediaList,
                                     public sbISearchableMediaList,
                                     public sbISortableMediaList,
                                     public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTVIEW
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBIFILTERABLEMEDIALIST
  NS_DECL_SBISEARCHABLEMEDIALIST
  NS_DECL_SBISORTABLEMEDIALIST
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaListView(sbILocalDatabaseLibrary* aLibrary,
                               sbIMediaList* aMediaList,
                               nsAString& aDefaultSortProperty,
                               PRUint32 aMediaListId);

  ~sbLocalDatabaseMediaListView();

  nsresult Init();
private:
  typedef nsTArray<nsString> sbStringArray;
  typedef nsClassHashtable<nsStringHashKey, sbStringArray> sbStringArrayHash;

  static PLDHashOperator PR_CALLBACK
    AddFilterToGUIDArrayCallback(nsStringHashKey::KeyType aKey,
                                 sbStringArray* aEntry,
                                 void* aUserData);

  nsresult MakeStandardQuery(sbIDatabaseQuery** _retval);

  nsresult UpdateFiltersInternal(sbIPropertyArray* aPropertyArray,
                                 PRBool aReplace);

  nsresult UpdateViewArrayConfiguration();

  nsresult CreateQueries();

  nsresult Invalidate();

  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;

  // Property Manager
  nsCOMPtr<sbIPropertyManager> mPropMan;

  // The media list this view is of
  nsCOMPtr<sbIMediaList> mMediaList;

  // The default sort property of the guid array
  nsString mDefaultSortProperty;

  // Internal id of this media list, or 0 for the full library
  PRUint32 mMediaListId;

  // Database guid and location
  nsString mDatabaseGuid;
  nsCOMPtr<nsIURI> mDatabaseLocation;

  // GUID array for this view instance
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mArray;

  // Filter say for this view, if any
  nsCOMPtr<sbICascadeFilterSet> mCascadeFilterSet;

  // Tree view for this view, if any
  nsCOMPtr<nsITreeView> mTreeView;

  // Map of current view filter configuration
  sbStringArrayHash mViewFilters;

  // Current search filter configuration
  nsCOMPtr<sbIPropertyArray> mViewSearches;

  // Current sort filter configuration
  nsCOMPtr<sbIPropertyArray> mViewSort;

  // Query to return list of values for a given property
  nsString mDistinctPropertyValuesQuery;

  // Whether we're in batch mode.
  PRPackedBool mInBatch;

  // True when we should invalidate when batching ends
  PRPackedBool mInvalidatePending;
};

#endif /* __SB_LOCALDATABASEMEDIALISTVIEW_H__ */

