/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef __SB_PLAYBACKHISTORYSERVICE_H__
#define __SB_PLAYBACKHISTORYSERVICE_H__

#include <sbIPlaybackHistoryService.h>

#include <nsIClassInfo.h>
#include <nsIComponentManager.h>
#include <nsIFile.h>
#include <nsIGenericFactory.h>
#include <nsIObserver.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <sbIDatabaseQuery.h>

class sbPlaybackHistoryService : public sbIPlaybackHistoryService,
                                 public nsIObserver
{
public:
  sbPlaybackHistoryService();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIPLAYBACKHISTORYSERVICE

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
                                nsIFile* aPath,
                                const char* aLoaderStr,
                                const char* aType,
                                const nsModuleComponentInfo *aInfo);

  NS_METHOD Init();
  
  nsresult CreateQueries();
  nsresult CreateDefaultQuery(sbIDatabaseQuery **aQuery);

  nsresult EnsureHistoryDatabaseAvailable();

  nsresult FillAddQueryParameters(sbIDatabaseQuery *aQuery,
                                  sbIPlaybackHistoryEntry *aEntry);
  nsresult FillAddAnnotationsQueryParameters(sbIDatabaseQuery *aQuery,
                                             sbIPlaybackHistoryEntry *aEntry);

protected:
  ~sbPlaybackHistoryService();

private:
  nsString mAddEntryQuery;

  nsString mGetEntryByIndexQuery;
  nsString mGetEntriesByIndexQuery;
  nsString mGetEntriesByTimestampQuery;

  nsString mRemoveEntriesByIndexQuery;
  
  nsString mRemoveAllEntriesQuery;
  nsString mRemoveAllAnnotationsQuery;

};

#endif /* __SB_PLAYBACKHISTORYSERVICE_H__ */
