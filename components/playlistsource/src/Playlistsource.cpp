/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/** 
* \file  Playlistsource.cpp
* \brief Songbird Playlistsource Component Implementation.
*/

#pragma warning(push)
#pragma warning(disable:4800)

#include "nscore.h"
#include "prlog.h"
#include "nspr.h"
#include "nsCOMPtr.h"
#include "rdf.h"
#include "nsIEnumerator.h"
#include "nsIRDFService.h"
#include "nsIRDFDataSource.h"
#include "nsIRDFNode.h"
#include "nsIRDFObserver.h"
#include "nsIInterfaceRequestor.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIContent.h"
#include "nsIXULTemplateBuilder.h"
#include "nsITimer.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsEnumeratorUtils.h"
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsIFile.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsAutoLock.h>
#include <xpcom/nsVoidArray.h>
#include <xpcom/nsCOMArray.h>
#include <xpcom/nsArrayEnumerator.h>
#include "nsCRT.h"
#include "nsRDFCID.h"
#include "nsLiteralString.h"

#include "Playlistsource.h"
#include "IPlaylist.h"

#define LOOKAHEAD_SIZE 30

#define MODULE_SHORTCIRCUIT 0

#if MODULE_SHORTCIRCUIT
#define METHOD_SHORTCIRCUIT  return NS_OK;
#define VMETHOD_SHORTCIRCUIT return;
#else
#define METHOD_SHORTCIRCUIT  /* nothing */
#define VMETHOD_SHORTCIRCUIT /* nothing */
#endif

#ifdef PR_LOGGING
/*
 * NSPR_LOG_MODULES=sbPlaylistsource:5
 */
static PRLogModuleInfo* gPlaylistsourceLog =
  PR_NewLogModule("sbPlaylistsource");
#endif

#define LOG(args) PR_LOG(gPlaylistsourceLog, PR_LOG_DEBUG, args)

static sbPlaylistsource* gPlaylistPlaylistsource = nsnull;

sbPlaylistsource::observers_t  sbPlaylistsource::g_Observers;
sbPlaylistsource::stringmap_t  sbPlaylistsource::g_StringMap;
sbPlaylistsource::infomap_t    sbPlaylistsource::g_InfoMap;
sbPlaylistsource::valuemap_t   sbPlaylistsource::g_ValueMap;
sbPlaylistsource::resultlist_t sbPlaylistsource::g_ResultGarbage;
sbPlaylistsource::commandmap_t sbPlaylistsource::g_CommandMap;

PRMonitor* sbPlaylistsource::g_pMonitor = nsnull;

PRInt32 sbPlaylistsource::g_ActiveQueryCount = 0;
PRInt32 sbPlaylistsource::sRefCnt = 0;

PRBool sbPlaylistsource::g_NeedUpdate = PR_FALSE;

nsCOMPtr<nsIStringBundle> sbPlaylistsource::m_StringBundle;

// Strings that we'll use to compose all the goofy queries.
NS_NAMED_LITERAL_STRING(select_str, "select ");
NS_NAMED_LITERAL_STRING(unique_str, "select distinct");
NS_NAMED_LITERAL_STRING(row_str, "id");
NS_NAMED_LITERAL_STRING(rowid_str, "_ROWID_");
NS_NAMED_LITERAL_STRING(from_str, " from ");
NS_NAMED_LITERAL_STRING(where_str, " where ");
NS_NAMED_LITERAL_STRING(having_str, " having ");
NS_NAMED_LITERAL_STRING(like_str, " like ");
NS_NAMED_LITERAL_STRING(order_str, " order by ");
NS_NAMED_LITERAL_STRING(left_join_str, " left join ");
NS_NAMED_LITERAL_STRING(right_join_str, " right join ");
NS_NAMED_LITERAL_STRING(inner_join_str, " inner join ");
NS_NAMED_LITERAL_STRING(collate_binary_str, " collate binary ");
NS_NAMED_LITERAL_STRING(on_str, " on ");
NS_NAMED_LITERAL_STRING(using_str, " using");
NS_NAMED_LITERAL_STRING(limit_str, " limit ");
NS_NAMED_LITERAL_STRING(op_str, "(");
NS_NAMED_LITERAL_STRING(cp_str, ")");
NS_NAMED_LITERAL_STRING(eq_str, "=");
NS_NAMED_LITERAL_STRING(ge_str, ">=");
NS_NAMED_LITERAL_STRING(qu_str, "\"");
NS_NAMED_LITERAL_STRING(pct_str, "%");
NS_NAMED_LITERAL_STRING(dot_str, ".");
NS_NAMED_LITERAL_STRING(comma_str, ",");
NS_NAMED_LITERAL_STRING(spc_str, " ");
NS_NAMED_LITERAL_STRING(and_str, " and ");
NS_NAMED_LITERAL_STRING(or_str, " or ");
NS_NAMED_LITERAL_STRING(uuid_str, "uuid");
NS_NAMED_LITERAL_STRING(playlist_uuid_str, "playlist_uuid");
NS_NAMED_LITERAL_STRING(library_str, "library");

// CLASSES ====================================================================

// A callback from the database for when things change and we should repaint.
NS_IMPL_ISUPPORTS1(MyQueryCallback, sbIDatabaseSimpleQueryCallback)
//-----------------------------------------------------------------------------
MyQueryCallback::MyQueryCallback()
{
  //
  //
  // IN THE UI THREAD.
  //
  //
  LOG(("MyQueryCallback::MyQueryCallback"));
  nsresult rv;
  m_Timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create timer for MyQueryCallback!");

  m_pMonitor = nsAutoMonitor::NewMonitor("MyQueryCallback.m_pMonitor");
  NS_ASSERTION(m_pMonitor, "Failed to create local monitor");
  m_Count = 0;
}

MyQueryCallback::~MyQueryCallback()
{
  LOG(("MyQueryCallback::~MyQueryCallback"));
  if (m_pMonitor) {
    nsAutoMonitor::DestroyMonitor(m_pMonitor);
    m_pMonitor = nsnull;
  }
}

