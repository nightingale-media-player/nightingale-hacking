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
* \file  Servicesource.cpp
* \brief Songbird Servicesource Component Implementation.
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

#include "nsITimer.h"

#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsCOMArray.h"
#include "nsEnumeratorUtils.h"
#include "nsArrayEnumerator.h"

/*
#include "nsIRDFFileSystem.h"
#include "nsRDFBuiltInServicesources.h"
*/

#include "nsCRTGlue.h"
#include "nsRDFCID.h"
#include <nsStringGlue.h>

#include "Servicesource.h"

#include "sbIPlaylist.h"
#include "sbIDatabaseResult.h"
#include "sbIPlaylistPlayback.h"

#define MODULE_SHORTCIRCUIT 0

#if MODULE_SHORTCIRCUIT
# define METHOD_SHORTCIRCUIT return NS_OK;
# define VMETHOD_SHORTCIRCUIT return;
#else
# define METHOD_SHORTCIRCUIT
# define VMETHOD_SHORTCIRCUIT
#endif

#ifdef PR_LOGGING
/*
 * NSPR_LOG_MODULES=sbServicesource:5
 */
static PRLogModuleInfo* gServicesourceLog = PR_NewLogModule("sbServicesource");
#endif
#define LOG(args) PR_LOG(gServicesourceLog, PR_LOG_DEBUG, (args))

static  CServicesource  *gServicesource = nsnull;
static  nsIRDFService   *gRDFService = nsnull;
static  sbIPlaylistPlayback *gPPS;

// A callback from the database for when things change and we should repaint.
class MyServicesourceQueryCallback : public sbIDatabaseSimpleQueryCallback
{
  NS_DECL_ISUPPORTS
    NS_DECL_SBIDATABASESIMPLEQUERYCALLBACK

    MyServicesourceQueryCallback()
  {
      nsresult rv;
      m_Timer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to create timer in servicesource");
  }

  static void MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure)
  {
    if ( gServicesource )
    {
      gServicesource->Update();
    }
  }
  nsCOMPtr< nsITimer >          m_Timer;
};
NS_IMPL_ISUPPORTS1(MyServicesourceQueryCallback, sbIDatabaseSimpleQueryCallback)
/* void OnQueryEnd (in sbIDatabaseResult dbResultObject, in wstring dbGUID, in wstring strQuery); */
NS_IMETHODIMP MyServicesourceQueryCallback::OnQueryEnd(sbIDatabaseResult *dbResultObject, const nsAString &dbGUID, const nsAString &strQuery)
{
  if ( m_Timer.get() )
  {
    m_Timer->Cancel();
  }

  m_Timer->InitWithFuncCallback( &MyTimerCallbackFunc, gServicesource, 0, 0 );

  return NS_OK;
}

// Lame hardcode to find the playlist folder
const int nPlaylistsInsert = 1;

static nsString gPlaylistUrl( NS_LITERAL_STRING("chrome://songbird/content/xul/playlist_test.xul?") );

static nsString     gParentLabels[ NUM_PARENTS ] =
{
  NS_LITERAL_STRING("&servicesource.welcome"),
  NS_LITERAL_STRING("&servicesource.library"),
};

static nsString     gParentIcons[ NUM_PARENTS ] =
{
  NS_LITERAL_STRING("chrome://songbird/skin/default/logo_16.png"),
  NS_LITERAL_STRING("chrome://songbird/skin/default/icon_lib_16x16.png"),
};

static nsString     gParentUrls[ NUM_PARENTS ] =
{
  NS_LITERAL_STRING("http://www.songbirdnest.com/birdhouse/"),
  NS_LITERAL_STRING("chrome://songbird/content/xul/playlist_test.xul?library"),
};

static nsString     gParentOpen[ NUM_PARENTS ] =
{
  NS_LITERAL_STRING("false"),
  NS_LITERAL_STRING("false"),
};

static nsString     gChildLabels[ NUM_PARENTS ][ MAX_CHILDREN ] =
{
  // Group 1
  {
    nsString( ),
  },
  // Group 2
  {
    nsString( ),
  },
};

