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

#include "sbLocalDatabaseSimpleMediaList.h"
#include "sbLocalDatabaseMediaListBase.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseLibrary.h"

#include <sbIMediaListListener.h>
#include <sbILibrary.h>
#include <sbIMediaListListener.h>

#include <nsComponentManagerUtils.h>

#define DEFAULT_SORT_PROPERTY \
  NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#ordinal")
#define DEFAULT_FETCH_SIZE 1000

NS_IMPL_ISUPPORTS_INHERITED0(sbLocalDatabaseSimpleMediaList,
                             sbLocalDatabaseMediaListBase)

sbLocalDatabaseSimpleMediaList::sbLocalDatabaseSimpleMediaList(sbILibrary* aLibrary,
                                                               const nsAString& aGuid) :
  sbLocalDatabaseMediaListBase(aLibrary, aGuid)
{
}

sbLocalDatabaseSimpleMediaList::~sbLocalDatabaseSimpleMediaList()
{
}

nsresult
sbLocalDatabaseSimpleMediaList::Init()
{
  nsresult rv;

  mFullArray = do_CreateInstance(SB_LOCALDATABASE_GUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbLocalDatabaseLibrary* library =
    NS_STATIC_CAST(sbLocalDatabaseLibrary*, mLibrary.get());

  /*
   * Get the media_item_id for this simple media list
   */
  PRUint32 mediaItemId;
  rv = library->GetMediaItemIdForGuid(mGuid, &mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  library->GetDatabaseGuid(databaseGuid);

  rv = mFullArray->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseTable(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseConstraintColumn(NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseConstraintValue(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->AddSort(DEFAULT_SORT_PROPERTY, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetFetchSize(DEFAULT_FETCH_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

