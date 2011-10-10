/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SBSELECTIONLISTUTILS_H__
#define __SBSELECTIONLISTUTILS_H__

#include <nsStringGlue.h>
#include <nsDataHashtable.h>
#include <nsITreeSelection.h>

typedef nsDataHashtable<nsStringHashKey, nsString> sbSelectionList;

NS_HIDDEN_(PLDHashOperator) PR_CALLBACK
  SB_SerializeSelectionListCallback(nsStringHashKey::KeyType aKey,
                                    nsString aEntry,
                                    void* aUserData);

NS_HIDDEN_(PLDHashOperator) PR_CALLBACK
  SB_CopySelectionListCallback(nsStringHashKey::KeyType aKey,
                               nsString aEntry,
                               void* aUserData);

NS_HIDDEN_(PLDHashOperator) PR_CALLBACK
  SB_SelectionListGuidsToTArrayCallback(nsStringHashKey::KeyType aKey,
                                        nsString aEntry,
                                        void* aUserData);

class sbAutoSuppressSelectionEvents
{
public:
  sbAutoSuppressSelectionEvents(nsITreeSelection* aSelection) :
    mSelection(aSelection)
  {
    NS_ASSERTION(aSelection, "aSelection is null");
#ifdef DEBUG
    nsresult rv =
#endif
    mSelection->SetSelectEventsSuppressed(PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set");
  }

  ~sbAutoSuppressSelectionEvents()
  {
#ifdef DEBUG
    nsresult rv =
#endif
    mSelection->SetSelectEventsSuppressed(PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to unset");
  }

private:
  nsITreeSelection* mSelection;
};

#endif // __SBSELECTIONLISTUTILS_H__
