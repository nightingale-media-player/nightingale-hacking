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

/** 
 * \file  Playlistsource.h
 * \brief Songbird Playlistsource Component Definition.
 */

#ifndef __PLAYLIST_SOURCE_H__
#define __PLAYLIST_SOURCE_H__

#include <nsISupportsImpl.h>
#include <nsISupportsUtils.h>
#include <nsIStringBundle.h>
#include <nsIRDFLiteral.h>
#include <nsString.h>
#include <nspr/prmon.h>

#include "sbIPlaylistsource.h"
#include "sbIDatabaseQuery.h"
#include "sbIDatabaseResult.h"

#include <map>
#include <set>
#include <vector>
#include <list>

// DEFINES ====================================================================
#define SONGBIRD_PLAYLISTSOURCE_CONTRACTID                \
  "@mozilla.org/rdf/datasource;1?name=playlist"
#define SONGBIRD_PLAYLISTSOURCE_CLASSNAME                 \
  "Songbird Media Data Source Component"
#define SONGBIRD_PLAYLISTSOURCE_CID                       \
{ /* 46d6999a-7584-4e21-b1c4-c170ec8236e8 */              \
  0x46d6999a,                                             \
  0x7584,                                                 \
  0x4e21,                                                 \
  {0xb1, 0xc4, 0xc1, 0x70, 0xec, 0x82, 0x36, 0xe8}        \
}
#define SONGBIRD_PLAYLISTSOURCE_DESCRIPTION SONGBIRD_PLAYLISTSOURCE_CLASSNAME

#define NUM_PLAYLIST_ITEMS 0

// FORWARD DECLS ==============================================================
class MyQueryCallback;
class nsITimer;
class nsIRDFService;
class nsAutoMonitor;

// CLASSES ====================================================================
class sbPlaylistsource : public sbIPlaylistsource
{
  friend class MyQueryCallback;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPLAYLISTSOURCE
  NS_DECL_NSIRDFDATASOURCE

  sbPlaylistsource();
  virtual ~sbPlaylistsource();

  static sbPlaylistsource* GetSingleton();

  NS_IMETHODIMP UpdateObservers();

  struct sbObserver
  {
    PRBool operator== (const sbPlaylistsource::sbObserver &T) const
    {
      return m_Ptr == T.m_Ptr;
    }
    PRBool operator< (const sbPlaylistsource::sbObserver &T) const
    {
      return m_Ptr < T.m_Ptr;
    }
    sbObserver &operator= (const sbPlaylistsource::sbObserver &T)
    {
      m_Observer = T.m_Observer;
      m_Ptr = T.m_Ptr;
      m_Ref = T.m_Ref;
      return *this;
    }

    nsCOMPtr<nsIRDFObserver> m_Observer;
    void*                    m_Ptr;
    nsString                 m_Ref;
  };

  class observers_t : public std::set<sbObserver> {};

  void ClearPlaylistSTR(const PRUnichar *RefName);
  void ClearPlaylistRDF(nsIRDFResource *RefResource);

  struct sbFilterInfo
  {
    ~sbFilterInfo() { /*ClearPlaylistSTR(m_Ref.get());*/ }
    nsString m_Ref;
    nsString m_Filter;
    nsString m_Column;
  };

  struct sbValueInfo;
  struct sbFeedInfo;

  class filtermap_t : public std::map<PRInt32, sbFilterInfo> {};
  class reslist_t   : public std::vector<nsIRDFResource*> {};
  class columnmap_t : public std::map<nsIRDFResource*, PRInt32> {};

  struct sbResultInfo
  {
    nsCOMPtr<sbIDatabaseResult>   m_Results;
    nsCOMPtr<nsIRDFResource>      m_Source;
    nsCOMPtr<nsISimpleEnumerator> m_OldTarget;
    nsString                      m_Ref;
    PRBool                        m_ForceGetTargets;
    PRBool                        m_Processed;
  };

  class resultlist_t : public std::vector<sbResultInfo> {};

  struct sbValueInfo
  {
    sbFeedInfo*                 m_Info;
    PRInt32                     m_Row;
    PRInt32                     m_ResMapIndex;
    PRInt32                     m_ResultsRow;
    nsString                    m_Id;
    PRBool                      m_All;
    nsCOMPtr<sbIDatabaseResult> m_Resultset;

    sbValueInfo() : m_All(PR_FALSE),
                    m_Row(-1),
                    m_Id(),
                    m_Info(nsnull),
                    m_Resultset()
    { }
  };

  class values_t : public std::vector<sbValueInfo*> {};