static nsString     gChildUrls[ NUM_PARENTS ][ MAX_CHILDREN ] =
{
  // Group 1
  {
    nsString( ),
  },
  // Group 2
  {
    nsString( ),
  },
};

static nsString     gChildIcons[ NUM_PARENTS ][ MAX_CHILDREN ] =
{
  // Group 1
  {
    nsString( ),
  },
  // Group 2
  {
    nsString( ),
  },
};

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(CServicesource, sbIServicesource, nsIRDFDataSource)
//-----------------------------------------------------------------------------
CServicesource::CServicesource()
{
  Init();
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CServicesource::~CServicesource()
{
  DeInit();

} //dtor

void CServicesource::Update(void)
{
  VMETHOD_SHORTCIRCUIT;
  // Inform the observers that everything changed.
  for ( observers_t::iterator o = m_Observers.begin(); o != m_Observers.end(); o++ )
  {
    (*o)->OnBeginUpdateBatch( this );
    (*o)->OnEndUpdateBatch( this );
  }
}

void CServicesource::Init(void)
{
  LOG("ServiceSource::Init()");
  VMETHOD_SHORTCIRCUIT;
  {
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

    // Get the playlist playback service to test for being a playlist
    if ( nsnull == gPPS )
    {
      rv = CallGetService("@songbirdnest.com/Songbird/PlaylistPlayback;1", &gPPS );
    }

    // Get the RDF service
    if ( nsnull == gRDFService )
    {
      rv = CallGetService("@mozilla.org/rdf/rdf-service;1", &gRDFService);
    }
    if ( NS_SUCCEEDED(rv) )
    {
      gRDFService->GetResource(NS_LITERAL_CSTRING("NC:Servicesource"),
        &kNC_Servicesource);
      gRDFService->GetResource(NS_LITERAL_CSTRING("NC:ServicesourceFlat"),
        &kNC_ServicesourceFlat);
      gRDFService->GetResource(NS_LITERAL_CSTRING("NC:ServicesourcePlayable"),
        &kNC_ServicesourcePlayable);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "Label"),
        &kNC_Label);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "Icon"),
        &kNC_Icon);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "URL"),
        &kNC_URL);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "Properties"),
        &kNC_Properties);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "Open"),
        &kNC_Open);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "DBGUID"),
        &kNC_DBGUID);
      gRDFService->GetResource(NS_LITERAL_CSTRING(NC_NAMESPACE_URI  "DBTable"),
        &kNC_DBTable);

      int i,j;
      for ( i = 0; i < NUM_PARENTS; i++ )
      {
        gRDFService->GetAnonymousResource( &(kNC_Parent[ i ]) );
      }
      for ( i = 0; i < NUM_PARENTS; i++ )
      for ( j = 0; j < MAX_CHILDREN; j++ )
      {

        gRDFService->GetAnonymousResource( &(kNC_Child[ i ][ j ]) );
      }

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
      gRDFService->GetResource(NS_LITERAL_CSTRING(RDF_NAMESPACE_URI "Seq"),
        &kRDF_Seq);

      gRDFService->GetLiteral(NS_LITERAL_STRING("true").get(),       
        &kLiteralTrue);
      gRDFService->GetLiteral(NS_LITERAL_STRING("false").get(),      
        &kLiteralFalse);
    }

    // Setup the list of playlists as a persistent query
    m_PlaylistsQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
    m_PlaylistsQuery->SetAsyncQuery( PR_TRUE );
    m_PlaylistsQuery->SetDatabaseGUID( NS_LITERAL_STRING("songbird") );

    MyServicesourceQueryCallback *callback;
    NS_NEWXPCOM(callback, MyServicesourceQueryCallback);
    if (!callback)
      NS_WARNING("Couldn't create MyServicesourceQueryCalback");

    if (callback) {
      callback->AddRef();
      m_PlaylistsQuery->AddSimpleQueryCallback( callback );
    }
    m_PlaylistsQuery->SetPersistentQuery( PR_TRUE );
    
    nsCOMPtr< sbIPlaylistManager > pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
    if(pPlaylistManager.get())
      pPlaylistManager->GetAllPlaylistList( m_PlaylistsQuery );

    gServicesource = this;
  }
}

