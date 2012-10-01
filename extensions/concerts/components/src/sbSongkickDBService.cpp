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

#include <sbIDatabaseResult.h>

#include <nsICategoryManager.h>
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsIIOService.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>
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


//==============================================================================
// String escaping utility functions
//==============================================================================

nsString c2h( PRUnichar dec )
{
  char dig1 = (dec&0xF0)>>4;
  char dig2 = (dec&0x0F);
  if ( 0<= dig1 && dig1<= 9) dig1+=48;    //0,48inascii
  if (10<= dig1 && dig1<=15) dig1+=97-10; //a,97inascii
  if ( 0<= dig2 && dig2<= 9) dig2+=48;
  if (10<= dig2 && dig2<=15) dig2+=97-10;

  nsString r;
  r.Append(dig1);
  r.Append(dig2);
  return r;
}

//based on javascript encodeURIComponent()
nsString encodeURIComponent(const nsString &c)
{
  nsString escaped;
  int max = c.Length();
  for (int i=0; i<max; i++) {
    if ((48 <= c[i] && c[i] <= 57) ||//0-9
        (65 <= c[i] && c[i] <= 90) ||//abc...xyz
        (97 <= c[i] && c[i] <= 122) || //ABC...XYZ
        (c[i]=='~' || c[i]=='!' || c[i]=='*' || c[i]=='(' || c[i]==')' || c[i]=='\'')
       )
    {
      escaped.Append(c[i]);
    }
    else {
      escaped.AppendLiteral("%");
      escaped.Append( c2h(c[i]) );//converts char 255 to string "ff"
    }
  }
  return escaped;
}


//==============================================================================
// sbSongkickResultEnumerator
//==============================================================================

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
                                      bool *aHasEntry);

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
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("id"), id);
    NS_ENSURE_SUCCESS(rv, rv);

    // If this record has already been recorded, just continue.
    bool hasRecord = PR_FALSE;
    rv = ContainsConcertList(id, mConcertInfoList, &hasRecord);
    if (NS_FAILED(rv) || hasRecord) {
      continue;
    }
    
    nsString artistName;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("name"), artistName);
    NS_ENSURE_SUCCESS(rv, rv);
    artistName = encodeURIComponent(artistName);

    nsString artistURL;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("artistURL"), artistURL);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString ts;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("timestamp"), ts);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString venue;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("venue"), venue);
    NS_ENSURE_SUCCESS(rv, rv);
    venue = encodeURIComponent(venue);

    nsString city;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("city"), city);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString title;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("title"), title);
    NS_ENSURE_SUCCESS(rv, rv);
    title = encodeURIComponent(title);

    nsString url;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("concertURL"), url);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString lib;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("libraryArtist"), lib);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString venueURL;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("venueURL"), venueURL);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString tickets;
    rv = aResult->GetRowCellByColumn(i, NS_LITERAL_STRING("tickets"), tickets);
    NS_ENSURE_SUCCESS(rv, rv);

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
      artistName = encodeURIComponent(artistName);

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
    bool *aHasEntry)
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
sbSongkickResultEnumerator::HasMoreElements(bool *aHasMore)
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


//==============================================================================
// sbSongkickDBInfo
//==============================================================================

class sbSongkickDBInfo : public nsIRunnable
{
  friend class sbSongkickDBService;

public:
  sbSongkickDBInfo();
  virtual ~sbSongkickDBInfo();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult Init(sbSongkickDBService *aDBService);

protected:
  nsresult LoadGotLocationInfo();
  nsresult LoadLocationCountryInfo();
  nsresult LoadLocationStateInfo();
  nsresult LoadLocationCityInfo();

  // Shared members:
  bool                        mGotLocationInfo;
  nsCOMPtr<nsIMutableArray>     mCountriesProps;
  nsCOMPtr<nsIMutableArray>     mStateProps;
  nsCOMPtr<nsIMutableArray>     mCityProps;

private:
  nsCOMPtr<sbIDatabaseQuery>    mDBQuery;
  nsRefPtr<sbSongkickDBService> mDBService;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickDBInfo, nsIRunnable)

sbSongkickDBInfo::sbSongkickDBInfo()
  : mGotLocationInfo(PR_FALSE)
{
}

sbSongkickDBInfo::~sbSongkickDBInfo()
{
}

