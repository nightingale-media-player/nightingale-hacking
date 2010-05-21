/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbSongkickDBService.h"

#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsIIOService.h>
#include <nsIMutableArray.h>
#include <nsIProperties.h>
#include <nsIRunnable.h>
#include <nsIThread.h>
#include <nsIThreadManager.h>
#include <nsIThreadPool.h>
#include <nsIURI.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

#include <list>

//------------------------------------------------------------------------------

class sbSongkickResultEnumerator : public nsISimpleEnumerator
{
public:
  sbSongkickResultEnumerator();
  virtual ~sbSongkickResultEnumerator();

  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  nsresult Init(sbIDatabaseResult *aResult,
                sbIDatabaseQuery *aQuery);

protected:
  typedef std::list<nsRefPtr<sbSongkickConcertInfo> > sbConcertInfoList;
  typedef sbConcertInfoList::const_iterator           sbConcertInfoListIter;

  static nsresult ContainsConcertList(const nsAString & aID,
                                      const sbConcertInfoList & aConcertList,
                                      PRBool *aHasEntry);

  sbConcertInfoList     mConcertInfoList;
  sbConcertInfoListIter mConcertInfoListIter;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickResultEnumerator, nsISimpleEnumerator)

sbSongkickResultEnumerator::sbSongkickResultEnumerator()
{
}

sbSongkickResultEnumerator::~sbSongkickResultEnumerator()
{
}