void CServicesource::DeInit (void)
{
  VMETHOD_SHORTCIRCUIT;
#ifdef DEBUG_REFS
  --gInstanceCount;
  fprintf(stdout, "%d - RDF: CServicesource\n", gInstanceCount);
#endif
  {
    if ( nsnull != gRDFService )
    {

      NS_RELEASE(kNC_Servicesource);
      NS_RELEASE(kNC_ServicesourceFlat);
      NS_RELEASE(kNC_ServicesourcePlayable);
      NS_RELEASE(kNC_Label);
      NS_RELEASE(kNC_Icon);
      NS_RELEASE(kNC_URL);
      NS_RELEASE(kNC_Properties);
      NS_RELEASE(kNC_Open);
      NS_RELEASE(kNC_DBGUID);
      NS_RELEASE(kNC_DBTable);
      int i,j;
      for ( i = 0; i < NUM_PARENTS; i++ )
      {
        NS_RELEASE(kNC_Parent[ i ]);
      }
      for ( i = 0; i < NUM_PARENTS; i++ )
      for ( j = 0; j < MAX_CHILDREN; j++ )
      {
        NS_RELEASE(kNC_Child[ i ][ j ]);
      }
      NS_RELEASE(kRDF_InstanceOf);
      NS_RELEASE(kRDF_type);
      NS_RELEASE(gRDFService);
    }
    gServicesource = nsnull;
  }
}

