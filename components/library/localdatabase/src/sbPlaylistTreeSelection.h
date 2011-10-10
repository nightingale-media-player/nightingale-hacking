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

#ifndef __SBPLAYLISTTREESELECTION_H__
#define __SBPLAYLISTTREESELECTION_H__

#include "sbLocalDatabaseTreeView.h"

#include <nsCOMPtr.h>
#include <nsITreeSelection.h>
#include <sbIMediaListViewSelection.h>

class sbPlaylistTreeSelection : public nsITreeSelection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITREESELECTION

  sbPlaylistTreeSelection(nsITreeSelection* aTreeSelection,
                          sbIMediaListViewSelection* aViewSelection,
                          sbLocalDatabaseTreeView* aTreeView);

private:
  nsCOMPtr<nsITreeSelection> mTreeSelection;

  // sbLocalDatabaseTreeView keeps a strong ref to us so
  // we must keep weak references to both the tree and
  // the view selection
  sbIMediaListViewSelection* mViewSelection;
  sbLocalDatabaseTreeView* mTreeView;

  PRInt32 mShiftSelectPivot;
};

#endif /* __SBPLAYLISTTREESELECTION_H__ */
