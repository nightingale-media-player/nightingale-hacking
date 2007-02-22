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

//
// sbIPlaylistReader Object (RSS)
//

const SONGBIRD_PLAYLISTRSS_CONTRACTID = "@songbirdnest.com/Songbird/Playlist/Reader/RSS;1";
const SONGBIRD_PLAYLISTRSS_CLASSNAME = "Songbird RSS Playlist Interface";
const SONGBIRD_PLAYLISTRSS_CID = Components.ID("{4c55f911-0174-4a73-8a41-949459585aec}");
const SONGBIRD_PLAYLISTRSS_IID = Components.interfaces.sbIPlaylistReader;

function CPlaylistRSS()
{
  this._uriFixup = null;
  
  try {
    var urifixup = Components.classes["@mozilla.org/docshell/urifixup;1"]
                              .getService(Components.interfaces.nsIURIFixup);
    this._uriFixup = urifixup;
  }
  catch (e) {
    Components.utils.reportError(e);
  }
  
  this.gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
}

/* I actually need a constructor in this case. */
CPlaylistRSS.prototype.constructor = CPlaylistRSS;

/* the CPlaylistRSS class def */
CPlaylistRSS.prototype = 
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
              this.m_query.setAsyncQuery(false);
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
      throw "CPlaylistRSS::read - " + err;
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
      throw "CPlaylistRSS::vote - " + err;
    }
  },
  
  name: function()
  {
    try
    {
      return "RSS Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistRSS::name - " + err;
    }
  },
  
  description: function()
  {
    try
    {
      return "RSS Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistRSS::description - " + err;
    }
  },
  
  supportedMIMETypes: function( /* out */ nMIMECount )
  {
    var retval = new Array;
    nMIMECount.value = 0;
    
    try
    {
      retval.push( "text/rss+xml" );
      retval.push( "application/rss+xml" );
      
      nMIMECount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistRSS::supportedMIMETypes - " + err;
    }
    return retval;
  },
  
  supportedFileExtensions: function( /* out */ nExtCount )
  {
    var retval = new Array;
    nExtCount.value = 0;
    
    try
    {
      retval.push( "rss" );
      retval.push( "xml" );
      
      nExtCount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistRSS::supportedFileExtensions - " + err;
    }
    
    return retval;
  },
  
  processXMLDocument: function( xmlDocument )
  {
    var ret = false;
    try
    {
      var document = xmlDocument.documentElement;
      
      if(document.tagName == "rss")
      {
        var rssVersion = document.getAttribute("version");
        dump("CPlaylistRSS::processXMLDocument - RSS Version: " + rssVersion + "\n");
        
        if(rssVersion[0] == "2")
        {
          ret = this.processRSSFeed(document);
        }
      }
    }
    catch(err)
    {
      throw "CPlaylistRSS::processXMLDocument - " + err;
    }
    
    return ret;
  },
  
  processRSSFeed: function( xmlDocument )
  {
    try
    {
      var channels = xmlDocument.getElementsByTagName("channel");
      var i = 0;
      
      for(; i < channels.length; i++)
      {
        var playlistTitle = "";
        var playlistDescription = "";
        
        var channel = channels.item(i);
        
        var children = channel.childNodes;
        var j = 0;
        for(; j < children.length; j++)
        {
          var child = children.item(j);
          
          switch(child.nodeName)
          {
            case "title": this.m_playlist.setReadableName(child.firstChild.data); break;
            case "description": playlistDescription = ""; break;
            case "item": this.processRSSItem(child); break;
          }
        }
        
        var ret = this.m_query.execute();
        
        return (ret == 0);
      }
    }
    catch(err)
    {
      throw "CPlaylistRSS::processRSSDocument - " + err;
    }
    
    return false;
  },
  
  processRSSItem: function(xmlElement)
  {
    try
    {
      var url = "";
      var artist = "";
      var title = "";
      var type = "";
      var size = "";

      var i = 0;
      var children = xmlElement.childNodes;
      for(; i < children.length; i++)
      {
        var child = children.item(i);
        
        switch(child.nodeName)
        {
          case "title":
          {
            if(child.hasChildNodes() && child.firstChild.nodeType == 3)
            {
              title = child.firstChild.data;
            }
          }
          break;

          case "itunes:author":
          {
            if(child.hasChildNodes() && child.firstChild.nodeType == 3)
            {
              artist = child.firstChild.data;
            }
          }
          break;
          
          case "enclosure":
          {
            if(child.hasAttributes() && child.nodeType == 1)
            {
              url = child.getAttribute("url");
              type = child.getAttribute("type");
              size = child.getAttribute("length");
            }
          }
          break;
        }
      }
      
      if(url != "" && this.gPPS.isMediaURL(url))
      {
        var aMetaKeys = new Array("artist", "title", "content_type");
        var aMetaValues = new Array( artist, title, type );

        var urlURI = this._uriFixup.createFixupURI(url, 0);

        var guid = this.m_library.addMedia( urlURI.spec, aMetaKeys.length, aMetaKeys, aMetaValues.length, aMetaValues, this.m_append, true );
        this.m_playlist.addByGUID( guid, this.m_guid, -1, this.m_append, true );
        
        return true;
      }
    }
    catch(err)
    {
      throw "CPlaylistRSS::ProcessRSSItem - " + err;
    }
  },
  
  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_PLAYLISTRSS_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
}; //CPlaylistRSS

/**
 * \class sbPlaylistRSSModule
 * \brief 
 */
var sbPlaylistRSSModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTRSS_CID, 
                                    SONGBIRD_PLAYLISTRSS_CLASSNAME, 
                                    SONGBIRD_PLAYLISTRSS_CONTRACTID, 
                                    fileSpec, 
                                    location,
                                    type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if(!cid.equals(SONGBIRD_PLAYLISTRSS_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if(!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbPlaylistRSSFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbPlaylistRSSModule

/**
 * \class sbPlaylistHTMLFactory
 * \brief 
 */
var sbPlaylistRSSFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_PLAYLISTRSS_IID) &&
        !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

    return (new CPlaylistRSS()).QueryInterface(iid);
  }
}; //sbPlaylistRSSFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistRSSModule;
} //NSGetModule