nsresult
sbSongkickDBInfo::Init(sbSongkickDBService *aService)
{
  NS_ENSURE_ARG_POINTER(aService);
  mDBService = aService;

  nsresult rv;
  mCountriesProps =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mStateProps =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mCityProps =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBInfo::Run()
{
  NS_ENSURE_TRUE(mDBService, NS_ERROR_UNEXPECTED);

  nsresult rv;
  rv = mDBService->GetDatabaseQuery(getter_AddRefs(mDBQuery));
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    // set an active flag...?
    return NS_OK;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadGotLocationInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadLocationCountryInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadLocationStateInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadLocationCityInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbSongkickDBInfo::LoadGotLocationInfo()
{
  nsresult rv;
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  nsString stmt;
  stmt.AssignLiteral("select count(*) from cities");

  rv = mDBQuery->AddQuery(stmt);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 queryResult;
  rv = mDBQuery->Execute(&queryResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> results;
  rv = mDBQuery->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString rowCountStr;
  rv = results->GetRowCell(0, 0, rowCountStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount = rowCountStr.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mGotLocationInfo = rowCount > 0;

  return NS_OK;
}

nsresult
sbSongkickDBInfo::LoadLocationCountryInfo()
{
  nsresult rv;
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("select * from countries"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 queryResult;
  rv = mDBQuery->Execute(&queryResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> results;
  rv = mDBQuery->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount = 0;
  rv = results->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsString id, name;
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("id"), id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("name"), name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbSongkickProperty> curProp = new sbSongkickProperty();
    NS_ENSURE_TRUE(curProp, NS_ERROR_OUT_OF_MEMORY);

    rv = curProp->Init(name, id, EmptyString());
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mCountriesProps->AppendElement(curProp, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbSongkickDBInfo::LoadLocationStateInfo()
{
  nsresult rv;
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("select * from states"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 queryResult;
  rv = mDBQuery->Execute(&queryResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> results;
  rv = mDBQuery->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount = 0;
  rv = results->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsString id, name, country;
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("id"), id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("name"), name);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("country"), country);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbSongkickProperty> curProp = new sbSongkickProperty();
    NS_ENSURE_TRUE(curProp, NS_ERROR_OUT_OF_MEMORY);

    rv = curProp->Init(name, id, country);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStateProps->AppendElement(curProp, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbSongkickDBInfo::LoadLocationCityInfo()
{
  nsresult rv;
  rv = mDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("select * from cities"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 queryResult;
  rv = mDBQuery->Execute(&queryResult);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> results;
  rv = mDBQuery->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount = 0;
  rv = results->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsString id, name, state;
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("id"), id);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("name"), name);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = results->GetRowCellByColumn(i, NS_LITERAL_STRING("state"), state);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbSongkickProperty> curProp = new sbSongkickProperty();
    NS_ENSURE_TRUE(curProp, NS_ERROR_OUT_OF_MEMORY);

    rv = curProp->Init(name, id, state);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mCityProps->AppendElement(curProp, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//==============================================================================
// sbSongkickQuery
//==============================================================================

class sbSongkickQuery : public nsIRunnable
{
public:
  sbSongkickQuery();
  virtual ~sbSongkickQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult Init(const nsAString & aQueryStmt,
                sbSongkickDBService *aDBService,
                sbISongkickEnumeratorCallback *aCallback);

  void RunEnumCallbackStart();
  void RunEnumCallbackEnd();

private:
  nsCOMPtr<sbISongkickEnumeratorCallback> mCallback;
  nsCOMPtr<nsIThread>                     mCallingThread;
  nsRefPtr<sbSongkickResultEnumerator>    mResultsEnum;
  nsRefPtr<sbSongkickDBService>           mDBService;
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
  NS_ENSURE_TRUE(mDBService, NS_ERROR_UNEXPECTED);

  // First, notify the enum callback of enumeration start.
  nsresult rv;
  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbSongkickQuery, this, RunEnumCallbackStart);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);
  rv = mCallingThread->Dispatch(runnable, NS_DISPATCH_SYNC);

  // Setup the database query.
  nsCOMPtr<sbIDatabaseQuery> db;
  rv = mDBService->GetDatabaseQuery(getter_AddRefs(db));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = db->AddQuery(mQueryStmt);
  NS_ENSURE_SUCCESS(rv, rv);
  {
    // Try and save deadlocking the database...
    nsAutoLock lock(mDBService->mQueryRunningLock);
    
    // Phew, finally send of the query.
    PRInt32 queryResult;
    rv = db->Execute(&queryResult);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(queryResult == 0, NS_ERROR_FAILURE);
  }

  // Convert any results into the appropriate type.
  nsCOMPtr<sbIDatabaseResult> results;
  rv = db->GetResultObject(getter_AddRefs(results));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create another db query for lookup up playing at data while enumerating
  // through the current results.
  nsCOMPtr<sbIDatabaseQuery> artistDB;
  rv = mDBService->GetDatabaseQuery(getter_AddRefs(artistDB));
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
                      sbSongkickDBService *aDBService,
                      sbISongkickEnumeratorCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  NS_ENSURE_ARG_POINTER(aDBService);
 
  mQueryStmt = aQueryStmt;
  mDBService = aDBService;
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

//==============================================================================
// sbSongkickCountQuery
//==============================================================================

class sbSongkickCountQuery : public nsIRunnable
{
public:
  sbSongkickCountQuery();
  virtual ~sbSongkickCountQuery();

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  nsresult Init(const nsAString & aQueryStmt,
                sbSongkickDBService *aDBService,
                sbISongkickConcertCountCallback *aCallback);

  void RunEnumCountCallback();

private:
  nsCOMPtr<sbISongkickConcertCountCallback> mCallback;
  nsCOMPtr<nsIThread>                       mCallingThread;
  nsRefPtr<sbSongkickDBService>             mDBService;
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
                           sbSongkickDBService *aDBService,
                           sbISongkickConcertCountCallback *aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  NS_ENSURE_ARG_POINTER(aDBService);

  mDBService = aDBService;
  mCallback = aCallback;
  mQueryStmt = aQueryStmt;

  // Grab a reference to the calling thread.
  nsresult rv;
  rv = NS_GetCurrentThread(getter_AddRefs(mCallingThread));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickCountQuery::Run()
{
  nsresult rv;

  // Setup the database query.
  nsCOMPtr<sbIDatabaseQuery> db;
  rv = mDBService->GetDatabaseQuery(getter_AddRefs(db));
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

//==============================================================================
// sbSongkickDBService
//==============================================================================

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickDBService, sbPISongkickDBService)

sbSongkickDBService::sbSongkickDBService()
  : mQueryRunningLock(
      nsAutoLock::NewLock("sbSongkickDBService::mQueryRunningLock"))
{
  NS_ASSERTION(mQueryRunningLock, "Failed to create mQueryRunningLock");
}

sbSongkickDBService::~sbSongkickDBService()
{
  if (mQueryRunningLock) {
    nsAutoLock::DestroyLock(mQueryRunningLock);
  }
}

nsresult
sbSongkickDBService::Init()
{
  // Preload some data on a background thread.
  nsresult rv;
  mDBInfo = new sbSongkickDBInfo();
  rv = mDBInfo->Init(this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    "final-ui-startup",
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbSongkickDBService::LookupDBInfo()
{
  NS_ENSURE_TRUE(mDBInfo, NS_ERROR_UNEXPECTED);

  // Start data lookup.
  nsresult rv;
  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPoolService->Dispatch(mDBInfo, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbSongkickDBService::GetDatabaseQuery(sbIDatabaseQuery **aOutDBQuery)
{
  NS_ENSURE_ARG_POINTER(aOutDBQuery);

  // Setup the database query.
  nsresult rv;
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

  // Ensure that the database has been created first.
  nsCOMPtr<nsIFile> dbFile;
  rv = profileFile->Clone(getter_AddRefs(dbFile));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbFile->Append(NS_LITERAL_STRING("concerts.db"));
  NS_ENSURE_SUCCESS(rv, rv);

  bool exists = PR_FALSE;
  rv = dbFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!exists) {
    return NS_ERROR_NOT_AVAILABLE;
  }

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

  db.forget(aOutDBQuery);
  return NS_OK;
}

//------------------------------------------------------------------------------
// XPCOM Startup Registration

/* static */ NS_METHOD
sbSongkickDBService::RegisterSelf(nsIComponentManager *aCompMgr,
                                  nsIFile *aPath,
                                  const char *aLoaderStr,
                                  const char *aType,
                                  const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SONGBIRD_SONGKICKDBSERVICE_CLASSNAME,
                                "service,"
                                SONGBIRD_SONGKICKDBSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbPISongkickDBService

NS_IMETHODIMP
sbSongkickDBService::GetHasLocationInfo(bool *aOutHasLocationInfo)
{
  NS_ENSURE_ARG_POINTER(aOutHasLocationInfo);
  NS_ENSURE_TRUE(mDBInfo, NS_ERROR_UNEXPECTED);

  *aOutHasLocationInfo = mDBInfo->mGotLocationInfo;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::GetLocationCountries(nsIArray **aOutArray)
{
  NS_ENSURE_ARG_POINTER(aOutArray);
  NS_ENSURE_TRUE(mDBInfo, NS_ERROR_NOT_AVAILABLE);

  NS_IF_ADDREF(*aOutArray = mDBInfo->mCountriesProps);
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::ReloadLocationInfo(void)
{
  NS_ENSURE_TRUE(mDBInfo, NS_ERROR_UNEXPECTED);
  nsresult rv;
  
  rv = LookupDBInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::GetLocationStates(const nsAString & aCountry,
                                       nsIArray **aOutArray)
{
  NS_ENSURE_ARG_POINTER(aOutArray);

  // The state information is stored without a filter. Create an array
  // that has a subset of the state information based on the country.
  nsresult rv;
  nsCOMPtr<nsIMutableArray> statesArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

  nsCOMPtr<nsISimpleEnumerator> statesEnum;
  rv = mDBInfo->mStateProps->Enumerate(getter_AddRefs(statesEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(statesEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> curItem;
    rv = statesEnum->GetNext(getter_AddRefs(curItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISongkickProperty> curProp =
      do_QueryInterface(curItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString curCountry;
    rv = curProp->GetKey(curCountry);
    NS_ENSURE_SUCCESS(rv, rv);

    if (curCountry.Equals(aCountry)) {
      rv = statesArray->AppendElement(curProp, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = CallQueryInterface(statesArray, aOutArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::GetLocationCities(const nsAString & aState,
                                       nsIArray **aOutArray)
{
  NS_ENSURE_ARG_POINTER(aOutArray);

  // The state information is stored without a filter. Create an array
  // that has a subset of the state information based on the country.
  nsresult rv;
  nsCOMPtr<nsIMutableArray> citiesArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

  nsCOMPtr<nsISimpleEnumerator> cityEnum;
  rv = mDBInfo->mCityProps->Enumerate(getter_AddRefs(cityEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  bool hasMore = PR_FALSE;
  while (NS_SUCCEEDED(cityEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> curItem;
    rv = cityEnum->GetNext(getter_AddRefs(curItem));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISongkickProperty> curProp =
      do_QueryInterface(curItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString curState;
    rv = curProp->GetKey(curState);
    NS_ENSURE_SUCCESS(rv, rv);

    if (curState.Equals(aState)) {
      rv = citiesArray->AppendElement(curProp, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = CallQueryInterface(citiesArray, aOutArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbSongkickDBService::StartAristConcertLookup(
    bool aFilter,
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
  rv = skQuery->Init(stmt, this, aCallback);
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
    bool aFilter,
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
  rv = skQuery->Init(stmt, this, aCallback);
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
    bool aFilter,
    bool aGroupByArtist,
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

  rv = skQuery->Init(stmt, this, aCallback);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIThreadPool> threadPoolService =
    do_GetService("@songbirdnest.com/Songbird/ThreadPoolService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = threadPoolService->Dispatch(skQuery, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbSongkickDBService::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  if (strcmp(aTopic, "final-ui-startup") == 0) {
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = LookupDBInfo();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


//==============================================================================
// sbSongkickConcertInfo
//==============================================================================

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

//==============================================================================
// sbSongkickArtistConcertInfo
//==============================================================================

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

//==============================================================================
// sbSongkickProperty
//==============================================================================

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSongkickProperty, sbISongkickProperty)

sbSongkickProperty::sbSongkickProperty()
{
}

sbSongkickProperty::~sbSongkickProperty()
{
}

nsresult
sbSongkickProperty::Init(const nsAString & aName,
                         const nsAString & aID,
                         const nsAString & aKey)
{
  mName = aName;
  mID = aID;
  mKey = aKey;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickProperty::GetName(nsAString & aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickProperty::GetId(nsAString & aID)
{
  aID = mID;
  return NS_OK;
}

NS_IMETHODIMP
sbSongkickProperty::GetKey(nsAString & aKey)
{
  aKey = mKey;
  return NS_OK;
}