NS_IMETHODIMP
CServicesource::GetURI(char **uri)
{
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(uri != nsnull, "null ptr");
  if (! uri)
    return NS_ERROR_NULL_POINTER;

  if ((*uri = NS_strdup("rdf:Servicesource")) == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}



NS_IMETHODIMP
CServicesource::GetSource(nsIRDFResource* property,
                    nsIRDFNode* target,
                    PRBool tv,
                    nsIRDFResource** source /* out */)
{
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
CServicesource::GetSources(nsIRDFResource *property,
                     nsIRDFNode *target,
                     PRBool tv,
                     nsISimpleEnumerator **sources /* out */)
{
  METHOD_SHORTCIRCUIT;
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
CServicesource::GetTarget(nsIRDFResource *source,
                    nsIRDFResource *property,
                    PRBool tv,
                    nsIRDFNode **target /* out */)
{
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

  *target = nsnull;

  nsresult        rv = NS_RDF_NO_VALUE;

  // we only have positive assertions in the file system data source.
  if (! tv)
    return NS_RDF_NO_VALUE;

  nsAutoString outstring;

  if ( ! outstring.Length() )
  {
    int i,j;
    for ( i = 0; i < NUM_PARENTS; i++ )
    {
      if ( source == kNC_Parent[ i ])
      {
        if (property == kNC_Label)
        {
          outstring = gParentLabels[ i ];
        }
        else if (property == kNC_Icon)
        {
          outstring = gParentIcons[ i ];
        }
        else if (property == kNC_URL)
        {
          outstring = gParentUrls[ i ];
        }
        else if (property == kNC_Properties)
        {
          outstring = NS_LITERAL_STRING( "" );
        }
        else if (property == kNC_Open)
        {
//          outstring = gParentOpen[ i ]; // no!
        }
      }
    }
    for ( i = 0; i < NUM_PARENTS; i++ )
    for ( j = 0; j < MAX_CHILDREN; j++ )
    {
      if ( source == kNC_Child[ i ][ j ] )
      {
        if (property == kNC_Label)
        {
          outstring = gChildLabels[ i ][ j ];
        }
        else if (property == kNC_Icon)
        {
          outstring = gChildIcons[ i ][ j ];
        }
        else if (property == kNC_URL)
        {
          outstring = gChildUrls[ i ][ j ];
        }
        else if (property == kNC_Properties)
        {
          outstring = NS_LITERAL_STRING( "" );
        }
        else if (property == kNC_Open)
        {
//          outstring = NS_LITERAL_STRING( "false" ); // no!
        }
      }
    }
  }


  // If it's not a hardcode source, check the playlists
  if ( ! outstring.Length() )
  {
    resmap_t::iterator pl = m_PlaylistMap.find( source );
    if ( pl != m_PlaylistMap.end() )
    {
      sbIDatabaseResult *resultset;
      m_PlaylistsQuery->GetResultObject( &resultset );

      if (property == kNC_Label)
      {
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("readable_name"), outstring);
      }
      else if (property == kNC_Icon)
      {
        // These icons come from the skin.
//        outstring = NS_LITERAL_STRING( "chrome://songbird/skin/default/icon_lib_16x16.png" );
      }
      else if (property == kNC_URL)
      {
        nsAutoString base_type, name, description, guid;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("base_type"), base_type);
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("name"), name);
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("description"), description);
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("service_uuid"), guid);

        // Is the value of the description url something we recognize as a playlist?  (html doesn't go here)
        PRBool isPlaylistURL = false;
        gPPS->IsPlaylistURL( description, &isPlaylistURL );

        // If this is an html dynamic playlist, show the html page instead of the full page playlist
        if ( base_type.EqualsLiteral("dynamic") && !isPlaylistURL )
        {
          outstring = description;
        }
        else
        {
          // If this is a library playlist, show the library
          outstring = gPlaylistUrl;
          if ( description == NS_LITERAL_STRING("library") )
          {
            outstring += NS_LITERAL_STRING("library");
          }
          else
          {
            // Otherwise, just show the normal playlist diplay.
            outstring += name;
          }
          // Lastly specify the database to which the playlist belongs.
          outstring += NS_LITERAL_STRING(",");
          outstring += guid;
        }
      }
      else if (property == kNC_Properties)
      {
        // Javascript code is assuming this ordering!
        outstring = NS_LITERAL_STRING( "playlist" );
        nsAutoString data;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("description"), data);
        if ( data == NS_LITERAL_STRING("library") )
        {
          outstring += NS_LITERAL_STRING(" library"); // Hack to represent libraries
        }
        else
        {
          resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("name"), data);
          outstring += NS_LITERAL_STRING(" ");
          outstring += data;
        }
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("service_uuid"), data);
        outstring += NS_LITERAL_STRING(" ");
        outstring += data;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("type"), data);
        outstring += NS_LITERAL_STRING(" ");
        outstring += data;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("base_type"), data);
        outstring += NS_LITERAL_STRING(" ");
        outstring += data;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("description"), data);
        outstring += NS_LITERAL_STRING(" ");
        outstring += data;
      }
      else if (property == kNC_Open)
      {
//        outstring = NS_LITERAL_STRING("false"); // no!
      }
      else if (property == kNC_DBGUID)
      {
        nsAutoString data;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("service_uuid"), data);
        outstring = data;
      }
      else if (property == kNC_DBTable)
      {
        nsAutoString data;
        resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("description"), data);
        if ( data == NS_LITERAL_STRING("library") )
        {
          outstring = NS_LITERAL_STRING("library"); // Hack to represent libraries
        }
        else
        {
          resultset->GetRowCellByColumn( (*pl).second, NS_LITERAL_STRING("name"), data);
          outstring = data;
        }
      }
    }
  }

  if ( outstring.Length() )
  {
    // Recompose from the stringbundle (hey, look, stringbundle code uses 'wstring' idl)
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
    if (rv == NS_RDF_NO_VALUE)  return(rv);

    return literal->QueryInterface(NS_GET_IID(nsIRDFNode), (void**) target);
  }

  return(NS_RDF_NO_VALUE);
}



