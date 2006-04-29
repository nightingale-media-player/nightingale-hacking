/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
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
#include "nsCRT.h"
#include "nsRDFCID.h"
#include "nsLiteralString.h"

#include "Playlistsource.h"
#include "IPlaylist.h"

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

static sbPlaylistsource *gPlaylistPlaylistsource = nsnull;

static nsIRDFService *gRDFService = nsnull;

nsIRDFResource *sbPlaylistsource::kNC_Playlist = nsnull;
nsIRDFResource *sbPlaylistsource::kNC_child = nsnull;
nsIRDFResource *sbPlaylistsource::kNC_pulse = nsnull;
nsIRDFResource *sbPlaylistsource::kRDF_InstanceOf = nsnull;
nsIRDFResource *sbPlaylistsource::kRDF_type = nsnull;
nsIRDFResource *sbPlaylistsource::kRDF_nextVal = nsnull;
nsIRDFResource *sbPlaylistsource::kRDF_Seq = nsnull;

nsIRDFLiteral *sbPlaylistsource::kLiteralTrue = nsnull;
nsIRDFLiteral *sbPlaylistsource::kLiteralFalse = nsnull;
sbPlaylistsource::observers_t  sbPlaylistsource::g_Observers;
sbPlaylistsource::stringmap_t  sbPlaylistsource::g_StringMap;
sbPlaylistsource::infomap_t    sbPlaylistsource::g_InfoMap;
sbPlaylistsource::valuemap_t   sbPlaylistsource::g_ValueMap;
sbPlaylistsource::resultlist_t sbPlaylistsource::g_ResultGarbage;
sbPlaylistsource::commandmap_t sbPlaylistsource::g_CommandMap;

PRMonitor* sbPlaylistsource::g_pMonitor = nsnull;

PRInt32 sbPlaylistsource::g_ActiveQueryCount = 0;
PRInt32 sbPlaylistsource::gRefCnt = 0;

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
  m_Timer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ASSERTION(m_Timer, "MyQueryCallback failed to create a timer");
}

void
MyQueryCallback::MyTimerCallbackFunc(nsITimer *aTimer,
                                     void     *aClosure)
{
  NS_ASSERTION(gPlaylistPlaylistsource,
    "MyQueryCallback timer fired before Playlistsource initialized");
   gPlaylistPlaylistsource->UpdateObservers();
}