  struct sbFeedInfo
  {
    PRInt32                       m_RefCount;
    PRBool                        m_ForceGetTargets;
    nsString                      m_Ref;
    nsString                      m_SearchString;
    nsString                      m_Table;
    nsString                      m_GUID;
    nsString                      m_Column;
    nsString                      m_SimpleQueryStr;
    nsString                      m_SortOrder;
    nsCOMPtr<nsIRDFResource>      m_RootResource;
    nsCOMPtr<nsISimpleEnumerator> m_RootTargets;
    nsCOMPtr<nsIRDFResource>      m_RowIdResource;
    nsCOMPtr<sbIDatabaseQuery>    m_Query;
    nsCOMPtr<sbIDatabaseResult>   m_Resultset;
    nsCOMPtr<MyQueryCallback>     m_Callback;
    filtermap_t                   m_Filters;
    reslist_t                     m_ResList;
    columnmap_t                   m_ColumnMap;
    values_t                      m_Values;
    nsCOMPtr<sbPlaylistsource>    m_Server;
  };

  class valuemap_t   : public std::map<nsIRDFResource*, sbValueInfo> {};
  class infomap_t    : public std::map<nsIRDFResource*, sbFeedInfo> {};
  class stringmap_t  : public std::map<nsString, nsIRDFResource*> {};
  class commandmap_t : public std::map<nsString,
                                       nsCOMPtr<sbIPlaylistCommands> > {};


  PRMonitor* g_pMonitor;
  PRInt32 g_ActiveQueryCount;
  PRInt32 sRefCnt;
  PRBool g_NeedUpdate;

  nsCOMPtr<nsIStringBundle> m_StringBundle;

  // Oh look, I can be a singleton and not use nasty statics.  Woo.
  nsCOMPtr<sbIDatabaseQuery> m_SharedQuery;
  nsString                   m_IncomingObserver;
  void*                      m_IncomingObserverPtr;

  NS_IMETHODIMP LoadRowResults(sbPlaylistsource::sbValueInfo& value, nsAutoMonitor& mon );

private:
  // Make these no longer be static globals, so they can destruct
  observers_t  g_Observers;
  stringmap_t  g_StringMap;
  infomap_t    g_InfoMap;
  valuemap_t   g_ValueMap;
  resultlist_t g_ResultGarbage;
  commandmap_t g_CommandMap;

  NS_IMETHODIMP Init(void);
  NS_IMETHODIMP DeInit(void);

  inline sbFeedInfo* GetFeedInfo(const nsAString& as)
  {
    nsString str( as );
    stringmap_t::iterator s = g_StringMap.find(str);
    if (s != g_StringMap.end())
      return GetFeedInfo((*s).second);
    return nsnull;
  }

  inline sbFeedInfo* GetFeedInfo(nsIRDFResource* res)
  {
    infomap_t::iterator i = g_InfoMap.find(res);
    if (i != g_InfoMap.end())
      return &((*i).second);
    return nsnull;
  }

  inline void EraseFeedInfo(nsIRDFResource *res)
  { }

  NS_IMETHODIMP GetQueryResult(const nsAString &RefName,
    sbIDatabaseResult** _retval);

  nsCOMPtr<nsIRDFService>  sRDFService;
  nsCOMPtr<nsIRDFResource> kNC_Playlist;
  nsCOMPtr<nsIRDFResource> kNC_child;
  nsCOMPtr<nsIRDFResource> kNC_pulse;
  nsCOMPtr<nsIRDFResource> kRDF_InstanceOf;
  nsCOMPtr<nsIRDFResource> kRDF_type;
  nsCOMPtr<nsIRDFResource> kRDF_nextVal;
  nsCOMPtr<nsIRDFResource> kRDF_Seq;
  nsCOMPtr<nsIRDFLiteral>  kLiteralTrue;
  nsCOMPtr<nsIRDFLiteral>  kLiteralFalse;

NS_IMETHODIMP    GetTargets2(nsIRDFResource*       source,
                             nsIRDFResource*       property,
                             PRBool                tv,
                             nsISimpleEnumerator** targets /* out */);

};

extern sbPlaylistsource* gPlaylistsource;

class MyQueryCallback : public sbIDatabaseSimpleQueryCallback
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASESIMPLEQUERYCALLBACK

public:
  MyQueryCallback();
  ~MyQueryCallback();
  void MyTimerCallback();
  static void MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure);
  NS_IMETHODIMP Post(void);

  sbPlaylistsource::sbFeedInfo *m_Info;
  nsCOMPtr<nsITimer> m_Timer;
  PRMonitor* m_pMonitor;
  PRInt32 m_Count;

  class resultslist_t   : public std::list< nsCOMPtr<sbIDatabaseResult> > {};
  resultslist_t m_Results;
};

#endif // __PLAYLIST_SOURCE_H__