NS_IMETHODIMP
MyQueryCallback::OnQueryEnd(sbIDatabaseResult* dbResultObject,
                            const PRUnichar*   dbGUID,
                            const PRUnichar*   strQuery)
{
  //
  //
  // IN A DATABASE THREAD.
  //
  //
  LOG(("MyQueryCallback::OnQueryEnd"));
  NS_ENSURE_ARG_POINTER(dbResultObject);
  NS_ENSURE_ARG_POINTER(dbGUID);
  NS_ENSURE_ARG_POINTER(strQuery);

/*
  nsAutoMonitor mon_local(m_pMonitor);

  m_Results.push_back( nsCOMPtr< sbIDatabaseResult >( dbResultObject ) );

  PRInt32 rowcount;
  dbResultObject->GetRowCount( &rowcount );
  printf( "- MyQueryCallback(0x%08X) -- %d rows\n", dbResultObject, rowcount );
*/

  NS_ENSURE_TRUE(gPlaylistPlaylistsource, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(m_Timer, NS_ERROR_OUT_OF_MEMORY);

  if ( m_Count++ == 0 )
  {
    // Tell us to wake up in the main thread so we can poke our observers and
    // take out our garbage.

    // XXX This is NOT a safe or reliable way to post events to another thread
    m_Timer->InitWithFuncCallback(&MyTimerCallbackFunc, this,
                                  0, nsITimer::TYPE_ONE_SHOT);
  }

  return NS_OK;
}

void
MyQueryCallback::MyTimerCallbackFunc(nsITimer* aTimer,
                                     void*     aClosure)
{
  //
  //
  // IN THE UI THREAD.
  //
  //
  MyQueryCallback *that = reinterpret_cast<MyQueryCallback *>(aClosure);
  that->MyTimerCallback( aTimer, aClosure );
}

void
MyQueryCallback::MyTimerCallback(nsITimer* aTimer,
                                     void*     aClosure)
{
  //
  //
  // IN THE UI THREAD.
  //
  //
  PRInt32 count = 0;
  { // SCOPE ADDED TO CONSTRAIN AUTOMONITORS
    LOG(("MyQueryCallback::MyTimerCallbackFunc"));
    NS_ASSERTION(gPlaylistPlaylistsource,
      "MyQueryCallback timer fired before Playlistsource initialized");

    // LOCK IT.
    nsAutoMonitor mon_global(sbPlaylistsource::g_pMonitor);

    if (!m_Info) {
      NS_WARNING("No m_Info on this Playlistsource::MyQueryCallback object!!!");
      return;
    }
    
    PRBool exec;
    // If we're executing, that means we expect another callback.
    m_Info->m_Query->IsExecuting( &exec );
    if ( exec )
    {
      // Just ignore this one, but always go on the next one.
      sbPlaylistsource::g_ActiveQueryCount = 0;
      m_Timer->InitWithFuncCallback(&MyTimerCallbackFunc, this,
                                    100, nsITimer::TYPE_ONE_SHOT);
      return;
    }

    nsAutoMonitor mon_local(m_pMonitor);
    {
      count = m_Count;
      m_Count = 0;
    }

    // This is ok by design
    if (sbPlaylistsource::g_ActiveQueryCount < 0)
      sbPlaylistsource::g_ActiveQueryCount = 0;

    sbPlaylistsource::sbResultInfo result;

    // sometimes we don't have one yet.
    if (m_Info->m_Resultset) {
      // Copy the old resultset
      result.m_Results = m_Info->m_Resultset;
    } else {
      m_Info->m_Query->GetResultObjectOrphan(getter_AddRefs(result.m_Results));
    }
    result.m_Source = m_Info->m_RootResource;
    result.m_OldTarget = m_Info->m_RootTargets;
    result.m_Ref = m_Info->m_Ref;
    result.m_ForceGetTargets = m_Info->m_ForceGetTargets;

    // Push the old resultset onto the garbage stack.
    sbPlaylistsource::g_ResultGarbage.push_back(result);

#if 0
    if ( ! m_Results.empty() )
    {
      m_Info->m_Resultset = *( m_Results.rbegin() );
      m_Results.clear();
    }
#else
    // Orphan the result for the query.
    nsCOMPtr<sbIDatabaseResult> res;
    nsresult rv = m_Info->m_Query->GetResultObjectOrphan(getter_AddRefs(res));
    if ( NS_SUCCEEDED( rv ) )
      m_Info->m_Resultset = res;
#endif
  }
  // Decrement and update if needbe.
  sbPlaylistsource::g_ActiveQueryCount -= count;
  if (sbPlaylistsource::g_ActiveQueryCount <= 0) {
    sbPlaylistsource::g_ActiveQueryCount = 0;
    sbPlaylistsource::g_NeedUpdate = PR_TRUE;
    gPlaylistPlaylistsource->UpdateObservers();
  }
}


//-----------------------------------------------------------------------------

// Playlistsource
NS_IMPL_ISUPPORTS2(sbPlaylistsource, sbIPlaylistsource, nsIRDFDataSource)
//-----------------------------------------------------------------------------
sbPlaylistsource::sbPlaylistsource()
{
  LOG(("sbPlaylistsource::sbPlaylistsource"));

  NS_ASSERTION(!gPlaylistPlaylistsource,
               "Playlistsource initialized more than once!");

  if (!g_pMonitor)
    g_pMonitor = nsAutoMonitor::NewMonitor("sbPlaylistsource.g_pMonitor");

  NS_ASSERTION(g_pMonitor, "sbPlaylistsource.g_pMonitor failed");

  nsresult rv = Init();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Init failed!");
}

//-----------------------------------------------------------------------------
sbPlaylistsource::~sbPlaylistsource()
{
  LOG(("sbPlaylistsource::~sbPlaylistsource"));

  nsresult rv = DeInit();
  NS_ASSERTION(NS_SUCCEEDED(rv), "DeInit failed!");

  if (g_pMonitor) {
    nsAutoMonitor::DestroyMonitor(g_pMonitor);
    g_pMonitor = nsnull;
  }
}

NS_IMETHODIMP
sbPlaylistsource::FeedPlaylist(const nsAString &aRefName,
                               const nsAString &aContextGUID,
                               const nsAString &aTableName)
{
  LOG(("sbPlaylistsource::FeedPlaylist"));

  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  // See if the feed is already there.
  sbFeedInfo* testinfo = GetFeedInfo(aRefName);
  if (testinfo) {
    // Found it.  Get the refcount and increment it.
    testinfo->m_RefCount++;
    // Clear whatever might have been there before.
    testinfo->m_Resultset = nsnull;
    return NS_OK;
  }

  // A new feed.  Fire a new Query object and stuff it in the tables.
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ENSURE_TRUE(query, NS_ERROR_FAILURE);

  nsresult rv;

  rv = query->SetAsyncQuery(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(nsPromiseFlatString(aContextGUID).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // New callback for a new query.
  MyQueryCallback* callback = nsnull;
  NS_NEWXPCOM(callback, MyQueryCallback);
  NS_ENSURE_TRUE(callback, NS_ERROR_OUT_OF_MEMORY);

  rv = callback->AddRef();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = query->AddSimpleQueryCallback(callback);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetPersistentQuery(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make a resource for it.
  nsIRDFResource *new_resource;
  rv = sRDFService->GetResource(NS_ConvertUTF16toUTF8(aRefName),
                                &new_resource);
  NS_ENSURE_SUCCESS(rv, rv);

  // And stuff it.
  sbFeedInfo info;
  info.m_Query = query;
  info.m_RefCount = 1;
  info.m_Ref = aRefName;
  info.m_Table = aTableName;
  info.m_GUID = aContextGUID;
  info.m_RootResource = new_resource;
  info.m_Server = this;
  info.m_Callback = callback;
  info.m_ForceGetTargets = PR_FALSE;
  g_InfoMap[new_resource] = info;
  callback->m_Info = &g_InfoMap[new_resource];
  g_StringMap[nsPromiseFlatString(aRefName)] = new_resource;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::ClearPlaylist(const nsAString &aRefName)
{
  LOG(("sbPlaylistsource::ClearPlaylist"));

  METHOD_SHORTCIRCUIT;

  ClearPlaylistSTR(nsPromiseFlatString(aRefName).get());

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::IncomingObserver(const nsAString &aRefName,
                                   nsIDOMNode*      aObject)
{
  LOG(("sbPlaylistsource::IncomingObserver %s %x", NS_ConvertUTF16toUTF8(RefName).get(), Observer));
  NS_ENSURE_ARG_POINTER(aObject);

  METHOD_SHORTCIRCUIT;

  // See if the object is already observing us
  for (observers_t::iterator oi = g_Observers.begin();
       oi != g_Observers.end();
       oi++ ) {
    // Find the pointer?
    if ((*oi).m_Ptr == aObject) {
      const_cast<sbObserver&>(*oi).m_Ref = aRefName;
      // Assume that no AddObserver call will occur, since we're already in the list.
      return NS_OK;
    }
  }
  // Not found.  Prep the object to assign these to the next incoming observer.
  m_IncomingObserver = aRefName;
  m_IncomingObserverPtr = aObject;
  return NS_OK;
}

void
sbPlaylistsource::ClearPlaylistSTR(const PRUnichar* RefName)
{
  LOG(("sbPlaylistsource::ClearPlaylistSTR"));
  NS_WARN_IF_FALSE(RefName, "ClearPlaylistSTR passed a null pointer");
  if (!RefName)
    return;

  VMETHOD_SHORTCIRCUIT;

  // Find the ref string in the stringmap.
  nsDependentString strRefName(RefName);
  stringmap_t::iterator s = g_StringMap.find(strRefName);
  if (s != g_StringMap.end())
    ClearPlaylistRDF((*s).second);
}

void
sbPlaylistsource::ClearPlaylistRDF(nsIRDFResource* RefResource)
{
  LOG(("sbPlaylistsource::ClearPlaylistRDF"));
  NS_WARN_IF_FALSE(RefResource, "ClearPlaylistRDF passed a null pointer");
  if (!RefResource)
    return;

  VMETHOD_SHORTCIRCUIT;

  sbFeedInfo* info = GetFeedInfo(RefResource);
  if (info) {
    // Found it.  Get the refcount and decrement it.
    if (--info->m_RefCount == 0)
      EraseFeedInfo( RefResource );
  }
}

NS_IMETHODIMP
sbPlaylistsource::GetRefRowCount(const nsAString &aRefName,
                                 PRInt32*         _retval)
{
  LOG(("sbPlaylistsource::GetRefRowCount %s", NS_ConvertUTF16toUTF8(RefName).get()));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;
  *_retval = -1;

  nsresult rv;

  nsCOMPtr<sbIDatabaseResult> resultset;
  rv = GetQueryResult(aRefName, getter_AddRefs(resultset));

  if (NS_SUCCEEDED(rv) && resultset)
    resultset->GetRowCount(_retval);

  LOG(("   count: %d", *_retval));
  return NS_OK;
}

/* wstring GetRefGUID (in wstring RefName); */
NS_IMETHODIMP sbPlaylistsource::GetRefGUID(const nsAString &aRefName, nsAString &_retval)
{
  LOG(("sbPlaylistsource::GetRefGUID"));

  METHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);

  // Find the ref string in the stringmap.
  sbFeedInfo* info = GetFeedInfo(aRefName);

  if (info) {
    _retval = info->m_GUID;
    return NS_OK;
  }

  // in no info, blank string
  _retval = NS_LITERAL_STRING("");

  return NS_OK;
}

/* wstring GetRefTable (in wstring RefName); */
NS_IMETHODIMP sbPlaylistsource::GetRefTable(const nsAString &aRefName, nsAString &_retval)
{
  LOG(("sbPlaylistsource::GetRefTable"));

  METHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);

  // Find the ref string in the stringmap.
  sbFeedInfo* info = GetFeedInfo(aRefName);

  if (info) {
    _retval = info->m_Table;
    return NS_OK;
  }

  // in no info, blank string
  _retval = NS_LITERAL_STRING("");

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetRefColumnCount(const nsAString &aRefName,
                                    PRInt32*         _retval)
{
  LOG(("sbPlaylistsource::GetRefColumnCount %s", NS_ConvertUTF16toUTF8(RefName).get()));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;
  *_retval = -1;

  nsresult rv;

  nsCOMPtr<sbIDatabaseResult> resultset;
  rv = GetQueryResult(aRefName, getter_AddRefs(resultset));

  if (NS_SUCCEEDED(rv) && resultset)
    resultset->GetColumnCount(_retval);

  LOG(("   count: %d", *_retval));
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetRefRowCellByColumn(const nsAString &aRefName,
                                        PRInt32          aRow,
                                        const nsAString &aColumn,
                                        nsAString       &_retval)
{
  LOG(("sbPlaylistsource::GetRefRowCellByColumn"));

  _retval = NS_LITERAL_STRING("");

  METHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);

  // Find the ref string in the stringmap.
  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  NS_ASSERTION(aRow < info->m_ResList.size(), "sbPlaylistsource::GetRefRowCellByColumn, Row is out of bounds!");
  if(aRow >= (PRInt32)info->m_ResList.size() && info->m_ResList.size() != 0) aRow = info->m_ResList.size() - 1;
  else if(info->m_ResList.size() == 0) return NS_OK;

  nsCOMPtr<nsIRDFResource> next_resource = info->m_ResList[aRow];
  valuemap_t::iterator v = g_ValueMap.find(next_resource);
  if (v != g_ValueMap.end()) {
    sbValueInfo &valueInfo = (*v).second;
    nsresult rv;
    if (!valueInfo.m_Resultset) {
      LoadRowResults(valueInfo, mon);
    }
    // The LoadRowResults above will fail silently if the table has been deleted and the UI is still asking for values.
    if (valueInfo.m_Resultset) {
      PRUnichar *val = nsnull;
      rv = valueInfo.m_Resultset->GetRowCellByColumn(valueInfo.m_ResultsRow,
                                                    nsPromiseFlatString(aColumn).get(),
                                                    &val);
      _retval = val;
      nsMemory::Free(val);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}


NS_IMETHODIMP
sbPlaylistsource::GetRefRowByColumnValue(const nsAString &aRefName,
                                         const nsAString &aColumn,
                                         const nsAString &aValue,
                                         PRInt32*         _retval)
{
  LOG(("sbPlaylistsource::GetRefRowByColumnValue"));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);

  *_retval = -1;

  // Find the ref string in the stringmap.
  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  nsresult rv;

  // Steal the query info used for the ref
  m_SharedQuery->ResetQuery();

  PRUnichar* guidPtr = nsnull;
  rv = info->m_Query->GetDatabaseGUID(&guidPtr);
  NS_ENSURE_SUCCESS(rv, rv);
  nsDependentString guid(guidPtr);

  rv = m_SharedQuery->SetDatabaseGUID(guid.get());
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar* queryPtr = nsnull;
  rv = info->m_Query->GetQuery(0, &queryPtr);
  NS_ENSURE_SUCCESS(rv, rv);
  nsDependentString query(queryPtr);

  nsAString::const_iterator start, end;
  query.BeginReading(start);
  query.EndReading(end);

  // If we already have a where, don't add another one
  nsAutoString aw_str;
  if (!FindInReadable(NS_LITERAL_STRING("where"), start, end))
    aw_str = NS_LITERAL_STRING(" where ");
  else
    aw_str = NS_LITERAL_STRING(" and ");

  // Append the metadata column restraint
  query += aw_str + aColumn + eq_str + qu_str + aValue + qu_str;

  PRInt32 exeReturn;
  rv = m_SharedQuery->AddQuery(query.get());
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();
  rv = m_SharedQuery->Execute(&exeReturn);
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();

  nsCOMPtr<sbIDatabaseResult> result;
  rv = m_SharedQuery->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NULL_POINTER);

  PRUnichar* val;
  rv = result->GetRowCell(0, 0, &val);
  if ( !val ) return NS_OK; // Whoa.
  NS_ENSURE_SUCCESS(rv, rv);
  nsDependentString v(val);

  // (sigh) Now linear search the info results object for the matching id value
  // to get the result index
  PRInt32 i, rowcount;
  rv = info->m_Resultset->GetRowCount(&rowcount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool found = PR_FALSE;
  PRUnichar *newval;
  for (i = 0 ; i < rowcount; i++ ) {
    rv = info->m_Resultset->GetRowCell(i, 0, &newval);
    NS_ENSURE_SUCCESS(rv, rv);
    nsDependentString test(newval);

    if (v == test)
      found = PR_TRUE;

    PR_Free(newval);
    newval = nsnull;

    if (found)
      break;
  }

  PR_Free(val);

  if (found)
    *_retval = i;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::IsQueryExecuting(const nsAString &aRefName,
                                   PRBool*          _retval)
{
  LOG(("sbPlaylistsource::IsQueryExecuting"));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;
  *_retval = PR_FALSE;

  // Find the ref string in the stringmap.
  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  return info->m_Query->IsExecuting(_retval);
}

// Adapted from nsNavHistory.cpp
NS_IMETHODIMP
ParseStringIntoArray(const nsString& aString,
                     nsStringArray*  aArray, 
                     PRUnichar aDelim)
{
  LOG(("ParseStringIntoArray"));
  NS_ENSURE_ARG_POINTER(aArray);

  aArray->Clear();

  PRUint32 length = aString.Length();
  if (length == 0)
    return NS_OK;

  PRInt32 lastBegin = -1;

  for (PRUint32 i = 0; i < length; i++) {
    if (aString[i] == aDelim) {
      if (lastBegin >= 0) {
        // found the end of a word
        aArray->AppendString(Substring(aString, lastBegin, i - lastBegin));
        lastBegin = -1;
      }
    } else {
      if (lastBegin < 0) {
        // found the beginning of a word
        lastBegin = i;
      }
    }
  }

  // last word
  if (lastBegin >= 0)
    aArray->AppendString(Substring(aString, lastBegin));

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::SetSearchString(const nsAString &aRefName,
                                  const nsAString &aSearchString)
{
  LOG(("sbPlaylistsource::SetSearchString"));

  METHOD_SHORTCIRCUIT;

  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(info->m_Resultset, NS_ERROR_UNEXPECTED);

  // Check for 0 columns
  PRInt32 col_count = 0;
  nsresult rv  = info->m_Resultset->GetColumnCount(&col_count);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXredfive may want to pull this out into a helper function later
  nsAString::const_iterator start, end;
  aSearchString.BeginReading(start);
  aSearchString.EndReading(end);

  // check to see if the new query is more restrictive
  PRBool subquery = FindInReadable(info->m_Override, start, end);

  if ( col_count == 0 && subquery ) {
    // we have no results and the filter string is at least as
    //  restrictive meaning we won't have a chance at getting
    //  more results if we quering the entire table.
    mon.Exit();
    return NS_OK;
  }

  info->m_Override = aSearchString;

  if (aSearchString.IsEmpty()) {
    // Revert to the normal override.
    mon.Exit();
    return ExecuteFeed(aRefName, nsnull);
  }

  // Crack the incoming list of filter strings (space delimited)
  nsStringArray filter_values;
  rv = ParseStringIntoArray(nsPromiseFlatString(aSearchString), &filter_values, NS_L(' '));
  NS_ENSURE_SUCCESS(rv, rv);

  nsDependentString table_name(info->m_Table);

  // The beginning of the main query string before we dump on it.
  nsAutoString main_query_str = info->m_SimpleQueryStr + where_str;

  PRInt32 filter_count = (PRInt32)info->m_Filters.size();
  PRBool any_column = PR_FALSE;

  // Compose an override query from the filter string and the columns in the
  // current results
  if (filter_count) {
    // We're going to submit n+1 queries;
    g_ActiveQueryCount += filter_count + 1;

    filtermap_t::iterator f = info->m_Filters.begin();
    for (; f != info->m_Filters.end(); f++) {
      // Compose an override string for the filter query.
      // select unique ( artist ) from "library" where (
      nsAutoString sub_query_str = unique_str + op_str + (*f).second.m_Column +
                                   cp_str + from_str + qu_str + table_name +
                                   qu_str + where_str + op_str;
      PRBool any_value = PR_FALSE;
      PRInt32 count = filter_values.Count();
      for (PRInt32 index = 0; index < count; index++) {
        if (any_value)
          sub_query_str += and_str;
        any_value = PR_TRUE;
        // ( artist like "%cc%" )
        sub_query_str += op_str + (*f).second.m_Column + like_str + qu_str +
                         pct_str + *filter_values[index] + pct_str + qu_str +
                         cp_str;
      }
      // ) order by artist
      sub_query_str += cp_str + order_str + (*f).second.m_Column;

      sbFeedInfo* filter_info = GetFeedInfo((*f).second.m_Ref);
      NS_ENSURE_TRUE(filter_info, NS_ERROR_NULL_POINTER);

      filter_info->m_Override = aSearchString;
      rv = filter_info->m_Query->ResetQuery();
      NS_ENSURE_SUCCESS(rv, rv);

      rv = filter_info->m_Query->AddQuery(sub_query_str.get());
      NS_ENSURE_SUCCESS(rv, rv);

      PRInt32 ret;
      rv = filter_info->m_Query->Execute(&ret);
      NS_ENSURE_SUCCESS(rv, rv);
    }
 
    PRBool any_value = PR_FALSE;
    PRInt32 count = filter_values.Count();
    for (PRInt32 index = 0; index < count; index++) {
      if (any_value)
        main_query_str += and_str;
      any_value = PR_TRUE;
      any_column = PR_FALSE;
      main_query_str += op_str;

      for (f = info->m_Filters.begin(); f != info->m_Filters.end(); f++) {
        // Add to the main query override string
        if (any_column)
          main_query_str += or_str;
        any_column = PR_TRUE;
        // ( artist like "%cc%" )
        main_query_str += op_str + (*f).second.m_Column + like_str + qu_str +
                          pct_str + *filter_values[index] + pct_str + qu_str +
                          cp_str;
      }

      // or ( title like "%cc%" ) )
      main_query_str += or_str + op_str + NS_LITERAL_STRING("title") +
                        like_str + qu_str + pct_str + *filter_values[index] +
                        pct_str + qu_str + cp_str + cp_str;
    }
  } // filter_count
  else { 
    // do it from the actual columns if there's no filters.
    PRBool any_value = PR_FALSE;
    PRInt32 count = filter_values.Count();
    for (PRInt32 index = 0; index < count; index++) {
      if (any_value)
        main_query_str += and_str;
      else
        main_query_str += op_str;
      any_value = PR_TRUE;
      any_column = PR_FALSE;
      // ( title like "%cc%" or genre like "%cc%" or artist like "%cc%"
      //     or album like "%cc%" )
      main_query_str += op_str + NS_LITERAL_STRING("title") + like_str +
                        qu_str + pct_str + *filter_values[index] + pct_str +
                        qu_str + or_str + NS_LITERAL_STRING("genre") +
                        like_str + qu_str + pct_str + *filter_values[index] +
                        pct_str + qu_str + or_str +
                        NS_LITERAL_STRING("artist") + like_str + qu_str +
                        pct_str + *filter_values[index] + pct_str + qu_str +
                        or_str + NS_LITERAL_STRING("album") + like_str +
                        qu_str + pct_str + *filter_values[index] + pct_str +
                        qu_str + cp_str;
    }
    if (any_value)
      main_query_str += cp_str;
  }

  mon.Exit();

  PRInt32 ret;
  rv = info->m_Query->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = info->m_Query->AddQuery( main_query_str.get() );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = info->m_Query->Execute( &ret );
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetSearchString(const nsAString &aRefName,
                                  nsAString       &_retval)
{
  LOG(("sbPlaylistsource::GetSearchString"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  _retval = info->m_Override;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::SetFilter(const nsAString &aRefName,
                            PRInt32          aIndex,
                            const nsAString &aFilterString,
                            const nsAString &aFilterRefName,
                            const nsAString &aFilterColumn)
{
  LOG(("sbPlaylistsource::SetFilter"));

  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);
  if (!info)
    return NS_OK;

  filtermap_t::iterator filterIter = info->m_Filters.find(aIndex);
  if (filterIter != info->m_Filters.end()) {
    // Change the existing strings
    (*filterIter).second.m_Filter = aFilterString;
    (*filterIter).second.m_Column = aFilterColumn;
    (*filterIter).second.m_Ref    = aFilterRefName;
  } else {
    // Make a new one, add it to the playlist feed
    sbFilterInfo filter;
    filter.m_Filter = aFilterString;
    filter.m_Column = aFilterColumn;
    filter.m_Ref    = aFilterRefName;
    info->m_Filters[ aIndex ] = filter;

    filterIter = info->m_Filters.find(aIndex);
  }

  // Set all the later filters blank.
  for (filterIter++; filterIter != info->m_Filters.end(); filterIter++) {
    (*filterIter).second.m_Filter = nsString();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetNumFilters(const nsAString &aRefName,
                                PRInt32*         _retval)
{
  LOG(("sbPlaylistsource::GetNumFilters"));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;
  *_retval = -1;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);

  if (info)
    *_retval = (PRInt32)info->m_Filters.size();

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::ClearFilter(const nsAString &aRefName,
                              PRInt32          aIndex)
{
  LOG(("sbPlaylistsource::ClearFilter"));

  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  filtermap_t::iterator f = info->m_Filters.find(aIndex);
  if (f != info->m_Filters.end()) {
      // Remember to make the destructor clean up
      info->m_Filters.erase(f);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetFilter(const nsAString &aRefName,
                            PRInt32          aIndex,
                            nsAString       &_retval)
{
  LOG(("sbPlaylistsource::GetFilter"));

  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);
  if (info && info->m_Override.IsEmpty()) {
    // Find the specific filter
    filtermap_t::iterator f = info->m_Filters.find(aIndex);
    if (f != info->m_Filters.end()) {
      // And return the filter string
      _retval = (*f).second.m_Filter;
      return NS_OK;
    }
  }

  // If we have an override, pretend the filter string is blank.
  _retval = NS_LITERAL_STRING("");

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetFilterRef(const nsAString &aRefName,
                               PRInt32          aIndex,
                               nsAString       &_retval)
{
  LOG(("sbPlaylistsource::GetFilterRef"));

  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);

  if (info) {
    filtermap_t::iterator f = info->m_Filters.find(aIndex);
    if (f != info->m_Filters.end()) {
      _retval = (*f).second.m_Ref;
      return NS_OK;
    }
  }

  // in no info, blank string
  _retval = NS_LITERAL_STRING("");

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::ExecuteFeed(const nsAString &aRefName,
                              PRInt32*         _retval)
{
  LOG(("sbPlaylistsource::ExecuteFeed"));

  METHOD_SHORTCIRCUIT;

  // XXX Why does this need a retval param if we're not going to use it?
  // MIG Because it is used.  Waaaaaay down at the bottom.  You just don't
  // __have__ to care.  So if you pass in a null, we set this pointer to a
  // stack variable and discard it at the end of the function.
  PRInt32 retval;
  if (!_retval)
    _retval = &retval;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  sbFeedInfo* info = GetFeedInfo(aRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  info->m_Override.Assign(NS_LITERAL_STRING(""));
  nsAutoString table_name(info->m_Table);

  nsCOMPtr<sbISimplePlaylist> pSimplePlaylist;
  nsCOMPtr<sbISmartPlaylist> pSmartPlaylist;

  nsCOMPtr<sbIDatabaseQuery> pQuery =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ENSURE_TRUE(pQuery, NS_ERROR_FAILURE);

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager =
    do_CreateInstance("@songbirdnest.com/Songbird/PlaylistManager;1");
  NS_ENSURE_TRUE(pPlaylistManager, NS_ERROR_FAILURE);

  nsresult rv;
  rv = pQuery->SetAsyncQuery(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pQuery->SetDatabaseGUID(info->m_GUID.get());
  NS_ENSURE_SUCCESS(rv, rv);


  mon.Exit();

  rv = pPlaylistManager->GetSimplePlaylist(table_name.get(),
                                           pQuery.get(),
                                           getter_AddRefs(pSimplePlaylist));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pPlaylistManager->GetSmartPlaylist(table_name.get(),
                                          pQuery.get(),
                                          getter_AddRefs(pSmartPlaylist));
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();

  PRBool isSimplePlaylist = pSimplePlaylist != nsnull;
  PRBool isSmartPlaylist = pSmartPlaylist != nsnull;

  nsAutoString library_query_str = select_str + row_str + from_str + qu_str +
                                   table_name + qu_str + where_str;
  nsAutoString library_simple_query_str = select_str + row_str + from_str +
                                          qu_str + table_name + qu_str;

  // The beginning of the main query string before we dump on it.
  nsAutoString playlist_simple_query_str = select_str + row_str + from_str +
                                           qu_str + table_name + qu_str +
                                           left_join_str + library_str +
                                           on_str + qu_str + table_name +
                                           qu_str + dot_str +
                                           playlist_uuid_str + eq_str +
                                           library_str + dot_str + uuid_str;

  PRBool useLibraryQuery = (table_name == library_str ||
                            isSimplePlaylist ||
                            isSmartPlaylist);
  nsAutoString simple_query_str = useLibraryQuery ?
                                  library_simple_query_str :
                                  playlist_simple_query_str;

  nsAutoString main_query_str = simple_query_str + where_str;

  info->m_SimpleQueryStr = simple_query_str;

  // The filters for the filters.
  nsAutoString sub_filter_str;

  // We're going to submit n+1 queries;
  g_ActiveQueryCount += (PRInt32)info->m_Filters.size() + 1;

  PRBool anything = PR_FALSE;
  filtermap_t::iterator filterIter = info->m_Filters.begin();
  for (; filterIter != info->m_Filters.end(); filterIter++) {
    // Crack the incoming list of filter strings (semicolon delimited)
    nsAutoString filter_str((*filterIter).second.m_Filter);
    nsStringArray filter_values;
    ParseStringIntoArray(filter_str, &filter_values, NS_L(';'));

    // Compose the SQL filter string
    nsAutoString sql_filter_str;
    PRBool any_filter = PR_FALSE;
    PRInt32 count = filter_values.Count();      
    for (PRInt32 index = 0; index < count; index++) {
      if (any_filter)
        sql_filter_str += or_str;
      sql_filter_str += (*filterIter).second.m_Column + eq_str + qu_str +
                        *filter_values[index] + qu_str;
      any_filter = PR_TRUE;
    }

    if (any_filter) {
      // Append this filter's constraints to the main query
      if (anything)
        main_query_str += and_str;
      main_query_str += op_str + sql_filter_str + cp_str;
    }

    // Compose the sub query
    nsAutoString sub_query_str = unique_str + op_str + (*filterIter).second.m_Column +
                                 cp_str + from_str + qu_str + table_name +
                                 qu_str;
    if (anything)
      sub_query_str += where_str + sub_filter_str;
    sub_query_str += order_str + (*filterIter).second.m_Column;

    // Append this filter's constraints to the next sub query
    if (any_filter) {
      if (anything)
        sub_filter_str += and_str;
      sub_filter_str += op_str + sql_filter_str + cp_str; 
    }

    // Make sure there is a feed for this filter
    sbFeedInfo* filter_info = GetFeedInfo((*filterIter).second.m_Ref);
    if (!filter_info) {
      FeedPlaylist((*filterIter).second.m_Ref,
                   info->m_GUID,
                   info->m_Table);
      filter_info = GetFeedInfo((*filterIter).second.m_Ref);
      NS_ENSURE_TRUE(filter_info, NS_ERROR_FAILURE);
    }

    // Remember the column
    filter_info->m_Column = (*filterIter).second.m_Column;
    filter_info->m_Override.Assign(NS_LITERAL_STRING(""));

    // Execute the new query for the feed
    PRBool isExecuting;
    rv = filter_info->m_Query->IsExecuting(&isExecuting);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isExecuting) {
      mon.Exit();
      // This will block until aborted
      PRBool dummy;
      rv = filter_info->m_Query->Abort(&dummy);
      NS_ENSURE_SUCCESS(rv, rv);
      mon.Enter();
    }

    rv = filter_info->m_Query->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = filter_info->m_Query->AddQuery(sub_query_str.get());
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 ret;
    rv = filter_info->m_Query->Execute(&ret);
    NS_ENSURE_SUCCESS(rv, rv);

    // And note that at least one filter exists on the main query?  Maybe?
    if (any_filter)
      anything = PR_TRUE;
  } // for

  // Only alter the main playlist query if we have current filters.
  if (!anything)
    main_query_str = simple_query_str;

  // Remove the previous results
  info->m_Resultset = nsnull;

  // Change the main query and resubmit.
  rv = info->m_Query->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = info->m_Query->AddQuery(main_query_str.get());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = info->m_Query->Execute(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::RegisterPlaylistCommands(const nsAString     &aContextGUID,
                                           const nsAString     &aTableName,
                                           const nsAString     &aPlaylistType,
                                           sbIPlaylistCommands *aCommandObj)
{
  LOG(("sbPlaylistsource::RegisterPlaylistCommands"));
  NS_ENSURE_ARG_POINTER(aCommandObj);

  METHOD_SHORTCIRCUIT;

  // Yes, I'm copying strings 1 time too many here.  I can live with that.
  nsString key(aContextGUID);
  nsString type(aPlaylistType);

  // Hah, uh, NO.
  if (key.Equals(NS_LITERAL_STRING("songbird")))
    return NS_OK;

  key += aTableName;

  g_CommandMap[type] = aCommandObj;
  g_CommandMap[key] = aCommandObj;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetPlaylistCommands(const nsAString      &aContextGUID,
                                      const nsAString      &aTableName,
                                      const nsAString      &aPlaylistType,
                                      sbIPlaylistCommands **_retval)
{
  LOG(("sbPlaylistsource::GetPlaylistCommands"));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;


  nsString key(aContextGUID);
  nsString type(aPlaylistType);

  key += aTableName;

  // "type" takes precedence over specific table names
  commandmap_t::iterator c = g_CommandMap.find(type);
  if (c != g_CommandMap.end()) {
    (*c).second->Duplicate(_retval);
    return NS_OK;
  }

  c = g_CommandMap.find(key);
  if (c != g_CommandMap.end()) {
    (*c).second->Duplicate(_retval);
    return NS_OK;
  }

  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::UpdateObservers()
{
  LOG(("sbPlaylistsource::UpdateObservers"));

  VMETHOD_SHORTCIRCUIT;

  resultlist_t old_garbage;
  PRBool need_update = PR_FALSE;
  {
    nsAutoMonitor mon(g_pMonitor);
    if (g_NeedUpdate) {
      need_update = PR_TRUE;
      g_NeedUpdate = PR_FALSE;
      old_garbage = g_ResultGarbage; // Destroyed when old_garbage goes out of scope.
      g_ResultGarbage.clear();
    }
  }

  if (!need_update)
    return NS_OK;

  nsresult rv;

  // Check to see if any are "forced" (loaded outside of the UI)
  for (resultlist_t::iterator r = old_garbage.begin();
       r != old_garbage.end();
       r++) {
    if ((*r).m_ForceGetTargets) {
      rv = ForceGetTargets((*r).m_Ref);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Inform the observers that everything changed.
  for (sbPlaylistsource::observers_t::iterator o =
         sbPlaylistsource::g_Observers.begin();
       o != sbPlaylistsource::g_Observers.end();
       o++) {
    nsAutoString os((*o).m_Ref);

    // Did the observer tell us which ref it wants to observe?
    if (os.IsEmpty()) {
      // If not, always update it.
      rv = (*o).m_Observer->OnBeginUpdateBatch(this);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = (*o).m_Observer->OnEndUpdateBatch(this);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      // Otherwise, check to see if the observer matches anything in the update
      // list
      for (resultlist_t::iterator r = old_garbage.begin();
           r != old_garbage.end();
           r++) {
        nsAutoString rs((*r).m_Ref);
        if (rs.Equals(os)) {
          rv = (*o).m_Observer->OnBeginUpdateBatch(this);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = (*o).m_Observer->OnEndUpdateBatch(this);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
  }

  // Only free the resultsets that observers might be using AFTER the observers
  // have been told to wake up.
  for (resultlist_t::iterator r = old_garbage.begin();
       r != old_garbage.end();
       r++) {
    if ((*r).m_Results.get()) {
      rv = (*r).m_Results->ClearResultSet();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::Init()
{
  LOG(("sbPlaylistsource::Init"));
  NS_ENSURE_TRUE(sRefCnt == 0, NS_ERROR_ALREADY_INITIALIZED);

  METHOD_SHORTCIRCUIT;

  // Make the shared query
  m_SharedQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ENSURE_TRUE(m_SharedQuery, NS_ERROR_FAILURE);

  nsresult rv;
  rv = m_SharedQuery->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the string bundle for our strings
  nsCOMPtr<nsIStringBundleService> sbs =
    do_GetService("@mozilla.org/intl/stringbundle;1");
  NS_ENSURE_TRUE(sbs, NS_ERROR_FAILURE);

  rv = sbs->CreateBundle("chrome://songbird/locale/songbird.properties",
                          getter_AddRefs(m_StringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  sRDFService = do_GetService("@mozilla.org/rdf/rdf-service;1");
  NS_ENSURE_TRUE(sRDFService, NS_ERROR_FAILURE);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING("NC:Playlist"),
                                       getter_AddRefs(kNC_Playlist));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING(NC_NAMESPACE_URI NS_L("child")),
                                getter_AddRefs(kNC_child));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING(NC_NAMESPACE_URI NS_L("pulse")),
                                getter_AddRefs(kNC_pulse));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING(RDF_NAMESPACE_URI NS_L("instanceOf")),
                                getter_AddRefs(kRDF_InstanceOf));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING(RDF_NAMESPACE_URI NS_L("type")),
                                getter_AddRefs(kRDF_type));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING(RDF_NAMESPACE_URI NS_L("nextVal")),
                                getter_AddRefs(kRDF_nextVal));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetUnicodeResource(NS_LITERAL_STRING(RDF_NAMESPACE_URI NS_L("Seq")),
                                getter_AddRefs(kRDF_Seq));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetLiteral(NS_LITERAL_STRING("PR_TRUE").get(),
                               getter_AddRefs(kLiteralTrue));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sRDFService->GetLiteral(NS_LITERAL_STRING("PR_FALSE").get(),
                               getter_AddRefs(kLiteralFalse));
  NS_ENSURE_SUCCESS(rv, rv);

  gPlaylistPlaylistsource = this;
  sRefCnt++;
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::DeInit()
{
  LOG(("sbPlaylistsource::DeInit"));
  NS_ENSURE_TRUE(sRefCnt == 1, NS_ERROR_UNEXPECTED);
  METHOD_SHORTCIRCUIT;
  gPlaylistPlaylistsource = nsnull;
  sRefCnt--;
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetURI(char** uri)
{
  LOG(("sbPlaylistsource::GetURI"));
  NS_ENSURE_ARG_POINTER(uri);
  METHOD_SHORTCIRCUIT;

  if (!(*uri = nsCRT::strdup("rdf:playlist")))
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetSource(nsIRDFResource*  property,
                            nsIRDFNode*      target,
                            PRBool           tv,
                            nsIRDFResource** source /* out */)
{
  LOG(("sbPlaylistsource::GetSource"));
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(target);
  NS_ENSURE_ARG_POINTER(source);

  METHOD_SHORTCIRCUIT;

  *source = nsnull;
  return NS_RDF_NO_VALUE;
}

NS_IMETHODIMP
sbPlaylistsource::GetSources(nsIRDFResource*       property,
                             nsIRDFNode*           target,
                             PRBool                tv,
                             nsISimpleEnumerator** sources /* out */)
{
  LOG(("sbPlaylistsource::GetSources"));
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(target);
  NS_ENSURE_ARG_POINTER(sources);

  METHOD_SHORTCIRCUIT;

  NS_NOTREACHED("Not yet implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistsource::GetTarget(nsIRDFResource* source,
                            nsIRDFResource* property,
                            PRBool          tv,
                            nsIRDFNode**    target /* out */)
{
  LOG(("sbPlaylistsource::GetTarget"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(target);

  METHOD_SHORTCIRCUIT;

  *target = nsnull;

  // we only have positive assertions in the file system data source.
  if (!tv)
    return NS_RDF_NO_VALUE;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsAutoString outstring( NS_LITERAL_STRING("") );
  nsresult rv;

#if 1

  // Look in the value map
  valuemap_t::iterator v = g_ValueMap.find(source);

  if (v == g_ValueMap.end())
    return NS_RDF_NO_VALUE;

  sbValueInfo &valInfo = (*v).second;

  // If we only have one value for this source ref, always return it.
  // Magic special case for the filter lists.
  if (valInfo.m_Info->m_ColumnMap.size() == 1) {
    if (valInfo.m_All)
      outstring += NS_LITERAL_STRING("All");
    else {
      PRUnichar *val = nsnull;
      rv = valInfo.m_Info->m_Resultset->GetRowCellPtr(valInfo.m_Row,
                                                      0,
                                                      &val);
      NS_ENSURE_SUCCESS(rv, rv);

      if(val != nsnull)
        outstring += nsDependentString(val);
    }
  } else {
    // Figure out the metadata column
    columnmap_t::iterator c = valInfo.m_Info->m_ColumnMap.find(property);
    if (c != valInfo.m_Info->m_ColumnMap.end()) {
      // And return that.
      if (property == valInfo.m_Info->m_RowIdResource)
        outstring.AppendInt(valInfo.m_Row + 1);
      else {
        PRUnichar *val = nsnull;
        if (valInfo.m_Info->m_Column.IsEmpty()) {
          // Only query the full metadata during get target.
          if (!valInfo.m_Resultset) {
            rv = LoadRowResults(valInfo, mon);
            NS_ENSURE_SUCCESS(rv, rv);
          }
          // The LoadRowResults above will fail silently if the table has been deleted and the UI is still asking for values.
          if ( valInfo.m_Resultset ) {
            rv = valInfo.m_Resultset->GetRowCellPtr(valInfo.m_ResultsRow,
                                                    (*c).second,
                                                    &val);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        } else {
          rv = valInfo.m_Info->m_Resultset->GetRowCellPtr(valInfo.m_Row,
                                                          (*c).second,
                                                          &val);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        if(val != nsnull)
          outstring += nsDependentString(val);
      }
    }
  }

#endif

  if (outstring.IsEmpty())
    return NS_RDF_NO_VALUE;

  // Recompose from the stringbundle
  if (outstring[0] == NS_L('&')) {
    PRUnichar* key = (PRUnichar*)outstring.get() + 1;
    PRUnichar* value;
    rv = m_StringBundle->GetStringFromName(key, &value);
    if (NS_SUCCEEDED(rv)) {
      outstring.Assign(value);
      PR_Free(value);
    }
  }

  nsCOMPtr<nsIRDFLiteral> literal;
  rv = sRDFService->GetLiteral(outstring.get(), getter_AddRefs(literal));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!literal)
    return NS_RDF_NO_VALUE;

  return literal->QueryInterface(NS_GET_IID(nsIRDFNode), (void**)target);
}

NS_IMETHODIMP
sbPlaylistsource::ForceGetTargets(const nsAString &aRefName)
{
  LOG(("sbPlaylistsource::ForceGetTargets"));

  METHOD_SHORTCIRCUIT;

  sbFeedInfo* info = GetFeedInfo(aRefName);
  
  if (info) {
    // So, like, force it. We don't actually want to hang on to
    //   the enumeration - by design!!
    nsCOMPtr<nsISimpleEnumerator> enumer;
    nsresult rv = GetTargets(info->m_RootResource, kNC_child,
                             PR_TRUE, getter_AddRefs(enumer));
    NS_ENSURE_SUCCESS(rv, rv);

    // And remember that it's forced.
    info->m_ForceGetTargets = PR_TRUE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetTargets2(nsIRDFResource*       source,
                             nsIRDFResource*       property,
                             PRBool                tv,
                             nsISimpleEnumerator** targets /* out */)
{
  LOG(("sbPlaylistsource::GetTargets"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(targets);

  METHOD_SHORTCIRCUIT;

  nsresult rv;
  *targets = nsnull;

  // we only have positive assertions in the file system data source.
  if (!tv)
    return NS_RDF_NO_VALUE;

  // We only respond targets to children, I guess.
  if (property != kNC_child)
    return NS_NewEmptyEnumerator(targets);

  // Make the array to hold our response
  nsCOMArray<nsIRDFResource> nextItemArray;

  // Store the new row resources in the array 
  // (and a map for quick lookup to the row PRInt32 -- STOOOOPID)

  for (PRInt32 i = 0; i < 20; i++) { 
    nsIRDFResource *next_resource = NULL;
    rv = sRDFService->GetAnonymousResource(&next_resource);
    NS_ENSURE_SUCCESS(rv, rv);
    nextItemArray.AppendObject(next_resource);
  }

  // Stuff the array into the enumerator
  rv = NS_NewArrayEnumerator(targets, nextItemArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetTargets(nsIRDFResource*       source,
                             nsIRDFResource*       property,
                             PRBool                tv,
                             nsISimpleEnumerator** targets /* out */)
{
  LOG(("sbPlaylistsource::GetTargets"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(targets);

  METHOD_SHORTCIRCUIT;

  *targets = nsnull;

  // we only have positive assertions in the file system data source.
  if (!tv)
    return NS_RDF_NO_VALUE;

  // We only respond targets to children, I guess.
  if (property != kNC_child)
    return NS_NewEmptyEnumerator(targets);

  // LOCK IT!
  nsAutoMonitor mon(g_pMonitor);

  // Okay, so, the "source" item should be found in the Map
  sbFeedInfo* info = GetFeedInfo(source);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  info->m_RefCount++; // Pleah.  This got away from me.

  if (!info->m_Query)
    return NS_RDF_NO_VALUE;

  // We need to create a new array enumerator, fill it with our next item, and send it back.

  // Let's try to reuse this so there's not so much map traffic?

  // Clear out the current vector of column resources
  columnmap_t::iterator c = info->m_ColumnMap.begin();
  for (; c != info->m_ColumnMap.end(); c++) {
    nsIRDFResource* p = const_cast<nsIRDFResource*>( (*c).first );
    NS_ENSURE_TRUE(p, NS_ERROR_UNEXPECTED);
    NS_RELEASE(p);
  }
  info->m_ColumnMap.clear();

  // Using the resultset object, make a set of resources for the table data
  nsCOMPtr<sbIDatabaseResult> resultset;
  // Might be different than resultset depending on #if blocks
  nsCOMPtr<sbIDatabaseResult> colresults;

  nsresult rv;

  if (info->m_Resultset)
    resultset = info->m_Resultset;
  else {
    rv = info->m_Query->GetResultObjectOrphan(getter_AddRefs(resultset));
    NS_ENSURE_SUCCESS(rv, rv);
    info->m_Resultset = resultset;
  }

  if (info->m_Column.IsEmpty()) {
    // If we're not a filterlist
    // Get the first row from the table to know what the columns should be?

    // Set the guid from the info's query guid.
    rv = m_SharedQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);

    // XXX These variable names are horrible

    PRUnichar* g;
    rv = info->m_Query->GetDatabaseGUID(&g);
    NS_ENSURE_SUCCESS(rv, rv);

    nsDependentString guid(g);
    rv = m_SharedQuery->SetDatabaseGUID(guid.get());
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the query string from the info's query string.
    PRUnichar* oq;
    rv = info->m_Query->GetQuery(0, &oq);
    NS_ENSURE_SUCCESS(rv, rv);

    nsDependentString q(oq);
    nsAutoString tablerow_str = library_str + dot_str + row_str;
    nsAutoString find_str = spc_str + tablerow_str + spc_str;

    if (q.Find(find_str) == -1) {
      q.ReplaceSubstring(NS_LITERAL_STRING(" id "),
                         NS_LITERAL_STRING(" *,id "));
    }
    else {
      nsAutoString replacement = spc_str + NS_LITERAL_STRING("*,") +
                                 tablerow_str + spc_str;
      q.ReplaceSubstring(find_str, replacement);
    }
    q += NS_LITERAL_STRING(" limit 1");

    rv = m_SharedQuery->AddQuery(q.get());
    NS_ENSURE_SUCCESS(rv, rv);

    // Then do it.
    mon.Exit();

    PRInt32 exec;
    rv = m_SharedQuery->Execute( &exec );
    NS_ENSURE_SUCCESS(rv, rv);

    mon.Enter();

    rv = m_SharedQuery->GetResultObjectOrphan(getter_AddRefs(colresults));
    NS_ENSURE_SUCCESS(rv, rv);

    PR_Free(g);
    PR_Free(oq);
  }
  else // info->m_Column.IsEmpty()
    colresults = resultset;

  // First re/create resources for the columns
  PRInt32 colcount, j;
  rv = colresults->GetColumnCount(&colcount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (j = 0; j < colcount; j++) {
    PRUnichar* col_name;
    rv = colresults->GetColumnName(j, &col_name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsDependentString col_str(col_name);
    nsCAutoString utf8_resource_name = NS_LITERAL_CSTRING(NC_NAMESPACE_URI) +
                                       NS_ConvertUTF16toUTF8(col_str);

    nsIRDFResource* col_resource;
    rv = sRDFService->GetResource(utf8_resource_name,
                                  &col_resource);
    NS_ENSURE_SUCCESS(rv, rv);


    info->m_ColumnMap[col_resource] = j;
    PR_Free(col_name);
  }

  if ((colcount == 0) && (!info->m_Column.IsEmpty())) {
    // If there is a column for this item, use it instead.
    nsCAutoString utf8_resource_name = NS_LITERAL_CSTRING(NC_NAMESPACE_URI) +
                                       NS_ConvertUTF16toUTF8(info->m_Column);
    nsIRDFResource *col_resource;
    rv = sRDFService->GetResource(utf8_resource_name,
                                  &col_resource);
    NS_ENSURE_SUCCESS(rv, rv);

    info->m_ColumnMap[col_resource] = 0;
    colcount = 1;
  } else if (colcount > 1) {
    // Add the magic "row_id" column.
    nsCAutoString utf8_resource_name;
    utf8_resource_name.Assign(NS_LITERAL_CSTRING(NC_NAMESPACE_URI "row_id"));

    nsIRDFResource *col_resource;
    rv = sRDFService->GetResource(utf8_resource_name,
                                  &col_resource);
    NS_ENSURE_SUCCESS(rv, rv);

    info->m_ColumnMap[col_resource] = j;
    info->m_RowIdResource = col_resource;
  } else {
    // XXX What about this case?
  }

  // Make the array to hold our response
  nsCOMArray<nsIRDFResource> nextItemArray;

  PRInt32 rowcount;
  rv = resultset->GetRowCount(&rowcount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 start = 0;
  PRInt32 size = (PRInt32)info->m_ResList.size();

  // One column must be a filter list.  (Well, for all practical purposes)
  if (colcount == 1) {
    // Magic bonus row for the "All" selection.
    start++;
    rowcount++;

    // Make sure there's enough reslist items.
    if (size < rowcount) {
      info->m_ResList.resize(rowcount);
      info->m_Values.resize(rowcount);

      // Insert nsnull.
      for (PRInt32 i = size; i < rowcount; i++) {
        info->m_ResList[i] = nsnull;
        info->m_Values[i] = nsnull;
      }
      size = (PRInt32)info->m_ResList.size();
    }

    // Grab the first item (or create it if it doesn't yet exist)
    nsIRDFResource *next_resource = info->m_ResList[0];
    if (!next_resource) {
      // Create a new resource for this item
      rv = sRDFService->GetAnonymousResource(&next_resource);
      NS_ENSURE_SUCCESS(rv, rv);

      info->m_ResList[0] = next_resource;
    }

    // Place it in the retval
    nextItemArray.AppendObject(next_resource);

    // And create a map entry for this item.
    sbValueInfo value;
    value.m_Info = info;
    value.m_Row = 0;
    value.m_All = PR_TRUE;

    // copies value
    g_ValueMap[next_resource] = value;
  }

  // Make sure there's enough reslist items.
  if (size < rowcount) {
    info->m_ResList.resize(rowcount);
    info->m_Values.resize(rowcount);

    // Insert nsnull (so we know later if we're recycling).
    for (PRInt32 i = size; i < rowcount; i++) {
      info->m_ResList[i] = nsnull;
      info->m_Values[i] = nsnull;
    }
  }

  // Store the new row resources in the array 
  // (and a map for quick lookup to the row PRInt32 -- STOOOOPID)

  for (PRInt32 i = start; i < rowcount; i++) {
    // Only create items if there is a value. Safe because our schema does not
    // allow this to be null for a real playlist.
    PRUnichar* ck;
    rv = resultset->GetRowCellPtr(i - start, 0, &ck);
    if ( !ck ) continue;
    NS_ENSURE_SUCCESS(rv, rv);

    nsDependentString check(ck);

    if ((!check.IsEmpty()) || (colcount > 1)) {
      nsIRDFResource *next_resource = info->m_ResList[i];

      if (!next_resource) {
        // let's try anonymous ones, eh?
        rv = sRDFService->GetAnonymousResource(&next_resource);
        NS_ENSURE_SUCCESS(rv, rv);

        info->m_ResList[i] = next_resource;

        sbValueInfo value;
        value.m_Info = info;
        value.m_Row = i - start;
        value.m_ResMapIndex = i;

        g_ValueMap[next_resource] = value;
      }

      // Always set these values
      sbValueInfo& value = g_ValueMap[next_resource];
      value.m_Id = check;
      value.m_Resultset = nsnull;

      nextItemArray.AppendObject(next_resource);
    }
  }

  // Stuff the array into the enumerator
  rv = NS_NewArrayEnumerator(targets, nextItemArray);
  NS_ENSURE_SUCCESS(rv, rv);

  // And give it to whomever is calling us
  //*targets = result;
  info->m_RootTargets = *targets;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::Assert(nsIRDFResource* source,
                         nsIRDFResource* property,
                         nsIRDFNode*     target,
                         PRBool          tv)
{
  LOG(("sbPlaylistsource::Assert"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(target);

  METHOD_SHORTCIRCUIT;
  
  return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
sbPlaylistsource::Unassert(nsIRDFResource* source,
                           nsIRDFResource* property,
                           nsIRDFNode*     target)
{
  LOG(("sbPlaylistsource::Unassert"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(target);

  METHOD_SHORTCIRCUIT;

  return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
sbPlaylistsource::Change(nsIRDFResource* aSource,
                         nsIRDFResource* aProperty,
                         nsIRDFNode*     aOldTarget,
                         nsIRDFNode*     aNewTarget)
{
  LOG(("sbPlaylistsource::Change"));
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aProperty);
  NS_ENSURE_ARG_POINTER(aOldTarget);
  NS_ENSURE_ARG_POINTER(aNewTarget);

  METHOD_SHORTCIRCUIT;

  return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
sbPlaylistsource::Move(nsIRDFResource* aOldSource,
                       nsIRDFResource* aNewSource,
                       nsIRDFResource* aProperty,
                       nsIRDFNode*     aTarget)
{
  LOG(("sbPlaylistsource::Move"));
  NS_ENSURE_ARG_POINTER(aOldSource);
  NS_ENSURE_ARG_POINTER(aNewSource);
  NS_ENSURE_ARG_POINTER(aProperty);
  NS_ENSURE_ARG_POINTER(aTarget);

  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}

NS_IMETHODIMP
sbPlaylistsource::HasAssertion(nsIRDFResource* source,
                               nsIRDFResource* property,
                               nsIRDFNode*     target,
                               PRBool          tv,
                               PRBool*         hasAssertion /* out */)
{
  LOG(("sbPlaylistsource::HasAssertion"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(property);
  NS_ENSURE_ARG_POINTER(target);
  NS_ENSURE_ARG_POINTER(hasAssertion);

  METHOD_SHORTCIRCUIT;

  // we only have positive assertions in the file system data source.
  *hasAssertion = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaylistsource::HasArcIn(nsIRDFNode*     aNode,
                           nsIRDFResource* aArc,
                           PRBool*         result)
{
  LOG(("sbPlaylistsource::HasArcIn"));
  NS_ENSURE_ARG_POINTER(aNode);
  NS_ENSURE_ARG_POINTER(aArc);
  NS_ENSURE_ARG_POINTER(result);

  METHOD_SHORTCIRCUIT;
  
  NS_NOTYETIMPLEMENTED("Not Implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaylistsource::HasArcOut(nsIRDFResource* aSource,
                            nsIRDFResource* aArc,
                            PRBool*         result)
{
  LOG(("sbPlaylistsource::HasArcOut"));
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(aArc);
  NS_ENSURE_ARG_POINTER(result);

  METHOD_SHORTCIRCUIT;
  
  *result = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::ArcLabelsIn(nsIRDFNode*           node,
                              nsISimpleEnumerator** labels /* out */)
{
  LOG(("sbPlaylistsource::ArcLabelsIn"));
  NS_ENSURE_ARG_POINTER(node);
  NS_ENSURE_ARG_POINTER(labels);

  METHOD_SHORTCIRCUIT;

  NS_NOTYETIMPLEMENTED("Not Implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistsource::ArcLabelsOut(nsIRDFResource*       source,
                               nsISimpleEnumerator** labels /* out */)
{
  LOG(("sbPlaylistsource::ArcLabelsOut"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(labels);

  METHOD_SHORTCIRCUIT;

  return NS_NewEmptyEnumerator(labels);
}

NS_IMETHODIMP
sbPlaylistsource::GetAllResources(nsISimpleEnumerator** aCursor)
{
  LOG(("sbPlaylistsource::GetAllResources"));
  NS_ENSURE_ARG_POINTER(aCursor);

  METHOD_SHORTCIRCUIT;
  
  NS_NOTYETIMPLEMENTED("Not Implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistsource::AddObserver(nsIRDFObserver* n)
{
  LOG(("sbPlaylistsource::AddObservers"));
  NS_ENSURE_ARG_POINTER(n);

  METHOD_SHORTCIRCUIT;

  for (observers_t::iterator oi = g_Observers.begin();
       oi != g_Observers.end();
       oi++) {
    // Find the pointer?
    if ((*oi).m_Observer == n) {
      // Cool, we already knew about this guy?
      const_cast<sbObserver&>(*oi).m_Ref = m_IncomingObserver;
      const_cast<sbObserver&>(*oi).m_Ptr = m_IncomingObserverPtr;
      return NS_OK;
    }
  }

  // If you didn't call "IncomingObserver()" first, you get ALL updates.
  sbObserver ob;
  ob.m_Observer = n;
  ob.m_Ref = m_IncomingObserver;
  ob.m_Ptr = m_IncomingObserverPtr;
  
  // Clear the "incoming" ref.
  m_IncomingObserver = NS_LITERAL_STRING("");
  m_IncomingObserverPtr = nsnull;

  g_Observers.insert(ob);

  // This is stupid, but it works.  Or at least it used to.

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::RemoveObserver(nsIRDFObserver* n)
{
  LOG(("sbPlaylistsource::RemoveObserver"));
  NS_ENSURE_ARG_POINTER(n);

  METHOD_SHORTCIRCUIT;

  sbObserver ob;
  ob.m_Observer = n;

  for (observers_t::iterator oi = g_Observers.begin();
       oi != g_Observers.end();
       oi++) {
    // Find the pointer?
    if ((*oi).m_Observer == n) {
      g_Observers.erase(oi);
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetAllCmds(nsIRDFResource*       source,
                             nsISimpleEnumerator** commands)
{
  LOG(("sbPlaylistsource::GetAllCmds"));
  NS_ENSURE_ARG_POINTER(source);
  NS_ENSURE_ARG_POINTER(commands);

  METHOD_SHORTCIRCUIT;
  
  return NS_NewEmptyEnumerator(commands);
}

NS_IMETHODIMP
sbPlaylistsource::IsCommandEnabled(nsISupportsArray* aSources,
                           nsIRDFResource*           aCommand,
                           nsISupportsArray*         aArguments,
                           PRBool*                   aResult)
{
  LOG(("sbPlaylistsource::IsCommandEnabled"));
  NS_ENSURE_ARG_POINTER(aSources);
  NS_ENSURE_ARG_POINTER(aCommand);
  NS_ENSURE_ARG_POINTER(aArguments);
  NS_ENSURE_ARG_POINTER(aResult);

  METHOD_SHORTCIRCUIT;

  NS_NOTYETIMPLEMENTED("Not Implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistsource::DoCommand(nsISupportsArray* aSources,
                            nsIRDFResource*   aCommand,
                            nsISupportsArray* aArguments)
{
  LOG(("sbPlaylistsource::DoCommand"));
  NS_ENSURE_ARG_POINTER(aSources);
  NS_ENSURE_ARG_POINTER(aCommand);
  NS_ENSURE_ARG_POINTER(aArguments);

  METHOD_SHORTCIRCUIT;

  NS_NOTYETIMPLEMENTED("Not Implemented");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbPlaylistsource::BeginUpdateBatch()
{
  LOG(("sbPlaylistsource::BeginUpdateBatch"));

  METHOD_SHORTCIRCUIT;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::EndUpdateBatch()
{
  LOG(("sbPlaylistsource::EndUpdateBatch"));

  METHOD_SHORTCIRCUIT;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::LoadRowResults(sbPlaylistsource::sbValueInfo& value, nsAutoMonitor& mon)
{
  LOG(("sbPlaylistsource::LoadRowResults"));

  METHOD_SHORTCIRCUIT;
  
//  nsAutoMonitor mon(g_pMonitor); // NO!  This is called by methods that already lock this monitor.

  nsresult rv;
  
  rv = m_SharedQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar* g;
  rv = value.m_Info->m_Query->GetDatabaseGUID(&g);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid(g);

  rv = m_SharedQuery->SetDatabaseGUID(guid.get());
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar* oq;
  rv = value.m_Info->m_Query->GetQuery(0, &oq);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString q(oq);

  // If we already have a where, don't add another one
  nsAutoString aw_str;
  if (q.Find(NS_LITERAL_STRING("where"), PR_TRUE) == -1)
    aw_str = NS_LITERAL_STRING(" where ");
  else
    aw_str = NS_LITERAL_STRING(" and ");

  nsAutoString tablerow_str = library_str + dot_str + row_str;
  nsAutoString find_str = spc_str + tablerow_str + spc_str;

  if (q.Find(find_str) == -1 )
  {
    q.ReplaceSubstring(NS_LITERAL_STRING(" id "),
                       NS_LITERAL_STRING(" *,id "));
    q += aw_str + NS_LITERAL_STRING("id >= ");
  } else {
    nsAutoString replace = spc_str + NS_LITERAL_STRING("*,") + tablerow_str +
                           spc_str;
    q.ReplaceSubstring(find_str, replace);
    q += aw_str + tablerow_str + spc_str + ge_str + spc_str;
  }

  q += value.m_Id + limit_str;
  q.AppendInt((PRInt32)LOOKAHEAD_SIZE);

  rv = m_SharedQuery->AddQuery(q.get());
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Exit();

  PRInt32 exe;
  rv = m_SharedQuery->Execute( &exe );
  NS_ENSURE_SUCCESS(rv, rv);

  mon.Enter();

  // Stash the result object
  nsCOMPtr<sbIDatabaseResult> result;
  rv = m_SharedQuery->GetResultObjectOrphan(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  // Walk through the ResList to put the lookahead values into the rows
  PRInt32 rows;
  rv = result->GetRowCount(&rows);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we don't run off the end of the m_ResList
  PRInt32 end = 0;
  if (value.m_ResMapIndex + rows < (PRInt32)value.m_Info->m_ResList.size())
    end = value.m_ResMapIndex + rows;
  else
    end = value.m_Info->m_ResList.size();

  for (PRInt32 i = value.m_ResMapIndex; i < end; i++) {
    sbValueInfo& val = g_ValueMap[value.m_Info->m_ResList[i]];
    val.m_Resultset = result;
    val.m_ResultsRow = i - value.m_ResMapIndex;
  }

  // Free the strings we got
  PR_Free(g);
  PR_Free(oq);

  return NS_OK;
}

// This is now an internal method.
NS_IMETHODIMP
sbPlaylistsource::GetQueryResult(const nsAString    &aRefName,
                                 sbIDatabaseResult** _retval)
{
  LOG(("sbPlaylistsource::GetQueryResult %s", NS_ConvertUTF16toUTF8(aRefName).get()));
  NS_ENSURE_ARG_POINTER(_retval);

  METHOD_SHORTCIRCUIT;

  // Find the ref string in the stringmap.
  sbFeedInfo* info = GetFeedInfo(aRefName);

  if (!info)
    return NS_ERROR_FAILURE;  // Since JS doesn't call it, use the return codes

  nsAutoMonitor mon(g_pMonitor);

  if (info->m_Resultset) {
    *_retval = info->m_Resultset;
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  // _retval is AddRef'd in GetResultObject
  nsresult rv = info->m_Query->GetResultObject(_retval);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Whoa, failed getting Result Object");

  return rv;
}


#pragma warning(pop)