NS_IMETHODIMP
CServicesource::GetTargets(nsIRDFResource *source,
                     nsIRDFResource *property,
                     PRBool tv,
                     nsISimpleEnumerator **targets /* out */)
{
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

  // we only have positive assertions in the data source.
  if (! tv)
    return NS_RDF_NO_VALUE;

  if (source == kNC_Servicesource)
  {
    if (property == kNC_child)
    {
      nsresult rv = NS_OK;
      *targets = NULL;

      // Make the array to hold our response
      nsCOMArray<nsIRDFResource> nextItemArray;

      // Append items to the array
      for ( int i = 0; i < NUM_PARENTS; i++ )
      {
        nextItemArray.AppendObject( kNC_Parent[ i ] );
        // Magic stuff to list the playlists
        if ( i == nPlaylistsInsert )
        {
          sbIDatabaseResult *resultset;
          m_PlaylistsQuery->GetResultObject( &resultset );
          PRInt32 num_rows;
          resultset->GetRowCount( &num_rows );

          for( PRInt32 j = 0; j < num_rows; j++ )
          {
            // Create a new resource for this item
            nsIRDFResource *next_resource;
            gRDFService->GetAnonymousResource( &next_resource );
            nextItemArray.AppendObject( next_resource );
            m_PlaylistMap[ next_resource ] = j;
          }

          resultset->Release();
        }
      }
      return NS_NewArrayEnumerator(targets, nextItemArray);
    }
  }
  else if (source == kNC_ServicesourceFlat)
  {
    if (property == kNC_child)
    {
      nsresult rv = NS_OK;
      *targets = NULL;

      // Make the array to hold our response
      nsCOMArray<nsIRDFResource> nextItemArray;

      // Append items to the array
      for ( int i = 0; i < NUM_PARENTS; i++ )
      {
        // Only add the parent if it has no children
        if ( ! gChildLabels[ i ][ 0 ].Length() )
        {
          nextItemArray.AppendObject( kNC_Parent[ i ] );
        }
        else
        {
          // Otherwise add the children
          for ( int j = 0; j < MAX_CHILDREN; j++ )
          {
            // If there is a child, append it.
            if ( gChildLabels[ i ][ j ].Length() )
            {
              nextItemArray.AppendObject( kNC_Child[ i ][ j ] );
            }
          }
        }
      }
      return NS_NewArrayEnumerator(targets, nextItemArray);
    }
  }
  if (source == kNC_ServicesourcePlayable)
  {
    if (property == kNC_child)
    {
      nsresult rv = NS_OK;
      *targets = NULL;

      // Make the array to hold our response
      nsCOMArray<nsIRDFResource> nextItemArray;

      nextItemArray.AppendObject( kNC_Parent[ nPlaylistsInsert ] );

      sbIDatabaseResult *resultset;
      m_PlaylistsQuery->GetResultObject( &resultset );
      PRInt32 num_rows;
      resultset->GetRowCount( &num_rows );

      for( PRInt32 j = 0; j < num_rows; j++ )
      {
        // Create a new resource for this item
        nsIRDFResource *next_resource;
        gRDFService->GetAnonymousResource( &next_resource );
        nextItemArray.AppendObject( next_resource );
        m_PlaylistMap[ next_resource ] = j;
      }

      resultset->Release();

      return NS_NewArrayEnumerator(targets, nextItemArray);
    }
  }
  else 
  {
    if ( property == kNC_child )
    {
      int i,j;
      for ( i = 0; i < NUM_PARENTS; i++ )
      {
        if ( source == kNC_Parent[ i ])
        {
          nsresult rv = NS_OK;
          *targets = NULL;

          // Make the array to hold our response
          nsCOMArray<nsIRDFResource> nextItemArray;

          {
            // Append items to the array
            for ( j = 0; j < MAX_CHILDREN; j++ )
            {
              // If there is a label for this element, append it.
              if ( gChildLabels[ i ][ j ].Length() )
              {
                nextItemArray.AppendObject( kNC_Child[ i ][ j ] );
              }
            }
          }
          return NS_NewArrayEnumerator(targets, nextItemArray);
        }
      }
    }
  }
  return NS_NewEmptyEnumerator(targets);
}