nsresult
sbSongkickResultEnumerator::Init(sbIDatabaseResult *aResult,
                                 sbIDatabaseQuery *aQuery)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aQuery);

  // Convert all the values in the result to the concert interface type.
  nsresult rv;
  PRUint32 rowCount = 0;
  rv = aResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsString id;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("id"), id);

    // If this record has already been recorded, just continue.
    PRBool hasRecord = PR_FALSE;
    rv = ContainsConcertList(id, mConcertInfoList, &hasRecord);
    if (NS_FAILED(rv) || hasRecord) {
      continue;
    }
    
    nsString artistName;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("name"), artistName);

    nsString artistURL;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("artistURL"), artistURL);

    nsString ts;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("timestamp"), ts);

    // XXX KREEGER DECODE THIS YO
    nsString venue;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("venue"), venue);

    nsString city;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("city"), city);

    // XXX KREEGER DECODE THIS YO
    nsString title;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("title"), title);

    nsString url;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("concertURL"), url);

    nsString lib;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("libraryArtist"), lib);

    nsString venueURL;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("venueURL"), venueURL);

    nsString tickets;
    aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("tickets"), tickets);

    // Now lookup the playing at artists.
    rv = aQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);

    nsString stmt;
    stmt.AppendLiteral(
        "select * from artists join playing_at on playing_at.concertid=");
    stmt.Append(id);
    stmt.AppendLiteral(" and playing_at.artistid = artists.ROWID");

    rv = aQuery->AddQuery(stmt);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 queryResult;
    rv = aQuery->Execute(&queryResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

    nsCOMPtr<sbIDatabaseResult> playingAtResult;
    rv = aQuery->GetResultObject(getter_AddRefs(playingAtResult));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 playingAtCount = 0;
    rv = playingAtResult->GetRowCount(&playingAtCount);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMutableArray> artistConcertInfoArray =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    for (PRUint32 i = 0; i < playingAtCount; i++) {
      nsRefPtr<sbSongkickArtistConcertInfo> curArtistInfo =
        new sbSongkickArtistConcertInfo();
      NS_ENSURE_TRUE(curArtistInfo, NS_ERROR_OUT_OF_MEMORY);

      nsString artistName;
      playingAtResult->GetRowCellByColumn(
          i, NS_LITERAL_STRING("name"), artistName);

      nsString artistURL;
      playingAtResult->GetRowCellByColumn(
          i, NS_LITERAL_STRING("artistURL"), artistURL);

      rv = curArtistInfo->Init(artistName, artistURL);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = artistConcertInfoArray->AppendElement(curArtistInfo, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsRefPtr<sbSongkickConcertInfo> curInfo =
      new sbSongkickConcertInfo();
    NS_ENSURE_TRUE(curInfo, NS_ERROR_OUT_OF_MEMORY);

    rv = curInfo->Init(artistName,
                       artistURL,
                       id,
                       ts,
                       venue,
                       city,
                       title,
                       url,
                       venueURL,
                       tickets,
                       artistConcertInfoArray,
                       lib);
    NS_ENSURE_SUCCESS(rv, rv);

    mConcertInfoList.push_back(curInfo);
  }

  // Set the iter position
  mConcertInfoListIter = mConcertInfoList.begin();
  return NS_OK;
}

/* static */ nsresult
sbSongkickResultEnumerator::ContainsConcertList(
    const nsAString & aID,
    const sbConcertInfoList & aConcertList,
    PRBool *aHasEntry)
{
  NS_ENSURE_ARG_POINTER(aHasEntry);
  *aHasEntry = PR_FALSE;

  sbConcertInfoListIter begin = aConcertList.begin();
  sbConcertInfoListIter end = aConcertList.end();
  sbConcertInfoListIter next;
  for (next = begin; next != end && !*aHasEntry; ++next) {
    if ((*next)->mID.Equals(aID)) {
      *aHasEntry = PR_TRUE;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickResultEnumerator::HasMoreElements(PRBool *aHasMore)
{
  NS_ENSURE_ARG_POINTER(aHasMore);
  *aHasMore = mConcertInfoListIter != mConcertInfoList.end();
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickResultEnumerator::GetNext(nsISupports **aOutNext)
{
  NS_ENSURE_ARG_POINTER(aOutNext);

  nsresult rv;
  nsCOMPtr<nsISupports> curConcertInfo =
    do_QueryInterface(*mConcertInfoListIter++, &rv);

  curConcertInfo.forget(aOutNext);
  return NS_OK;
}

//------------------------------------------------------------------------------

class sbSongkickQuery : public nsIRunnable
{
public:
  sbSongkickQuery();
  virtual ~sbSongkickQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult Init(const nsAString & aQueryStmt,
                sbISongkickEnumeratorCallback *aCallback);

  void RunEnumCallbackStart();
  void RunEnumCallbackEnd();

private:
  nsCOMPtr<sbISongkickEnumeratorCallback> mCallback;
  nsCOMPtr<nsIThread>                     mCallingThread;
  nsRefPtr<sbSongkickResultEnumerator>    mResultsEnum;
  nsString                                mQueryStmt;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickQuery, nsIRunnable)

sbSongkickQuery::sbSongkickQuery()
{
}

sbSongkickQuery::~sbSongkickQuery()
{
}

NS_IMETHODIMP
sbSongkickQuery::Run()
{
  // First, notify the enum callback of enumeration start.
  nsresult rv;
  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbSongkickQuery, this, RunEnumCallbackStart);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);
  rv = mCallingThread->Dispatch(runnable, NS_DISPATCH_SYNC);

  // Setup the database query.
  nsCOMPtr<nsIIOService> ioService =
    do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperties> dirService =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> profileFile;
  rv = dirService->Get("ProfD",
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(profileFile));

  nsCOMPtr<nsIURI> dbLocationURI;
  rv = NS_NewFileURI(getter_AddRefs(dbLocationURI),
                     profileFile,
                     ioService);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> db =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->SetDatabaseLocation(dbLocationURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->SetDatabaseGUID(NS_LITERAL_STRING("concerts"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->AddQuery(mQueryStmt);
  NS_ENSURE_SUCCESS(rv, rv);

  // Phew, finally send of the query.
  PRInt32 queryResult;
  rv = db->Execute(&queryResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

  // Convert any results into the appropriate type.
  nsCOMPtr<sbIDatabaseResult> results;
  rv = db->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create another db query for lookup up playing at data while enumerating
  // through the current results.
  nsCOMPtr<sbIDatabaseQuery> artistDB =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = artistDB->SetDatabaseLocation(dbLocationURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = artistDB->SetDatabaseGUID(NS_LITERAL_STRING("concerts"));
  NS_ENSURE_SUCCESS(rv, rv);

  mResultsEnum = new sbSongkickResultEnumerator();
  NS_ENSURE_TRUE(mResultsEnum, NS_ERROR_OUT_OF_MEMORY);
  rv = mResultsEnum->Init(results, artistDB);
  NS_ENSURE_SUCCESS(rv, rv);

  // Finally, notify the listener of enumeration end.
  runnable =
    NS_NEW_RUNNABLE_METHOD(sbSongkickQuery, this, RunEnumCallbackEnd);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);
  rv = mCallingThread->Dispatch(runnable, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbSongkickQuery::Init(const nsAString & aQueryStmt,
                      sbISongkickEnumeratorCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  mQueryStmt = aQueryStmt;
  mCallback = aCallback;

  // Grap a reference to the calling thread.
  nsresult rv;
  nsCOMPtr<nsIThreadManager> threadMgr =
    do_GetService("@mozilla.org/thread-manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = threadMgr->GetCurrentThread(getter_AddRefs(mCallingThread));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbSongkickQuery::RunEnumCallbackStart()
{
  NS_ENSURE_TRUE(mCallback, /* void */);
  mCallback->OnEnumerationStart();
}

void
sbSongkickQuery::RunEnumCallbackEnd()
{
  NS_ENSURE_TRUE(mCallback, /* void */);

  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> outEnum = do_QueryInterface(
        NS_ISUPPORTS_CAST(nsISimpleEnumerator*, mResultsEnum),
        &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  mCallback->OnEnumerationEnd(outEnum);
}

//------------------------------------------------------------------------------

class sbSongkickCountQuery : public nsIRunnable
{
public:
  sbSongkickCountQuery();
  virtual ~sbSongkickCountQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult Init(const nsAString & aQueryStmt,
                sbISongkickConcertCountCallback *aCallback);

  void RunEnumCountCallback();

private:
  nsCOMPtr<sbISongkickConcertCountCallback> mCallback;
  nsCOMPtr<nsIThread>                       mCallingThread;
  nsString                                  mQueryStmt;
  PRUint32                                  mCount;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickCountQuery, nsIRunnable)

sbSongkickCountQuery::sbSongkickCountQuery()
  : mCount(0)
{
}

sbSongkickCountQuery::~sbSongkickCountQuery()
{
}

nsresult
sbSongkickCountQuery::Init(const nsAString & aQueryStmt,
                           sbISongkickConcertCountCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  mCallback = aCallback;
  mQueryStmt = aQueryStmt;

  // Grap a reference to the calling thread.
  nsresult rv;
  nsCOMPtr<nsIThreadManager> threadMgr =
    do_GetService("@mozilla.org/thread-manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = threadMgr->GetCurrentThread(getter_AddRefs(mCallingThread));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickCountQuery::Run()
{
  nsresult rv;

  // Setup the database query.
  nsCOMPtr<nsIIOService> ioService =
    do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperties> dirService =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> profileFile;
  rv = dirService->Get("ProfD",
                       NS_GET_IID(nsIFile),
                       getter_AddRefs(profileFile));

  nsCOMPtr<nsIURI> dbLocationURI;
  rv = NS_NewFileURI(getter_AddRefs(dbLocationURI),
                     profileFile,
                     ioService);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> db =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->SetDatabaseLocation(dbLocationURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->SetDatabaseGUID(NS_LITERAL_STRING("concerts"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = db->AddQuery(mQueryStmt);
  NS_ENSURE_SUCCESS(rv, rv);

  // Phew, finally send of the query.
  PRInt32 queryResult;
  rv = db->Execute(&queryResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

  // Convert any results into the appropriate type.
  nsCOMPtr<sbIDatabaseResult> results;
  rv = db->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString rowCountStr;
  rv = results->GetRowCell(0, 0, rowCountStr);
  NS_ENSURE_SUCCESS(rv, rv);

  mCount = rowCountStr.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbSongkickCountQuery, this, RunEnumCountCallback);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);
  rv = mCallingThread->Dispatch(runnable, NS_DISPATCH_SYNC);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbSongkickCountQuery::RunEnumCountCallback()
{
  NS_ENSURE_TRUE(mCallback, /* void */);
  mCallback->OnConcertCountEnd(mCount);
}

//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickDBService, sbPISongkickDBService)

sbSongkickDBService::sbSongkickDBService()
{
}

sbSongkickDBService::~sbSongkickDBService()
{
}

NS_IMETHODIMP
sbSongkickDBService::StartAristConcertLookup(
    PRBool aFilter,
    sbISongkickEnumeratorCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  nsString stmt;
  stmt.AppendLiteral("SELECT * FROM playing_at");
  stmt.AppendLiteral(" JOIN artists ON playing_at.artistid = artists.ROWID");
  stmt.AppendLiteral(" JOIN concerts ON playing_at.concertid = concerts.id");
  if (aFilter) {
    stmt.AppendLiteral(" WHERE playing_at.libraryArtist = 1");
  }
  stmt.AppendLiteral(" ORDER BY artists.name COLLATE NOCASE");

  nsRefPtr<sbSongkickQuery> skQuery = new sbSongkickQuery();
  NS_ENSURE_TRUE(skQuery, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  rv = skQuery->Init(stmt, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPoolService->Dispatch(skQuery, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::StartConcertLookup(
    const nsAString & aSort,
    PRBool aFilter,
    sbISongkickEnumeratorCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  nsString stmt;
  stmt.AppendLiteral("SELECT * FROM concerts");
  stmt.AppendLiteral(" JOIN playing_at ON playing_at.concertid = concerts.id");
  stmt.AppendLiteral(" JOIN artists ON playing_at.artistid = artists.ROWID");

  if (aFilter) {
    stmt.AppendLiteral(" WHERE playing_at.anyLibraryArtist = 1");
  }

  // Figure out the sort key
  stmt.AppendLiteral(" ORDER BY ");
  if (aSort.EqualsLiteral("date")) {
    stmt.AppendLiteral("timestamp");
  }
  else if (aSort.EqualsLiteral("venue")) {
    stmt.AppendLiteral("venue");
  }
  else {
    // Fallback to 'title' (this includes aSort == "title").
    stmt.AppendLiteral("title");
  }

  nsRefPtr<sbSongkickQuery> skQuery = new sbSongkickQuery();
  NS_ENSURE_TRUE(skQuery, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  rv = skQuery->Init(stmt, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPoolService->Dispatch(skQuery, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::StartConcertCountLookup(
    PRBool aFilter,
    PRBool aGroupByArtist,
    const nsAString & aDateProperty,
    const nsAString & aCeilingProperty,
    sbISongkickConcertCountCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);

  nsresult rv;

  nsString stmt;
  if (aGroupByArtist) {
    stmt.AppendLiteral("SELECT COUNT(distinct concertid) FROM playing_at");
    stmt.AppendLiteral(" JOIN concerts ON playing_at.concertid = concerts.id");
    stmt.AppendLiteral(" WHERE concerts.timestamp >= ");
    stmt.Append(aDateProperty);
    if (aFilter) {
      stmt.AppendLiteral(" AND playing_at.libraryArtist = 1");
    }
  }
  else {
    stmt.AppendLiteral("SELECT COUNT(distinct id) FROM playing_at");
    stmt.AppendLiteral(" JOIN concerts ON playing_at.concertid = concerts.id");
    stmt.AppendLiteral(" WHERE concerts.timestamp >= ");
    stmt.Append(aDateProperty);
    stmt.AppendLiteral(" AND concerts.timestamp < ");
    stmt.Append(aCeilingProperty);
    if (aFilter) {
      stmt.AppendLiteral(" AND playing_at.anyLibraryArtist = 1");
    }
  }

  nsRefPtr<sbSongkickCountQuery> skQuery = new sbSongkickCountQuery();
  NS_ENSURE_TRUE(skQuery, NS_ERROR_OUT_OF_MEMORY);

  rv = skQuery->Init(stmt, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPoolService->Dispatch(skQuery, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickConcertInfo, sbISongkickConcertInfo)

sbSongkickConcertInfo::sbSongkickConcertInfo()
{
}

sbSongkickConcertInfo::~sbSongkickConcertInfo()
{
}

nsresult
sbSongkickConcertInfo::Init(const nsAString & aArtistname,
                            const nsAString & aArtistURL,
                            const nsAString & aID,
                            const nsAString & aTS,
                            const nsAString & aVenue,
                            const nsAString & aCity,
                            const nsAString & aTitle,
                            const nsAString & aURL,
                            const nsAString & aVenueURL,
                            const nsAString & aTickets,
                            nsIArray *aArtistsConcertInfo,
                            const nsAString & aLibartist)
{
  mArtistname = aArtistname;
  mArtistURL = aArtistURL;
  mID = aID;
  mTS = aTS;
  mVenue = aVenue;
  mCity = aCity;
  mTitle = aTitle;
  mURL = aURL;
  mVenueURL = aVenueURL;
  mTickets = aTickets;
  mArtistConcertInfoArray = aArtistsConcertInfo;
  mLibArtist = aLibartist;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetArtistname(nsAString & aArtistname)
{
  aArtistname = mArtistname;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetArtisturl(nsAString & aArtisturl)
{
  aArtisturl = mArtistURL;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetId(nsAString & aId)
{
  aId = mID;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetTs(nsAString & aTs)
{
  aTs = mTS;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetVenue(nsAString & aVenue)
{
  aVenue = mVenue;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetCity(nsAString & aCity)
{
  aCity = mCity;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetTitle(nsAString & aTitle)
{
  aTitle = mTitle;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetUrl(nsAString & aUrl)
{
  aUrl = mURL;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetVenueURL(nsAString & aVenueURL)
{
  aVenueURL = mVenueURL;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetTickets(nsAString & aTickets)
{
  aTickets = mTickets;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetArtistsConcertInfo(nsIArray **aArtistsConcertInfo)
{
  NS_ENSURE_ARG_POINTER(aArtistsConcertInfo);
  NS_IF_ADDREF(*aArtistsConcertInfo = mArtistConcertInfoArray);
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickConcertInfo::GetLibartist(nsAString & aLibartist)
{
  mLibArtist = aLibartist;
  return NS_OK;
}

//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickArtistConcertInfo,
                              sbISongkickArtistConcertInfo)

sbSongkickArtistConcertInfo::sbSongkickArtistConcertInfo()
{
}

sbSongkickArtistConcertInfo::~sbSongkickArtistConcertInfo()
{
}

nsresult
sbSongkickArtistConcertInfo::Init(const nsAString & aArtistName,
                                  const nsAString & aArtistURL)
{
  mArtistName = aArtistName;
  mArtistURL = aArtistURL;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickArtistConcertInfo::GetArtistname(nsAString & aArtistname)
{
  aArtistname = mArtistName;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickArtistConcertInfo::GetArtisturl(nsAString & aArtisturl)
{
  aArtisturl = mArtistURL;
  return NS_OK;
}

