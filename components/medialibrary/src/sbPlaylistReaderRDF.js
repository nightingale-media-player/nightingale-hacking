/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

//
// sbIPlaylistReader Object (RSS)
//

const SONGBIRD_PLAYLISTRDF_CONTRACTID = "@songbirdnest.com/Songbird/Playlist/Reader/RDF;1";
const SONGBIRD_PLAYLISTRDF_CLASSNAME = "Songbird RDF Playlist Interface";
const SONGBIRD_PLAYLISTRDF_CID = Components.ID("{68008842-0c55-4c53-8e9f-5eb00e31adf6}");
const SONGBIRD_PLAYLISTRDF_IID = Components.interfaces.sbIPlaylistReader;

function CPlaylistRDF()
{
  jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/songbird_interfaces.js", this );
  this.gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
}

/* I actually need a constructor in this case. */
CPlaylistRDF.prototype.constructor = CPlaylistRDF;

/* the CPlaylistRDF class def */
CPlaylistRDF.prototype = 
{
  originalURL: "",
  
  m_document: null,
  m_playlistmgr: null,
  m_playlist: null,
  m_library: null,
  m_query: null,
  
  m_guid: "",
  m_table: "",
  m_append: false,
  
  read: function( strURL, strGUID, strDestTable, bAppendOrReplace, /* out */ errorCode )
  {
    try
    {
      errorCode.value = 0;

      var domParser = Components.classes["@mozilla.org/xmlextras/domparser;1"].createInstance(Components.interfaces.nsIDOMParser);
      var pFileReader = (Components.classes["@mozilla.org/network/file-input-stream;1"]).createInstance(Components.interfaces.nsIFileInputStream);
      var pFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      var pURI = (Components.classes["@mozilla.org/network/simple-uri;1"]).createInstance(Components.interfaces.nsIURI);

      if ( domParser && pFile && pURI && pFileReader )
      {
        pURI.spec = strURL;
        if ( pURI.scheme == "file" )
        {
          var cstrPathToPLS = pURI.path;
          cstrPathToPLS = cstrPathToPLS.substr( 3, cstrPathToPLS.length );
          pFile.initWithPath(cstrPathToPLS);
          if ( pFile.isFile() )
          {
            pFileReader.init( pFile, /* PR_RDONLY */ 1, 0, /*nsIFileInputStream::CLOSE_ON_EOF*/ 0 );
            var document = domParser.parseFromStream(pFileReader, null, pFileReader.available(), "application/xml");
            pFileReader.close();
            
            if(document)
            {
              const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
              const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");

              this.m_guid = strGUID;
              this.m_table = strDestTable;
              this.m_append = bAppendOrReplace;
              
              this.m_query = new this.sbIDatabaseQuery();
              this.m_query.setAsyncQuery(true);
              this.m_query.setDatabaseGUID(strGUID);

              this.m_document = document;
              this.m_library = new MediaLibrary();
              this.m_playlistmgr = new PlaylistManager();
              
              this.m_library.setQueryObject(this.m_query);
              var playlist = this.m_playlistmgr.getPlaylist(strDestTable, this.m_query);
              
              if(playlist)
              {
                this.m_playlist = playlist;
                return this.processXMLDocument(this.m_document);
              }
            }
          }
        }
      }
    }
    catch ( err )
    {
      throw "CPlaylistRDF::read - " + err;
    }
    
    return false;
  },
  
  vote: function( strURL )
  {
    try
    {
      return 10000;
    }
    catch ( err )
    {
      throw "CPlaylistRDF::vote - " + err;
    }
  },
  
  name: function()
  {
    try
    {
      return "RDF Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistRDF::name - " + err;
    }
  },
  
  description: function()
  {
    try
    {
      return "RDF Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistRDF::description - " + err;
    }
  },
  
  supportedMIMETypes: function( /* out */ nMIMECount )
  {
    var retval = new Array;
    nMIMECount.value = 0;
    
    try
    {
      retval.push( "text/xml" );
      retval.push( "text/rdf+xml" );
      retval.push( "application/xml" );
      nMIMECount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistRDF::supportedMIMETypes - " + err;
    }
    return retval;
  },
  
  supportedFileExtensions: function( /* out */ nExtCount )
  {
    var retval = new Array;
    nExtCount.value = 0;
    
    try
    {
      retval.push( "rdf" );
      retval.push( "xml" );
      
      nExtCount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistRDF::supportedFileExtensions - " + err;
    }
    
    return retval;
  },
  
  processXMLDocument: function( xmlDocument )
  {
    var ret = false;
    try
    {
      var document = xmlDocument.documentElement;
      dump("CPlaylistRDF::processXMLDocument - Tag Name: " + document.tagName + "\n");
      
      if(document.tagName == "rdf:RDF")
      {
        dump("CPlaylistRDF::processXMLDocument - RSS/RDF Version: 1.0\n");
        ret = this.processRDFFeed(document);
      }
    }      
    catch(err)
    {
      throw "CPlaylistRDF::ProcessXMLDocument - " + err;
    }
    
    return ret;
  },
  
  processRDFFeed: function( xmlDocument )
  {
    try
    {
      
    }
    catch(err)
    {
      throw "CPlaylistRDF::ProcessXMLDocument - " + err;
    }
    
    return false;
  },
  
  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_PLAYLISTRDF_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
}; //CPlaylistRDF

/**
 * \class sbPlaylistRDFModule
 * \brief 
 */
var sbPlaylistRDFModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTRDF_CID, 
                                    SONGBIRD_PLAYLISTRDF_CLASSNAME, 
                                    SONGBIRD_PLAYLISTRDF_CONTRACTID, 
                                    fileSpec, 
                                    location,
                                    type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if(!cid.equals(SONGBIRD_PLAYLISTRDF_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if(!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbPlaylistRDFFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbPlaylistRDFModule

/**
 * \class sbPlaylistRDFFactory
 * \brief 
 */
var sbPlaylistRDFFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_PLAYLISTRDF_IID) &&
        !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

    return (new CPlaylistRDF()).QueryInterface(iid);
  }
}; //sbPlaylistRDFFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistRDFModule;
} //NSGetModule
