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

#ifndef __SBLOCALDATABASEVIEWMEDIALIST_H__
#define __SBLOCALDATABASEVIEWMEDIALIST_H__

#include "sbLocalDatabaseMediaListBase.h"

#include <sbIMediaList.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaItem.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbLocalDatabaseViewMediaList : public sbLocalDatabaseMediaListBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  sbLocalDatabaseViewMediaList(sbILocalDatabaseLibrary* aLibrary,
                               const nsAString& aGuid);

  nsresult Init();

  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, PRBool* _retval);
  NS_IMETHOD Add(sbIMediaItem *aMediaItem);
  NS_IMETHOD AddAll(sbIMediaList *aMediaList);
  NS_IMETHOD AddSome(nsISimpleEnumerator *aMediaItems);

private:
  ~sbLocalDatabaseViewMediaList();
};

#endif /* __SBLOCALDATABASEVIEWMEDIALIST_H__ */