NS_IMETHODIMP
MyQueryCallback::Post()
{
  NS_ENSURE_TRUE(gPlaylistPlaylistsource, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(m_Timer, NS_ERROR_OUT_OF_MEMORY);

  // Tell us to wake up in the main thread so we can poke our observers and
  // take out our garbage.

  // XXX This is NOT a safe or reliable way to post events to another thread
  m_Timer->InitWithFuncCallback(&MyTimerCallbackFunc, gPlaylistPlaylistsource,
                                0, 0);
  return NS_OK;
}

/* void OnQueryEnd (in sbIDatabaseResult dbResultObject,
                    in wstring dbGUID, in wstring strQuery); */
NS_IMETHODIMP
MyQueryCallback::OnQueryEnd(sbIDatabaseResult *dbResultObject,
                            const PRUnichar   *dbGUID,
                            const PRUnichar   *strQuery)
{
  NS_ENSURE_ARG_POINTER(dbResultObject);
  NS_ENSURE_ARG_POINTER(dbGUID);
  NS_ENSURE_ARG_POINTER(strQuery);

  // LOCK IT.
  nsAutoMonitor mon(sbPlaylistsource::g_pMonitor);

  NS_ASSERTION(sbPlaylistsource::g_ActiveQueryCount > 0,
               "MyQueryCallback running with an invalid active query count");

  NS_ENSURE_TRUE(m_Info->m_Resultset, NS_ERROR_UNEXPECTED);

  sbPlaylistsource::sbResultInfo result;

  // Copy the old resultset
  result.m_Results = m_Info->m_Resultset;
  result.m_Source = m_Info->m_RootResource;
  result.m_OldTarget = m_Info->m_RootTargets;
  result.m_Ref = m_Info->m_Ref;
  result.m_ForceGetTargets = m_Info->m_ForceGetTargets;

  // Push the old resultset onto the garbage stack.
  sbPlaylistsource::g_ResultGarbage.push_back(result);

  nsresult rv;

  // Orphan the result for the query.
  sbIDatabaseResult *res = nsnull;
  rv = m_Info->m_Query->GetResultObjectOrphan(&res);
  NS_ENSURE_SUCCESS(rv, NS_ERROR_NULL_POINTER);

  m_Info->m_Resultset = res;

  if (--sbPlaylistsource::g_ActiveQueryCount == 0) {
    sbPlaylistsource::g_NeedUpdate = PR_TRUE;
    return Post();
  }

  return NS_OK;
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
  gPlaylistPlaylistsource = this;
  if (!g_pMonitor) {
    g_pMonitor = nsAutoMonitor::NewMonitor("sbPlaylistsource.g_pMonitor");
    NS_ASSERTION(g_pMonitor, "sbPlaylistsource.g_pMonitor failed");
  }
  gPlaylistPlaylistsource->Init();
}

//-----------------------------------------------------------------------------
sbPlaylistsource::~sbPlaylistsource()
{
  LOG(("sbPlaylistsource::~sbPlaylistsource"));
  DeInit();
  if (g_pMonitor) {
    nsAutoMonitor::DestroyMonitor(g_pMonitor);
    g_pMonitor = nsnull;
  }
}

NS_IMETHODIMP
sbPlaylistsource::FeedPlaylist(const PRUnichar *RefName,
                               const PRUnichar *ContextGUID,
                               const PRUnichar *TableName)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(ContextGUID);
  NS_ENSURE_ARG_POINTER(TableName);

  LOG(("sbPlaylistsource::FeedPlaylist"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  // Find the ref string in the stringmap.
  nsDependentString strRefName(RefName);

  // See if the feed is already there.
  sbFeedInfo *testinfo = GetFeedInfo(strRefName);
  if (testinfo) {
    // Found it.  Get the refcount and increment it.
    testinfo->m_RefCount++;
    testinfo->m_Resultset = nsnull;
    return NS_OK;
  }

  // A new feed.  Fire a new Query object and stuff it in the tables.
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbird.org/Songbird/DatabaseQuery;1");
  NS_ENSURE_TRUE(query, NS_ERROR_FAILURE);

  nsresult rv;

  rv = query->SetAsyncQuery(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(ContextGUID);
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
  nsCOMPtr<nsIRDFResource> new_resource;
  rv = gRDFService->GetResource(NS_ConvertUTF16toUTF8(strRefName),
                                getter_AddRefs(new_resource));
  NS_ENSURE_SUCCESS(rv, rv);

  // And stuff it.
  sbFeedInfo info;
  info.m_Query = query;
  info.m_RefCount = 1;
  info.m_Ref = strRefName;
  info.m_Table = TableName;
  info.m_GUID = ContextGUID;
  info.m_RootResource = new_resource;
  info.m_Server = this;
  info.m_Callback = callback;
  info.m_ForceGetTargets = false;
  g_InfoMap[new_resource] = info;
  callback->m_Info = &g_InfoMap[new_resource];
  g_StringMap[strRefName] = new_resource;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::ClearPlaylist(const PRUnichar *RefName)
{
  NS_ENSURE_ARG_POINTER(RefName);
  LOG(("sbPlaylistsource::ClearPlaylist"));
  METHOD_SHORTCIRCUIT;
  ClearPlaylistSTR(RefName);
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::IncomingObserver(const PRUnichar *RefName,
                                   nsIDOMNode      *Observer)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(Observer);
  LOG(("sbPlaylistsource::IncomingObserver"));
  METHOD_SHORTCIRCUIT;
  for (observers_t::iterator oi = g_Observers.begin();
       oi != g_Observers.end();
       oi++ ) {
    // Find the pointer?
    if ((*oi).m_Ptr == Observer) {
      const_cast<sbObserver&>(*oi).m_Ref = RefName;
      // Assume that no AddObserver call will occur.
      return NS_OK;
    }
  }
  // Not found
  m_IncomingObserver = RefName;
  m_IncomingObserverPtr = Observer;
  return NS_OK;
}

void
sbPlaylistsource::ClearPlaylistSTR(const PRUnichar *RefName)
{
  NS_ASSERTION(RefName, "ClearPlaylistSTR passed a null pointer");
  if (!RefName)
    return;

  LOG(("sbPlaylistsource::ClearPlaylistSTR"));
  VMETHOD_SHORTCIRCUIT;

  // Find the ref string in the stringmap.
  nsDependentString strRefName(RefName);
  stringmap_t::iterator s = g_StringMap.find(strRefName);
  if (s != g_StringMap.end())
    ClearPlaylistRDF((*s).second);
}

void
sbPlaylistsource::ClearPlaylistRDF(nsIRDFResource *RefResource)
{
  NS_ASSERTION(RefResource, "ClearPlaylistRDF passed a null pointer");
  if (!RefResource)
    return;

  LOG(("sbPlaylistsource::ClearPlaylistRDF"));
  VMETHOD_SHORTCIRCUIT;

  sbFeedInfo *info = GetFeedInfo(RefResource);
  if (info) {
    // Found it.  Get the refcount and decrement it.
    if (--info->m_RefCount == 0)
      EraseFeedInfo( RefResource );
  }
}

NS_IMETHODIMP
sbPlaylistsource::GetQueryResult(const PRUnichar   *RefName,
                                 sbIDatabaseResult **_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);
  LOG(("sbPlaylistsource::GetQueryResult"));
  METHOD_SHORTCIRCUIT;
  *_retval = nsnull;

  // Find the ref string in the stringmap.
  nsString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  nsAutoMonitor mon(g_pMonitor);

  if (info->m_Resultset) {
    *_retval = info->m_Resultset;
    return NS_OK;
  }

  nsresult rv;

  nsCOMPtr<sbIDatabaseResult> resultObject;
  rv = info->m_Query->GetResultObject(getter_AddRefs(resultObject));
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = resultObject;
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetRefRowCount(const PRUnichar *RefName,
                                 PRInt32         *_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);
  LOG(("sbPlaylistsource::GetRefRowCount"));
  METHOD_SHORTCIRCUIT;
  *_retval = -1;

  nsresult rv;

  nsCOMPtr<sbIDatabaseResult> resultset;
  rv = GetQueryResult(RefName, getter_AddRefs(resultset));
  NS_ENSURE_SUCCESS(rv, rv);

  resultset->GetRowCount(_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetRefColumnCount(const PRUnichar *RefName,
                                    PRInt32         *_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);
  LOG(("sbPlaylistsource::GetRefColumnCount"));
  METHOD_SHORTCIRCUIT;
  *_retval = -1;

  nsresult rv;

  nsCOMPtr<sbIDatabaseResult> resultset;
  rv = GetQueryResult(RefName, getter_AddRefs(resultset));
  NS_ENSURE_SUCCESS(rv, rv);

  resultset->GetColumnCount(_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetRefRowCellByColumn(const PRUnichar *RefName,
                                        PRInt32         Row,
                                        const PRUnichar *Column,
                                        PRUnichar       **_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(Column);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::GetRefRowCellByColumn"));
  METHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);

  // Find the ref string in the stringmap.
  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  nsIRDFResource *next_resource = info->m_ResList[Row];
  valuemap_t::iterator v = g_ValueMap.find(next_resource);
  if (v != g_ValueMap.end()) {
    sbValueInfo valueInfo = (*v).second;
    nsresult rv;
    if (!valueInfo.m_Resultset) {
      LoadRowResults(valueInfo);
    }
    rv = valueInfo.m_Resultset->GetRowCellByColumn(valueInfo.m_ResultsRow,
                                                   Column,
                                                   _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


NS_IMETHODIMP
sbPlaylistsource::GetRefRowByColumnValue(const PRUnichar *RefName,
                                         const PRUnichar *Column,
                                         const PRUnichar *Value,
                                         PRInt32         *_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(Column);
  NS_ENSURE_ARG_POINTER(Value);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::GetRefRowByColumnValue"));
  METHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);

  *_retval = -1;

  // Find the ref string in the stringmap.
  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  nsresult rv;

  // Steal the query info used for the ref
  m_SharedQuery->ResetQuery();

  PRUnichar *guidPtr = nsnull;
  rv = info->m_Query->GetDatabaseGUID(&guidPtr);
  NS_ENSURE_SUCCESS(rv, rv);
  nsDependentString guid(guidPtr);

  rv = m_SharedQuery->SetDatabaseGUID(guid.get());
  NS_ENSURE_SUCCESS(rv, rv);

  PRUnichar *queryPtr = nsnull;
  rv = info->m_Query->GetQuery(0, &queryPtr);
  NS_ENSURE_SUCCESS(rv, rv);
  nsDependentString query(queryPtr);

  nsAString::const_iterator start, end;
  query.BeginReading(start);
  query.EndReading(end);

  // If we already have a where, don't add another one
  nsAutoString aw_str;
  if (FindInReadable(NS_LITERAL_STRING("where"), start, end))
    aw_str = NS_LITERAL_STRING(" where ");
  else
    aw_str = NS_LITERAL_STRING(" and ");

  // Append the metadata column restraint
  nsDependentString value_str(Value);
  nsDependentString column_str(Column);
  query += aw_str + column_str + eq_str +qu_str + value_str + qu_str;

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

  PRUnichar *val = nsnull;
  rv = result->GetRowCell(0, 0, &val);
  NS_ENSURE_SUCCESS(rv, rv);
  nsDependentString v(val);

  // (sigh) Now linear search the info results object for the matching id value to get the result index
  PRInt32 i, rowcount;
  rv = info->m_Resultset->GetRowCount(&rowcount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool found = PR_FALSE;
  PRUnichar *newval = nsnull;
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
sbPlaylistsource::IsQueryExecuting(const PRUnichar *RefName,
                                   PRBool          *_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::IsQueryExecuting"));
  METHOD_SHORTCIRCUIT;
  *_retval = PR_FALSE;

  // Find the ref string in the stringmap.
  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  return info->m_Query->IsExecuting(_retval);
}

// From nsNavHistory.cpp
void
ParseStringIntoArray(const nsString& aString,
                     nsStringArray*  aArray, 
                     PRUnichar aDelim)
{
  NS_ASSERTION(aArray, "Null pointer passed");
  if (!aArray)
    return;

  PRInt32 lastBegin = -1;
  for (PRUint32 i = 0; i < aString.Length(); i++) {
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
}

NS_IMETHODIMP
sbPlaylistsource::FeedPlaylistFilterOverride(const PRUnichar *RefName,
                                             const PRUnichar *FilterString)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(FilterString);

  LOG(("sbPlaylistsource::FeedPlaylistFilterOverride"));
  METHOD_SHORTCIRCUIT;

  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  nsDependentString filter_str(FilterString);

  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);
  NS_ENSURE_TRUE(info->m_Resultset, NS_ERROR_UNEXPECTED);

  info->m_Override = filter_str;

  if (filter_str.IsEmpty()) {
    // Revert to the normal override.
    mon.Exit();
    return FeedFilters(RefName, nsnull);
  }

  // Crack the incoming list of filter strings (space delimited)
  nsStringArray filter_values;
  ParseStringIntoArray(filter_str, &filter_values, NS_L(' '));

  nsDependentString table_name(info->m_Table);

  // The beginning of the main query string before we dump on it.
  nsAutoString main_query_str = info->m_SimpleQueryStr +where_str;

  // Compose an override query from the filter string and the columns in the
  // current results
  PRInt32 col_count, filter_count = (PRInt32)info->m_Filters.size();
  PRBool any_column = PR_FALSE;

  nsresult rv;
  rv = info->m_Resultset->GetColumnCount(&col_count);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(filter_count || col_count, "Nothing to do here!!!");

  if (filter_count) {
    // We're going to submit n+1 queries;
    g_ActiveQueryCount += filter_count + 1;

    filtermap_t::iterator f = info->m_Filters.begin();
    for (; f != info->m_Filters.end(); f++) {
      // Compose an override string for the filter query.
      // XXX Can't we just write it here instead of all this + crap?
      nsAutoString sub_query_str = unique_str + op_str + (*f).second.m_Column +
                                   cp_str + from_str + qu_str + table_name +
                                   qu_str + where_str + op_str;
      PRBool any_value = PR_FALSE;
      PRInt32 count = filter_values.Count();
      for (PRInt32 index = 0; index < count; index++) {
        if (any_value)
          sub_query_str += and_str;
        any_value = PR_TRUE;
        sub_query_str += op_str + (*f).second.m_Column + like_str + qu_str +
                         pct_str + *filter_values[index] + pct_str + qu_str +
                         cp_str;
      }
      sub_query_str += cp_str + order_str + (*f).second.m_Column;

      sbFeedInfo *filter_info = GetFeedInfo((*f).second.m_Ref);
      NS_ENSURE_TRUE(filter_info, NS_ERROR_NULL_POINTER);

      filter_info->m_Override = filter_str;
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
        main_query_str += op_str + (*f).second.m_Column + like_str + qu_str +
                          pct_str + *filter_values[index] + pct_str + qu_str +
                          cp_str;
      }

      main_query_str += or_str + op_str + NS_LITERAL_STRING("title") +
                        like_str + qu_str + pct_str + *filter_values[index] +
                        pct_str + qu_str + cp_str + cp_str;
    }
  } // filter_count
  else if (col_count) { 
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
sbPlaylistsource::GetFilterOverride(const PRUnichar *RefName,
                                    PRUnichar       **_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::GetFilterOverride"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  if (info->m_Override.IsEmpty()) {
    *_retval = (PRUnichar *)PR_Calloc(1, sizeof(PRUnichar));
    **_retval = 0;
    return NS_OK;
  }

  // Clunky, return the string
  PRUint32 nLen = info->m_Override.Length() + 1;
  *_retval = (PRUnichar *)PR_Calloc(nLen, sizeof(PRUnichar));
  memcpy(*_retval, info->m_Override.get(), (nLen - 1) * sizeof(PRUnichar));
  (*_retval)[info->m_Override.Length()] = 0;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::SetFilter(const PRUnichar *RefName,
                            PRInt32         Index,
                            const PRUnichar *FilterString,
                            const PRUnichar *FilterRefName,
                            const PRUnichar *FilterColumn)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(FilterString);
  NS_ENSURE_ARG_POINTER(FilterRefName);
  NS_ENSURE_ARG_POINTER(FilterColumn);

  LOG(("sbPlaylistsource::SetFilter"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  filtermap_t::iterator f = info->m_Filters.find(Index);
  if (f != info->m_Filters.end()) {
    // Change the existing strings
    (*f).second.m_Filter = FilterString;
    (*f).second.m_Column = FilterColumn;
    (*f).second.m_Ref    = FilterRefName;
  } else {
    // Make a new one, add it to the playlist feed
    sbFilterInfo filter;
    filter.m_Filter = FilterString;
    filter.m_Column = FilterColumn;
    filter.m_Ref    = FilterRefName;
    info->m_Filters[ Index ] = filter;
  }

  // Set all the later filters blank.
  for (f++; f != info->m_Filters.end(); f++) {
    (*f).second.m_Filter = nsString();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetNumFilters(const PRUnichar *RefName,
                                PRInt32         *_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::GetNumFilters"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);
  *_retval = (PRInt32)info->m_Filters.size();
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::ClearFilter(const PRUnichar *RefName,
                              PRInt32         Index)
{
  NS_ENSURE_ARG_POINTER(RefName);

  LOG(("sbPlaylistsource::ClearFilter"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  filtermap_t::iterator f = info->m_Filters.find(Index);
  if (f != info->m_Filters.end()) {
      // Remember to make the destructor clean up
      info->m_Filters.erase( f );
  }
  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetFilter(const PRUnichar *RefName,
                            PRInt32         Index,
                            PRUnichar       **_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::GetFilter"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);
  if (info->m_Override.IsEmpty()) {
    filtermap_t::iterator f = info->m_Filters.find(Index);
    if (f != info->m_Filters.end()) {
      // Clunky, return the string
      PRUint32 nLen = (*f).second.m_Filter.Length() + 1;
      *_retval = (PRUnichar*)PR_Calloc(nLen, sizeof(PRUnichar));
      memcpy(*_retval, (*f).second.m_Filter.get(),
             (nLen - 1) * sizeof(PRUnichar));
      (*_retval)[(*f).second.m_Filter.Length()] = 0;
      return NS_OK;
    }
  }

  // If we have an override, pretend the filter string is blank.
  *_retval = (PRUnichar *)PR_Calloc(1, sizeof(PRUnichar));
  **_retval = 0;

  return NS_OK;
}


NS_IMETHODIMP
sbPlaylistsource::GetFilterRef(const PRUnichar *RefName,
                               PRInt32         Index,
                               PRUnichar       **_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::GetFilterRef"));
  METHOD_SHORTCIRCUIT;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  filtermap_t::iterator f = info->m_Filters.find(Index);
  if (f != info->m_Filters.end()) {
    // Clunky, return the string
    PRUint32 nLen = (*f).second.m_Ref.Length() + 1;
    *_retval = (PRUnichar*)PR_Calloc(nLen, sizeof(PRUnichar));
    memcpy(*_retval, (*f).second.m_Ref.get(),
           (nLen - 1) * sizeof(PRUnichar));
    (*_retval)[(*f).second.m_Ref.Length()] = 0;
    return NS_OK;
  }

  // If we have an override, pretend the ref string is blank.
  *_retval = (PRUnichar *)PR_Calloc(1, sizeof(PRUnichar));
  **_retval = 0;

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::FeedFilters(const PRUnichar *RefName,
                              PRInt32         *_retval)
{
  NS_ENSURE_ARG_POINTER(RefName);
  // NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbPlaylistsource::FeedFilters"));
  METHOD_SHORTCIRCUIT;

  // No, you don't HAVE to pass in a value.
  // XXX Why does this need a retval param if we're not going to use it?
  PRInt32 retval;
  if ( !_retval )
  {
    _retval = &retval;
  }

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsDependentString strRefName(RefName);
  sbFeedInfo *info = GetFeedInfo(strRefName);
  NS_ENSURE_TRUE(info, NS_ERROR_NULL_POINTER);

  info->m_Override.Assign(NS_LITERAL_STRING(""));
  nsAutoString table_name(info->m_Table);

  nsCOMPtr<sbISimplePlaylist> pSimplePlaylist;
  nsCOMPtr<sbISmartPlaylist> pSmartPlaylist;

  nsCOMPtr<sbIDatabaseQuery> pQuery =
    do_CreateInstance("@songbird.org/Songbird/DatabaseQuery;1");
  NS_ENSURE_TRUE(pQuery, NS_ERROR_FAILURE);

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager =
    do_CreateInstance("@songbird.org/Songbird/PlaylistManager;1");
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
  filtermap_t::iterator f = info->m_Filters.begin();
  for (; f != info->m_Filters.end(); f++) {
    // Crack the incoming list of filter strings (semicolon delimited)
    nsAutoString filter_str((*f).second.m_Filter);
    nsStringArray filter_values;
    ParseStringIntoArray(filter_str, &filter_values, NS_L(';'));

    // Compose the SQL filter string
    nsString sql_filter_str;
    PRBool any_filter = PR_FALSE;
    PRInt32 count = filter_values.Count();      
    for (PRInt32 index = 0; index < count; index++) {
      if (any_filter)
        sql_filter_str += or_str;
      sql_filter_str += (*f).second.m_Column + eq_str + qu_str +
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
    nsAutoString sub_query_str = unique_str + op_str + (*f).second.m_Column +
                                 cp_str + from_str + qu_str + table_name +
                                 qu_str;
    if (anything)
      sub_query_str += where_str + sub_filter_str;
    sub_query_str += order_str + (*f).second.m_Column;

    // Append this filter's constraints to the next sub query
    if (any_filter) {
      if (anything)
        sub_filter_str += and_str;
      sub_filter_str += op_str + sql_filter_str + cp_str; 
    }

    // Make sure there is a feed for this filter
    sbFeedInfo *filter_info = GetFeedInfo((*f).second.m_Ref);
    if (!filter_info) {
      FeedPlaylist((*f).second.m_Ref.get(),
                   info->m_GUID.get(),
                   info->m_Table.get());
      filter_info = GetFeedInfo((*f).second.m_Ref);
      NS_ENSURE_TRUE(filter_info, NS_ERROR_FAILURE);
    }

    // Remember the column
    filter_info->m_Column = (*f).second.m_Column;
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

// End Ben's cleanup

/* void RegisterPlaylistCommands (in wstring ContextGUID, in wstring TableName, in sbIPlaylistCommands CommandObj); */
NS_IMETHODIMP sbPlaylistsource::RegisterPlaylistCommands(const PRUnichar *ContextGUID, const PRUnichar *TableName, const PRUnichar *PlaylistType, sbIPlaylistCommands *CommandObj)
{
  LOG(("sbPlaylistsource::RegisterPlaylistCommands"));
  METHOD_SHORTCIRCUIT;
  if ( ! ( ContextGUID && TableName && CommandObj ) )
  {
    return NS_OK;
  }

  nsString key( ContextGUID );
  nsString type( PlaylistType );

  // Hah, uh, NO.
  if ( key == NS_LITERAL_STRING("songbird") )
  {
    return NS_OK;
  }

  key += TableName;

  g_CommandMap[ type ] = CommandObj;
  g_CommandMap[ key ] = CommandObj;

  return NS_OK;
}

/* sbIPlaylistCommands GetPlaylistCommands (in wstring ContextGUID, in wstring TableName); */
NS_IMETHODIMP sbPlaylistsource::GetPlaylistCommands(const PRUnichar *ContextGUID, const PRUnichar *TableName, const PRUnichar *PlaylistType, sbIPlaylistCommands **_retval)
{
  LOG(("sbPlaylistsource::GetPlaylistCommands"));
  METHOD_SHORTCIRCUIT;
  if ( ! ( ContextGUID && TableName  ) )
  {
    return NS_OK;
  }

  *_retval = nsnull;

  nsString key( ContextGUID );
  nsString type( PlaylistType );
  key += TableName;

  commandmap_t::iterator c = g_CommandMap.find( type );
  if ( c != g_CommandMap.end() )
  {
    (*c).second->Duplicate( _retval );
  }
  else
  {
    commandmap_t::iterator c = g_CommandMap.find( key );
    if ( c != g_CommandMap.end() )
    {
      (*c).second->Duplicate( _retval );
    }
  }
  return NS_OK;
}



void sbPlaylistsource::UpdateObservers()
{
  LOG(("sbPlaylistsource::UpdateObservers"));
  VMETHOD_SHORTCIRCUIT;
  PRBool need_update = PR_FALSE;
  resultlist_t old_garbage;
  nsAutoMonitor mon(g_pMonitor);
  if ( g_NeedUpdate )
  {
    need_update = PR_TRUE;
    g_NeedUpdate = PR_FALSE;

    old_garbage = g_ResultGarbage; 
    g_ResultGarbage.clear();
  }
  mon.Exit();

  if ( need_update )
  {
    // Check to see if any are "forced" (loaded outside of the UI)
    for ( resultlist_t::iterator r = old_garbage.begin(); r != old_garbage.end(); r++ )
    {
      if ( (*r).m_ForceGetTargets )
      {
        ForceGetTargets( (*r).m_Ref.get() );
      }
    }

    // Inform the observers that everything changed.
    for ( sbPlaylistsource::observers_t::iterator o = sbPlaylistsource::g_Observers.begin(); o != sbPlaylistsource::g_Observers.end(); o++ )
    {
      nsString os = (*o).m_Ref;
      // Did the observer tell us which ref it wants to observe?
      if ( ! os.Length() )
      {
        // If not, always update it.
        (*o).m_Observer->OnBeginUpdateBatch( this );
        (*o).m_Observer->OnEndUpdateBatch( this );
      }
      else
      {
        // Otherwise, check to see does the observer match anything in the update list?
        for ( resultlist_t::iterator r = old_garbage.begin(); r != old_garbage.end(); r++ )
        {
          nsString rs = (*r).m_Ref;
          if ( rs == os  )
          {
            (*o).m_Observer->OnBeginUpdateBatch( this );
            (*o).m_Observer->OnEndUpdateBatch( this );
          }
        }
      }
    }

    // Only free the resultsets that observers might be using AFTER the observers have been told to wake up.
    for ( resultlist_t::iterator r = old_garbage.begin(); r != old_garbage.end(); r++ )
    {
      if ( (*r).m_Results.get() )
      {
        (*r).m_Results->ClearResultSet();
      }
    }
  }
}

void sbPlaylistsource::Init(void)
{
  LOG(("sbPlaylistsource::Init"));
  VMETHOD_SHORTCIRCUIT;
  if (gRefCnt++ == 0)
  {
    // Make the shared query
    m_SharedQuery = do_CreateInstance( "@songbird.org/Songbird/DatabaseQuery;1" );
    m_SharedQuery->SetAsyncQuery( PR_FALSE ); 

    nsresult rv = NS_OK;

    // Get the string bundle for our strings
    if ( ! m_StringBundle.get() )
    {
      nsIStringBundleService *  StringBundleService = nsnull;
      rv = CallGetService("@mozilla.org/intl/stringbundle;1", &StringBundleService );
      if ( NS_SUCCEEDED(rv) )
      {
        rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties", getter_AddRefs( m_StringBundle ) );
        //        StringBundleService->Release();
      }
    }

    if ( nsnull == gRDFService )
    {
      rv = CallGetService("@mozilla.org/rdf/rdf-service;1", &gRDFService);
    }
    if ( NS_SUCCEEDED(rv) )
    {
      gRDFService->GetResource(NS_LITERAL_CSTRING("NC:Playlist"),
        &kNC_Playlist);

      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "child"),
        &kNC_child);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "pulse"),
        &kNC_pulse);

      gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "instanceOf"),
        &kRDF_InstanceOf);
      gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "type"),
        &kRDF_type);
      gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "nextVal"),
        &kRDF_nextVal);
      //      gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "li"),
      //        &kRDF_li);
      gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Seq"),
        &kRDF_Seq);

      gRDFService->GetLiteral(NS_LITERAL_STRING("PR_TRUE").get(),       
        &kLiteralTrue);
      gRDFService->GetLiteral(NS_LITERAL_STRING("PR_FALSE").get(),      
        &kLiteralFalse);
    }

    gPlaylistPlaylistsource = this;
  }
}

void sbPlaylistsource::DeInit (void)
{
  LOG(("sbPlaylistsource::DeInit"));
  VMETHOD_SHORTCIRCUIT;
  // DEINIT THE STATIC 
  if (--gRefCnt == 0) 
  {
    if ( nsnull != gRDFService )
    {
      NS_RELEASE(kNC_Playlist);
      NS_RELEASE(kRDF_InstanceOf);
      NS_RELEASE(kRDF_type);
      NS_RELEASE(gRDFService);
    }
    gPlaylistPlaylistsource = nsnull;
  }
}

NS_IMETHODIMP
sbPlaylistsource::GetURI(char **uri)
{
  LOG(("sbPlaylistsource::GetURI"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(uri != nsnull, "null ptr");
  if (! uri)
    return NS_ERROR_NULL_POINTER;

  if ((*uri = nsCRT::strdup("rdf:playlist")) == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}



NS_IMETHODIMP
sbPlaylistsource::GetSource(nsIRDFResource* property,
                    nsIRDFNode* target,
                    PRBool tv,
                    nsIRDFResource** source /* out */)
{
  LOG(("sbPlaylistsource::GetSource"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(target != nsnull, "null ptr");
  if (! target)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  *source = nsnull;
  return NS_RDF_NO_VALUE;
}



NS_IMETHODIMP
sbPlaylistsource::GetSources(nsIRDFResource *property,
                     nsIRDFNode *target,
                     PRBool tv,
                     nsISimpleEnumerator **sources /* out */)
{
  LOG(("sbPlaylistsource::GetSources"));
  METHOD_SHORTCIRCUIT;
  //  NS_NOTYETIMPLEMENTED("write me");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
sbPlaylistsource::GetTarget(nsIRDFResource *source,
                    nsIRDFResource *property,
                    PRBool tv,
                    nsIRDFNode **target /* out */)
{
  LOG(("sbPlaylistsource::GetTarget"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(target != nsnull, "null ptr");
  if (! target)
    return NS_ERROR_NULL_POINTER;

  nsCString value;
  source->GetValueUTF8( value );

  *target = nsnull;

  nsresult        rv = NS_RDF_NO_VALUE;

  // we only have positive assertions in the file system data source.
  if (! tv)
    return NS_RDF_NO_VALUE;

  // LOCK IT.
  nsAutoMonitor mon(g_pMonitor);

  nsString outstring;
  // Look in the value map
  if ( ! outstring.Length() )
  {
    valuemap_t::iterator v = g_ValueMap.find( source );
    if ( v != g_ValueMap.end() )
    {
      // If we only have one value for this source ref, always return it.  
      // Magic special case for the filter lists.
      if ( (*v).second.m_Info->m_ColumnMap.size() == 1 )
      {
        PRUnichar *val = nsnull;
        if ( (*v).second.m_All )
        {
          outstring += NS_LITERAL_STRING("All");
        }
        else
        {
          (*v).second.m_Info->m_Resultset->GetRowCellPtr( (*v).second.m_Row, 0, &val );
        }
        outstring += val;
      }
      else
      {
        // Figure out the metadata column
        columnmap_t::iterator c = (*v).second.m_Info->m_ColumnMap.find( property );
        if ( c != (*v).second.m_Info->m_ColumnMap.end() )
        {
          // And return that.
          PRUnichar *val = nsnull;

          if ( property == (*v).second.m_Info->m_RowIdResource ) // row_id
          {
            outstring.AppendInt( (*v).second.m_Row + 1 );
          }
          else
          {
            if ( (*v).second.m_Info->m_Column.Length() == 0 )
            {
              // Only query the full metadata during get target.
              if ( (*v).second.m_Resultset.get() == nsnull )
              {
                LoadRowResults( (*v).second );
              }
              (*v).second.m_Resultset->GetRowCellPtr( (*v).second.m_ResultsRow, (*c).second, &val );
            }
            else
            {
              (*v).second.m_Info->m_Resultset->GetRowCellPtr( (*v).second.m_Row, (*c).second, &val );
            }
          }
          outstring += val;
//          PR_Free( val );    // GetRowCellPtr doesn't need freeing.
        }
      }
    }
  }

  if ( outstring.Length() )
  {
    // Recompose from the stringbundle
    if ( outstring[ 0 ] == '&' )
    {
      PRUnichar *key = (PRUnichar *)outstring.get() + 1;
      PRUnichar *value = nsnull;
      m_StringBundle->GetStringFromName( key, &value );
      if ( value && value[0] )
      {
        outstring = value;
      }
      PR_Free( value );
    }

    nsCOMPtr<nsIRDFLiteral> literal;
    rv = gRDFService->GetLiteral(outstring.get(), getter_AddRefs(literal));
    if (NS_FAILED(rv)) return(rv);
    if (!literal)   rv = NS_RDF_NO_VALUE;
    if (rv == NS_RDF_NO_VALUE) return(rv);

    return literal->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
  }

  return(NS_RDF_NO_VALUE);
}



NS_IMETHODIMP
sbPlaylistsource::ForceGetTargets(const PRUnichar *RefName)
{
  LOG(("sbPlaylistsource::ForceGetTargets"));
  METHOD_SHORTCIRCUIT;
  nsString strRefName( RefName );
  sbFeedInfo *info = GetFeedInfo( strRefName );

  if ( info )
  {
    // So, like, force it.
    nsCOMPtr< nsISimpleEnumerator > enumer;
    GetTargets( info->m_RootResource, kNC_child, true, getter_AddRefs( enumer ) );
    // And remember that it's forced.
    info->m_ForceGetTargets = true;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbPlaylistsource::GetTargets(nsIRDFResource *source,
                     nsIRDFResource *property,
                     PRBool tv,
                     nsISimpleEnumerator **targets /* out */)
{
  LOG(("sbPlaylistsource::GetTargets"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(targets != nsnull, "null ptr");
  if (! targets)
    return NS_ERROR_NULL_POINTER;

  *targets = nsnull;

  // we only have positive assertions in the file system data source.
  if (! tv)
    return NS_RDF_NO_VALUE;

  // We only respond targets to children, I guess.
  if (property == kNC_child)
  {

    PRInt32 rowcount = 0, colcount = 0;

    // LOCK IT!
    nsAutoMonitor mon(g_pMonitor);

    // Okay, so, the "source" item should be found in the Map
    sbFeedInfo *info = GetFeedInfo( source );
    if ( info )
    {
      info->m_RefCount++; // Pleah.  This got away from me.
      if ( !info->m_Query.get() )
      {
        return NS_RDF_NO_VALUE;
      }

      // We need to create a new array enumerator, fill it with our next item, and send it back.
      nsresult rv = NS_OK;
      *targets = nsnull;

      // Make the array to hold our response
      nsCOMPtr<nsISupportsArray> nextItemArray;
      rv = NS_NewISupportsArray(getter_AddRefs(nextItemArray));
      if (NS_FAILED(rv))
      {
        return rv;
      }

      //
      // Let's try to reuse this so there's not so much map traffic?
      //

      // Clear out the current vector of column resources
      columnmap_t::iterator c = info->m_ColumnMap.begin();
      for ( ; c != info->m_ColumnMap.end(); c++ )
      {
        nsIRDFResource *p = const_cast<nsIRDFResource *>( (*c).first );
        NS_RELEASE(p);
      }
      info->m_ColumnMap.clear();

      // Using the resultset object, make a set of resources for the table data
      nsCOMPtr< sbIDatabaseResult > resultset;
      nsCOMPtr< sbIDatabaseResult > colresults; // Might be diff than resultset dep on #if

      if ( info->m_Resultset.get() )
      {
        resultset = info->m_Resultset.get();
      }
      else
      {
        sbIDatabaseResult *res;
        info->m_Query->GetResultObjectOrphan( &res );
        resultset = res;
        res->Release(); // ah-hah!
        info->m_Resultset = resultset;
      }

      resultset->GetRowCount( &rowcount );

      // If we're not a filterlist
      if ( info->m_Column.Length() == 0 )
      {
        // Get the first row from the table to know what the columns should be?

        // Set the guid from the info's query guid.
        PRInt32 exec = 0;
        m_SharedQuery->ResetQuery();
        PRUnichar *g = nsnull;
        info->m_Query->GetDatabaseGUID( &g );
        nsString guid( g );
        m_SharedQuery->SetDatabaseGUID( guid.get() );

        // Set the query string from the info's query string.
        PRUnichar *oq = nsnull;
        info->m_Query->GetQuery( 0, &oq );
        nsString q( oq );
        nsString tablerow_str = library_str + dot_str + row_str;
        nsString find_str = spc_str + tablerow_str + spc_str;
        if ( q.Find( find_str ) != -1 )
        {
          q.ReplaceSubstring( find_str, spc_str + NS_LITERAL_STRING("*,") + tablerow_str + spc_str );
        }
        else
        {
          q.ReplaceSubstring( NS_LITERAL_STRING(" id "), NS_LITERAL_STRING(" *,id ") );
        }
        q += NS_LITERAL_STRING( " limit 1" );
        m_SharedQuery->AddQuery( q.get() );

        // Then do it.
        mon.Exit();
        m_SharedQuery->Execute( &exec );
        mon.Enter();
        sbIDatabaseResult *res;
        m_SharedQuery->GetResultObjectOrphan( &res );
        colresults = res;
        res->Release(); // ah-hah!
        PR_Free( g );
        PR_Free( oq );
      }
      else
      {
        colresults = resultset;
      }

      // First re/create resources for the columns
      PRInt32 j;
      colresults->GetColumnCount( &colcount );
      for ( j = 0; j < colcount; j++ )
      {
        nsIRDFResource *col_resource = nsnull;
        PRUnichar *col_name;
        colresults->GetColumnName( j, &col_name );
        nsString col_str( col_name );
        nsCString utf8_resource_name = NS_LITERAL_CSTRING( NC_NAMESPACE_URI ) +
          NS_ConvertUTF16toUTF8(col_str);
        gRDFService->GetResource( utf8_resource_name, &col_resource );
        info->m_ColumnMap[ col_resource ] = j;
        PR_Free( col_name );
      }
      if ( !j )
      {
        // If there is a column for this item, use it instead.
        if ( info->m_Column.Length() )
        {
          nsIRDFResource *col_resource = nsnull;
          nsCString utf8_resource_name = NS_LITERAL_CSTRING( NC_NAMESPACE_URI ) +
            NS_ConvertUTF16toUTF8( info->m_Column );
          gRDFService->GetResource( utf8_resource_name, &col_resource );
          info->m_ColumnMap[ col_resource ] = 0;
          colcount = 1;
        }
      }
      else if ( j > 1 )
      {
        // Add the magic "row_id" column.
        nsIRDFResource *col_resource = nsnull;
        nsCString utf8_resource_name = NS_LITERAL_CSTRING( NC_NAMESPACE_URI "row_id" );
        gRDFService->GetResource( utf8_resource_name, &col_resource );
        info->m_ColumnMap[ col_resource ] = j;
        info->m_RowIdResource = col_resource;
      }

      PRInt32 size = (PRInt32)info->m_ResList.size();
      // One column must be a filter list.  (Well, for all practical purposes)
      PRInt32 start = 0;
      if ( colcount == 1 )
      {
        // Magic bonus row for the "All" selection.
        start ++;
        rowcount ++;

        // Make sure there's enough reslist items.
        if ( size < rowcount )
        {
          info->m_ResList.resize( rowcount );
          info->m_Values.resize( rowcount );
          // Insert nsnull.
          for ( PRInt32 i = size; i < rowcount; i++ )
          {
            info->m_ResList[ i ] = nsnull;
            info->m_Values[ i ] = nsnull;
          }
          size = (PRInt32)info->m_ResList.size();
        }

        // Grab the first item (or create it if it doesn't yet exist)
        nsIRDFResource *next_resource = info->m_ResList[ 0 ];
        if ( ! next_resource )
        {
          // Create a new resource for this item
          gRDFService->GetAnonymousResource( &next_resource );
          info->m_ResList[ 0 ] = next_resource;
        }

        // Place it in the retval
        nextItemArray->AppendElement( next_resource );

        // And create a map entry for this item.
        sbValueInfo value;
        g_ValueMap[ next_resource ] = value;
        g_ValueMap[ next_resource ].m_Info = info;
        g_ValueMap[ next_resource ].m_Row = 0;
        g_ValueMap[ next_resource ].m_All = PR_TRUE;
//        info->m_Values[ 0 ] = & g_ValueMap[ next_resource ];
      }

      // Make sure there's enough reslist items.
      if ( size < rowcount )
      {
        info->m_ResList.resize( rowcount );
        info->m_Values.resize( rowcount );
        // Insert nsnull (so we know later if we're recycling).
        for ( PRInt32 i = size; i < rowcount; i++ )
        {
          info->m_ResList[ i ] = nsnull;
          info->m_Values[ i ] = nsnull;
        }
      }

      // Store the new row resources in the array 
      // (and a map for quick lookup to the row PRInt32 -- STOOOOPID)

      for ( PRInt32 i = start; i < rowcount; i++ )
      {
        // Only create items if there is a value
        // (Safe because our schema does not allow this to be null for a real playlist)
        PRUnichar *ck;
        resultset->GetRowCellPtr( i - start, 0, &ck );
        nsString check( ck );
        if ( check.Length() || ( colcount > 1 ) )
        {
          nsIRDFResource *next_resource = info->m_ResList[ i ];
          if ( ! next_resource )
          {
            gRDFService->GetAnonymousResource( &next_resource ); // let's try anonymous ones, eh?
            info->m_ResList[ i ] = next_resource;

            sbValueInfo value;
            value.m_Info = info;
            value.m_Row = i - start;
            value.m_ResMapIndex = i;
            g_ValueMap[ next_resource ] = value;
          }
          // Always set these values
          sbValueInfo &value = g_ValueMap[ next_resource ];
          value.m_Id = check;
          value.m_Resultset = nsnull;

          nextItemArray->AppendElement( next_resource );

        }
//        PR_Free( ck );
      }

      // Stuff the array into the enumerator
      nsISimpleEnumerator* result = new nsArrayEnumerator(nextItemArray);
      if (! result)
        return NS_ERROR_OUT_OF_MEMORY;

      // And give it to whomever is calling us
      NS_ADDREF(result);
      *targets = result;
      info->m_RootTargets = result;

      return rv;
    }
    else
    {
    }
  }

  return NS_NewEmptyEnumerator(targets);
}



NS_IMETHODIMP
sbPlaylistsource::Assert(nsIRDFResource *source,
                 nsIRDFResource *property,
                 nsIRDFNode *target,
                 PRBool tv)
{
  LOG(("sbPlaylistsource::Assert"));
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
sbPlaylistsource::Unassert(nsIRDFResource *source,
                   nsIRDFResource *property,
                   nsIRDFNode *target)
{
  LOG(("sbPlaylistsource::Unassert"));
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
sbPlaylistsource::Change(nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aOldTarget,
                 nsIRDFNode* aNewTarget)
{
  LOG(("sbPlaylistsource::Change"));
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
sbPlaylistsource::Move(nsIRDFResource* aOldSource,
               nsIRDFResource* aNewSource,
               nsIRDFResource* aProperty,
               nsIRDFNode* aTarget)
{
  LOG(("sbPlaylistsource::Move"));
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
sbPlaylistsource::HasAssertion(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv,
                       PRBool *hasAssertion /* out */)
{
  LOG(("sbPlaylistsource::HasAssertion"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(property != nsnull, "null ptr");
  if (! property)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(target != nsnull, "null ptr");
  if (! target)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(hasAssertion != nsnull, "null ptr");
  if (! hasAssertion)
    return NS_ERROR_NULL_POINTER;

  // we only have positive assertions in the file system data source.
  *hasAssertion = PR_FALSE;

  if (! tv) {
    return NS_OK;
  }

  if ( source == kNC_Playlist )
  {
    if (property == kRDF_type)
    {
      nsCOMPtr<nsIRDFResource> resource( do_QueryInterface(target) );
      if (resource.get() == kRDF_type)
      {
        *hasAssertion = PR_FALSE;
      }
    }
    if (property == kRDF_InstanceOf)
    {
      // Okay, so, first we'll try to say we're an RDF sequence.  Trees should like those.
      nsCOMPtr<nsIRDFResource> resource( do_QueryInterface(target) );
      if (resource.get() == kRDF_Seq)
      {
        *hasAssertion = PR_FALSE;
      }
    }
  }

  return NS_OK;
}



NS_IMETHODIMP 
sbPlaylistsource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
  LOG(("sbPlaylistsource::HasArcIn"));
  METHOD_SHORTCIRCUIT;
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP 
sbPlaylistsource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *result)
{
  LOG(("sbPlaylistsource::HasArcOut"));
  METHOD_SHORTCIRCUIT;
  *result = PR_FALSE;
  return NS_OK;
}



NS_IMETHODIMP
sbPlaylistsource::ArcLabelsIn(nsIRDFNode *node,
                      nsISimpleEnumerator ** labels /* out */)
{
  LOG(("sbPlaylistsource::ArcLabelsIn"));
  METHOD_SHORTCIRCUIT;
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
sbPlaylistsource::ArcLabelsOut(nsIRDFResource *source,
                       nsISimpleEnumerator **labels /* out */)
{
  LOG(("sbPlaylistsource::ArcLabelsOut"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(labels != nsnull, "null ptr");
  if (! labels)
    return NS_ERROR_NULL_POINTER;

  /*
  nsresult rv;

  if (source == kNC_Playlist)
  {
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) return rv;

  array->AppendElement(kNC_child);
  array->AppendElement(kNC_pulse);

  nsISimpleEnumerator* result = new nsArrayEnumerator(array);
  if (! result)
  return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *labels = result;
  return NS_OK;
  }
  else if (isFileURI(source))
  {
  nsCOMPtr<nsISupportsArray> array;
  rv = NS_NewISupportsArray(getter_AddRefs(array));
  if (NS_FAILED(rv)) return rv;

  if (isDirURI(source))
  {
  #ifdef  XP_WIN
  if (isValidFolder(source) == PR_TRUE)
  {
  array->AppendElement(kNC_child);
  }
  #else
  array->AppendElement(kNC_child);
  #endif
  array->AppendElement(kNC_pulse);
  }

  nsISimpleEnumerator* result = new nsArrayEnumerator(array);
  if (! result)
  return NS_ERROR_OUT_OF_MEMORY;

  NS_ADDREF(result);
  *labels = result;
  return NS_OK;
  }
  */
  return NS_NewEmptyEnumerator(labels);
}



NS_IMETHODIMP
sbPlaylistsource::GetAllResources(nsISimpleEnumerator** aCursor)
{
  LOG(("sbPlaylistsource::GetAllResources"));
  METHOD_SHORTCIRCUIT;
  NS_NOTYETIMPLEMENTED("sorry!");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
sbPlaylistsource::AddObserver(nsIRDFObserver *n)
{
  LOG(("sbPlaylistsource::AddObservers"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(n != nsnull, "null ptr");
  if (! n)
    return NS_ERROR_NULL_POINTER;

  PRBool found = PR_FALSE;
  for ( observers_t::iterator oi = g_Observers.begin(); oi != g_Observers.end(); oi++ )
  {
    // Find the pointer?
    if ( (*oi).m_Observer == n )
    {
      // Cool, we already knew about this guy?
      found = PR_TRUE;
      const_cast<sbObserver &>((*oi)).m_Ref = m_IncomingObserver;
      const_cast<sbObserver &>((*oi)).m_Ptr = m_IncomingObserverPtr;
    }
  }

  if ( !found )
  {
    // If you didn't call "IncomingObserver()" first, you get ALL updates.
    sbObserver ob;
    ob.m_Observer = n;
    ob.m_Ref = m_IncomingObserver;
    ob.m_Ptr = m_IncomingObserverPtr;
    m_IncomingObserver = NS_LITERAL_STRING( "" ); // Clear the "incoming" ref.
    m_IncomingObserverPtr = nsnull;
    g_Observers.insert( ob );
  }

  // This is stupid, but it works.  Or at least it used to.

  return NS_OK;
}



NS_IMETHODIMP
sbPlaylistsource::RemoveObserver(nsIRDFObserver *n)
{
  LOG(("sbPlaylistsource::RemoveObserver"));
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(n != nsnull, "null ptr");
  if (! n)
    return NS_ERROR_NULL_POINTER;

  sbObserver ob;
  ob.m_Observer = n;

  for ( observers_t::iterator oi = g_Observers.begin(); oi != g_Observers.end(); oi++ )
  {
    // Find the pointer?
    if ( (*oi).m_Observer == n )
    {
      g_Observers.erase( oi );
      break;
    }
  }

  return NS_OK;
}



NS_IMETHODIMP
sbPlaylistsource::GetAllCmds(nsIRDFResource* source,
                     nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
  LOG(("sbPlaylistsource::GetAllCmds"));
  METHOD_SHORTCIRCUIT;
  return(NS_NewEmptyEnumerator(commands));
}



NS_IMETHODIMP
sbPlaylistsource::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                           nsIRDFResource*   aCommand,
                           nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                           PRBool* aResult)
{
  LOG(("sbPlaylistsource::IsCommandEnabled"));
  METHOD_SHORTCIRCUIT;
  return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
sbPlaylistsource::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                    nsIRDFResource*   aCommand,
                    nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
  LOG(("sbPlaylistsource::DoCommand"));
  METHOD_SHORTCIRCUIT;
  return(NS_ERROR_NOT_IMPLEMENTED);
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

void
sbPlaylistsource::LoadRowResults( sbPlaylistsource::sbValueInfo & value )
{
  LOG(("sbPlaylistsource::LoadRowResults"));
  VMETHOD_SHORTCIRCUIT;
  nsAutoMonitor mon(g_pMonitor);
  m_SharedQuery->ResetQuery();
  PRUnichar *g = nsnull;
  value.m_Info->m_Query->GetDatabaseGUID( &g );
  nsString guid( g );
  m_SharedQuery->SetDatabaseGUID( guid.get() );

  PRUnichar *oq = nsnull;
  value.m_Info->m_Query->GetQuery( 0, &oq );
  nsString q( oq );

  // If we already have a where, don't add another one
  nsString aw_str;
  if ( q.Find( "where", PR_TRUE ) == -1 )
  {
    aw_str = NS_LITERAL_STRING( " where " );
  }
  else
  {
    aw_str = NS_LITERAL_STRING( " and " );
  }

  nsString tablerow_str = library_str + dot_str + row_str;
  nsString find_str = spc_str + tablerow_str + spc_str;
  if ( q.Find( find_str ) != -1 )
  {
    q.ReplaceSubstring( find_str, spc_str + NS_LITERAL_STRING("*,") + tablerow_str + spc_str );
    q += aw_str + tablerow_str + spc_str + ge_str + spc_str;
  }
  else
  {
    q.ReplaceSubstring( NS_LITERAL_STRING(" id "), NS_LITERAL_STRING(" *,id ") );
    q += aw_str + NS_LITERAL_STRING( "id >= " );
  }
  q += value.m_Id;

  const PRInt32 LOOKAHEAD_SIZE = 30;
  q += limit_str;
  q.AppendInt( LOOKAHEAD_SIZE );

  m_SharedQuery->AddQuery( q.get() );

  PRInt32 exe;
  mon.Exit();
  m_SharedQuery->Execute( &exe );
  mon.Enter();

  // Stash the result object
  nsCOMPtr< sbIDatabaseResult > result;
  m_SharedQuery->GetResultObjectOrphan( getter_AddRefs( result ) );

  // Walk through the ResList to put the lookahead values into the rows
  PRInt32 i,rows = 0;
  result->GetRowCount( &rows );
  PRInt32 end = ( value.m_ResMapIndex + rows < value.m_Info->m_ResList.size() ) ? value.m_ResMapIndex + rows : value.m_Info->m_ResList.size();
  for ( i = value.m_ResMapIndex; i < end; i++ )
  {
    sbPlaylistsource::sbValueInfo &val = g_ValueMap[ value.m_Info->m_ResList[ i ] ];
    val.m_Resultset = result;
    val.m_ResultsRow = i - value.m_ResMapIndex;
  }

  // Free the strings we got
  PR_Free( g );
  PR_Free( oq );
}

