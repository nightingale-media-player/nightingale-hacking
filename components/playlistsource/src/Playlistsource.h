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
 * \file  Playlistsource.h
 * \brief Songbird Playlistsource Component Definition.
 */

#pragma once

#include <nsISupportsImpl.h>
#include <nsISupportsUtils.h>
#include <nsIStringBundle.h>
#include <nsIRDFLiteral.h>
#include <nsString.h>
#include <nspr/prmon.h>

#include "sbIPlaylistsource.h"
#include "IDatabaseQuery.h"

#include <map>
#include <set>
#include <vector>

// DEFINES ====================================================================
#define SONGBIRD_PLAYLISTSOURCE_CONTRACTID                                    \
          "@mozilla.org/rdf/datasource;1?name=playlist"
#define SONGBIRD_PLAYLISTSOURCE_CLASSNAME                                     \
          "Songbird Media Data Source Component"
#define SONGBIRD_PLAYLISTSOURCE_CID                                           \
          {0x836d6ea5, 0xca63, 0x418f,                                        \
            {0xbf, 0xd8, 0x27, 0x70, 0x45, 0x9, 0xa6, 0xa3}}

#define NUM_PLAYLIST_ITEMS 0

// FORWARD DECLS ==============================================================
class MyQueryCallback;
class nsITimer;
class nsIRDFService;

// CLASSES ====================================================================
class sbPlaylistsource : public sbIPlaylistsource
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPLAYLISTSOURCE
  NS_DECL_NSIRDFDATASOURCE

  sbPlaylistsource();
  virtual ~sbPlaylistsource();
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
  static observers_t g_Observers;

  static void ClearPlaylistSTR(const PRUnichar *RefName);
  static void ClearPlaylistRDF(nsIRDFResource *RefResource);

  struct sbFilterInfo
  {
    ~sbFilterInfo() { ClearPlaylistSTR(m_Ref.get()); }
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
    nsString                      m_Override;
    nsString                      m_Table;
    nsString                      m_GUID;
    nsString                      m_Column;
    nsString                      m_SimpleQueryStr;
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

  static stringmap_t  g_StringMap;
  static infomap_t    g_InfoMap;
  static valuemap_t   g_ValueMap;
  static resultlist_t g_ResultGarbage;
  static commandmap_t g_CommandMap;

  static PRMonitor* g_pMonitor;
  static PRInt32    sRefCnt;
  static PRInt32    g_ActiveQueryCount;
  static PRBool     g_NeedUpdate;

  static nsCOMPtr<nsIStringBundle> m_StringBundle;

  // Oh look, I can be a singleton and not use nasty statics.  Woo.
  nsCOMPtr<sbIDatabaseQuery> m_SharedQuery;
  nsString                   m_IncomingObserver;
  void*                      m_IncomingObserverPtr;

  NS_IMETHODIMP LoadRowResults(sbPlaylistsource::sbValueInfo& value);

private:
  NS_IMETHODIMP Init(void);
  NS_IMETHODIMP DeInit(void);

  inline static sbFeedInfo* GetFeedInfo(nsString& str)
  {
    stringmap_t::iterator s = g_StringMap.find(str);
    if (s != g_StringMap.end())
      return GetFeedInfo((*s).second);
    return nsnull;
  }

  inline static sbFeedInfo* GetFeedInfo(nsIRDFResource* res)
  {
    infomap_t::iterator i = g_InfoMap.find(res);
    if (i != g_InfoMap.end())
      return &((*i).second);
    return nsnull;
  }

  inline static void EraseFeedInfo(nsIRDFResource *res)
  { }

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
};

class MyQueryCallback : public sbIDatabaseSimpleQueryCallback
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASESIMPLEQUERYCALLBACK

public:
  MyQueryCallback();
  static void MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure);
  NS_IMETHODIMP Post(void);

  sbPlaylistsource::sbFeedInfo *m_Info;

private:
  nsCOMPtr<nsITimer> m_Timer;
};