NS_IMETHODIMP
CServicesource::Assert(nsIRDFResource *source,
                 nsIRDFResource *property,
                 nsIRDFNode *target,
                 PRBool tv)
{
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
CServicesource::Unassert(nsIRDFResource *source,
                   nsIRDFResource *property,
                   nsIRDFNode *target)
{
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
CServicesource::Change(nsIRDFResource* aSource,
                 nsIRDFResource* aProperty,
                 nsIRDFNode* aOldTarget,
                 nsIRDFNode* aNewTarget)
{
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
CServicesource::Move(nsIRDFResource* aOldSource,
               nsIRDFResource* aNewSource,
               nsIRDFResource* aProperty,
               nsIRDFNode* aTarget)
{
  METHOD_SHORTCIRCUIT;
  return NS_RDF_ASSERTION_REJECTED;
}



NS_IMETHODIMP
CServicesource::HasAssertion(nsIRDFResource *source,
                       nsIRDFResource *property,
                       nsIRDFNode *target,
                       PRBool tv,
                       PRBool *hasAssertion /* out */)
{
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

//  if ( source == kNC_Servicesource )
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
CServicesource::HasArcIn(nsIRDFNode *aNode, nsIRDFResource *aArc, PRBool *result)
{
  METHOD_SHORTCIRCUIT;
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP 
CServicesource::HasArcOut(nsIRDFResource *aSource, nsIRDFResource *aArc, PRBool *result)
{
  METHOD_SHORTCIRCUIT;
  *result = PR_FALSE;

  if (aSource == kNC_Servicesource)
  {
    *result = ( aArc == kNC_child );
  }
  else
  {
    int i;
    for ( i = 0; i < NUM_PARENTS; i++ )
    {
      if ( aSource == kNC_Parent[ i ] )
      {
        *result = ( aArc == kNC_child );
      }
    }
  }
  return NS_OK;
}



NS_IMETHODIMP
CServicesource::ArcLabelsIn(nsIRDFNode *node,
                      nsISimpleEnumerator ** labels /* out */)
{
  METHOD_SHORTCIRCUIT;
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
CServicesource::ArcLabelsOut(nsIRDFResource *source,
                       nsISimpleEnumerator **labels /* out */)
{
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(source != nsnull, "null ptr");
  if (! source)
    return NS_ERROR_NULL_POINTER;

  NS_PRECONDITION(labels != nsnull, "null ptr");
  if (! labels)
    return NS_ERROR_NULL_POINTER;

  /*
  nsresult rv;

  if (source == kNC_Servicesource)
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
CServicesource::GetAllResources(nsISimpleEnumerator** aCursor)
{
  METHOD_SHORTCIRCUIT;
  NS_NOTYETIMPLEMENTED("sorry!");
  return NS_ERROR_NOT_IMPLEMENTED;
}



NS_IMETHODIMP
CServicesource::AddObserver(nsIRDFObserver *n)
{
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(n != nsnull, "null ptr");
  if (! n)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIRDFObserver> ob = n;
  m_Observers.insert( ob );

  return NS_OK;
}



NS_IMETHODIMP
CServicesource::RemoveObserver(nsIRDFObserver *n)
{
  METHOD_SHORTCIRCUIT;
  NS_PRECONDITION(n != nsnull, "null ptr");
  if (! n)
    return NS_ERROR_NULL_POINTER;

  nsCOMPtr<nsIRDFObserver> ob = n;

  observers_t::iterator o = m_Observers.find( ob );
  if ( o != m_Observers.end() )
  {
    m_Observers.erase( o );
  }

  return NS_OK;
}


NS_IMETHODIMP
CServicesource::GetAllCmds(nsIRDFResource* source,
                     nsISimpleEnumerator/*<nsIRDFResource>*/** commands)
{
  return(NS_NewEmptyEnumerator(commands));
}



NS_IMETHODIMP
CServicesource::IsCommandEnabled(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                           nsIRDFResource*   aCommand,
                           nsISupportsArray/*<nsIRDFResource>*/* aArguments,
                           PRBool* aResult)
{
  METHOD_SHORTCIRCUIT;
  return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
CServicesource::DoCommand(nsISupportsArray/*<nsIRDFResource>*/* aSources,
                    nsIRDFResource*   aCommand,
                    nsISupportsArray/*<nsIRDFResource>*/* aArguments)
{
  METHOD_SHORTCIRCUIT;
  return(NS_ERROR_NOT_IMPLEMENTED);
}



NS_IMETHODIMP
CServicesource::BeginUpdateBatch()
{
  METHOD_SHORTCIRCUIT;
  return NS_OK;
}



NS_IMETHODIMP
CServicesource::EndUpdateBatch()
{
  METHOD_SHORTCIRCUIT;
  return NS_OK;
}

/*

These are copied straight from the Playlistsource

*/
NS_IMETHODIMP
CServicesource::RegisterPlaylistCommands(const nsAString     &aContextGUID,
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
CServicesource::GetPlaylistCommands(const nsAString      &aContextGUID,
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


