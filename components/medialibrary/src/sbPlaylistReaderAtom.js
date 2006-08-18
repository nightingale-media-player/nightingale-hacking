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

const SONGBIRD_PLAYLISTATOM_CONTRACTID = "@songbirdnest.com/Songbird/Playlist/Reader/Atom;1";
const SONGBIRD_PLAYLISTATOM_CLASSNAME = "Songbird Atom Playlist Interface";
const SONGBIRD_PLAYLISTATOM_CID = Components.ID("{de42abd8-993e-4f68-ad50-bffab2a66c12}");
const SONGBIRD_PLAYLISTATOM_IID = Components.interfaces.sbIPlaylistReader;

function CPlaylistAtom()
{
  this.gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
}

/* I actually need a constructor in this case. */
CPlaylistAtom.prototype.constructor = CPlaylistAtom;

/* the CPlaylistAtom class def */
CPlaylistAtom.prototype = 
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
              const DatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");

              this.m_guid = strGUID;
              this.m_table = strDestTable;
              this.m_append = bAppendOrReplace;
              
              this.m_query = new DatabaseQuery();
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
      throw "CPlaylistAtom::read - " + err;
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
      throw "CPlaylistAtom::vote - " + err;
    }
  },
  
  name: function()
  {
    try
    {
      return "Atom Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistAtom::name - " + err;
    }
  },
  
  description: function()
  {
    try
    {
      return "Atom Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistAtom::description - " + err;
    }
  },
  
  supportedMIMETypes: function( /* out */ nMIMECount )
  {
    var retval = new Array;
    nMIMECount.value = 0;
    
    try
    {
      retval.push( "text/xml" );
      retval.push( "text/atom+xml" );
      retval.push( "application/xml" );
      
      nMIMECount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistAtom::supportedMIMETypes - " + err;
    }
    return retval;
  },
  
  supportedFileExtensions: function( /* out */ nExtCount )
  {
    var retval = new Array;
    nExtCount.value = 0;
    
    try
    {
      retval.push( "xml" );
      retval.push( "atom" );

      nExtCount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistAtom::supportedFileExtensions - " + err;
    }
    
    return retval;
  },
  
  processXMLDocument: function( xmlDocument )
  {
    var ret = false;
    try
    {
      var document = xmlDocument.documentElement;
      dump("CPlaylistAtom::ProcessXMLDocument - Tag Name: " + document.tagName + "\n");
      
      if(document.tagName == "feed")
      {
        var atomVersion = document.getAttribute("version");
        dump("CPlaylistAtom::ProcessXMLDocument - Atom Version: " + atomVersion + "\n");
        
        if(atomVersion[0] == "1" ||
           atomVersion == "0.3" )
        {
          ret = this.ProcessAtomFeed(document);
        }
      }
    }      
    catch(err)
    {
      throw "CPlaylistAtom::ProcessXMLDocument - " + err;
    }
    
    return ret;
  },
  
  processAtomFeed: function( xmlDocument )
  {
    try
    {
      
    }
    catch(err)
    {
      throw "CPlaylistAtom::ProcessAtomDocument - " + err;
    }
    
    return false;
  },
  
  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_PLAYLISTATOM_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
}; //CPlaylistAtom

/**
 * \class sbPlaylistAtomModule
 * \brief 
 */
var sbPlaylistAtomModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTATOM_CID, 
                                    SONGBIRD_PLAYLISTATOM_CLASSNAME, 
                                    SONGBIRD_PLAYLISTATOM_CONTRACTID, 
                                    fileSpec, 
                                    location,
                                    type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if(!cid.equals(SONGBIRD_PLAYLISTATOM_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if(!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbPlaylistAtomFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbPlaylistAtomModule

/**
 * \class sbPlaylistAtomFactory
 * \brief 
 */
var sbPlaylistAtomFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_PLAYLISTATOM_IID) &&
        !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

    return (new CPlaylistAtom()).QueryInterface(iid);
  }
}; //sbPlaylistAtomFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistAtomModule;
} //NSGetModule
