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
// sbIPlaylistReader Object (HTML)
//

const SONGBIRD_PLAYLISTHTML_CONTRACTID = "@songbirdnest.com/Songbird/Playlist/Reader/HTML;1";
const SONGBIRD_PLAYLISTHTML_CLASSNAME = "Songbird HTML Playlist Interface";
const SONGBIRD_PLAYLISTHTML_CID = Components.ID("{2a9656c6-ba21-4fa1-8578-3c8a11aecab8}");
const SONGBIRD_PLAYLISTHTML_IID = Components.interfaces.sbIPlaylistReader;

function CPlaylistHTML()
{
  jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/songbird_interfaces.js", this );
  this.gPPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
}

/* I actually need a constructor in this case. */
CPlaylistHTML.prototype.constructor = CPlaylistHTML;

/* the CPlaylistHTML class def */
CPlaylistHTML.prototype = 
{
  originalURL: "",

  m_HREFArray: new Array(),

  read: function( strURL, strGUID, strDestTable, replace, /* out */ errorCode )
  {
    try
    {
      // Twick the filename off the end of the url.
      if ( this.originalURL && this.originalURL.length )
      {
        this.originalURL = this.originalURL.substr( 0, this.originalURL.lastIndexOf("/") ) + "/";
      }
      
      // Setup the file input stream and pass it to a function to parse the html
      var retval = false;
      errorCode.value = 0;
      
      var pFileReader = (Components.classes["@mozilla.org/network/file-input-stream;1"]).createInstance(Components.interfaces.nsIFileInputStream);
      var pFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
      var pURI = (Components.classes["@mozilla.org/network/simple-uri;1"]).createInstance(Components.interfaces.nsIURI);

      if ( pFile && pURI && pFileReader )
      {
        pURI.spec = strURL;
        if ( pURI.scheme == "file" )
        {
          var cstrPathToPLS = pURI.path;
          cstrPathToPLS = cstrPathToPLS.substr( 3, cstrPathToPLS.length ); // .Cut(0, 3); //remove '///'
          pFile.initWithPath(cstrPathToPLS);
          if ( pFile.isFile() )
          {
            pFileReader.init( pFile, /* PR_RDONLY */ 1, 0, /*nsIFileInputStream::CLOSE_ON_EOF*/ 0 );
            retval = this.HandleHTMLStream( strGUID, strDestTable, pFileReader, replace );
            pFileReader.close();
          }
        }
      }
      return retval;
    }
    catch ( err )
    {
      throw "CPlaylistHTML::read - " + err;
    }
  },
  
  vote: function( strURL )
  {
    try
    {
      return 10000;
    }
    catch ( err )
    {
      throw "CPlaylistHTML::vote - " + err;
    }
  },
  
  name: function()
  {
    try
    {
      return "HTML Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistHTML::name - " + err;
    }
  },
  
  description: function()
  {
    try
    {
      return "HTML Playlist"; // ????
    }
    catch ( err )
    {
      throw "CPlaylistHTML::description - " + err;
    }
  },
  
  supportedMIMETypes: function( /* out */ nMIMECount )
  {
    var retval = new Array;
    nMIMECount.value = 0;

    try
    {
      retval.push( "text/html" );
      nMIMECount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistHTML::supportedMIMETypes - " + err;
    }
    return retval;
  },
  
  supportedFileExtensions: function( /* out */ nExtCount )
  {
    var retval = new Array;
    nExtCount.value = 0;
    
    try
    {
      retval.push( "html" );
      retval.push( "htm" );
      retval.push( "php" );
      retval.push( "php3" );
      retval.push( "" );
      
      nExtCount.value = retval.length;
    }
    catch ( err )
    {
      throw "CPlaylistHTML::supportedFileExtensions - " + err;
    }
    
    return retval;
  },
  
  HandleHTMLStream: function( strGUID, strDestTable, streamIn, replace )
  {
    var retval = false;
    try
    {
      streamIn = streamIn.QueryInterface( Components.interfaces.nsILineInputStream );
      var links = new Array();
      var inTag = false;
      var inATag = false;
      // Parse the file (line by line! woo hoo!)
      var out = {};
      var count = 0;
      while ( streamIn.readLine( out ) )
      {
        ++count;
        var line = out.value;
        while ( line.length && line.length > 0 )
        {
          // What state were we in when we last got in here?
          if ( inTag )
          {
            // See if we can find an href attribute
            if ( inATag )
            {
              var href_idx = line.indexOf( 'href="' );
              if ( href_idx > -1 )
              {
                var temp = line.substr( href_idx + 6, line.length );
                var href = temp.substr( 0, temp.indexOf('"') );
                if ( this.gPPS.isMediaURL( href ) )
                {
                  if ( href.indexOf( '://' ) == -1 )
                  {
                    href = this.originalURL + href;
                  }
                  links.push( href );
                }
              }
            }
            // Look for the closing of a tag
            var close = line.indexOf( '>' );
            if ( close > -1 )
            {
              inTag = false;
              inATag = false;
              line = line.substr( close + 1, line.length );
            }
            else
            {
              break; // EOL
            }
          }
          else
          {
            // Look for the opening of a tag
            var open = line.indexOf( '<' );
            if ( open > -1 )
            {
              inTag = true;
              line = line.substr( open + 1, line.length );
              if ( ( line[ 0 ] == "a" ) || ( line[ 0 ] == "A" ) )
              {
                inATag = true;
              }
            }
            else
            {
              break; // EOL
            }
          }
        }
      }
      // I'm tired of this function.  Let's have a different one.
      retval = this.createPlaylist( strGUID, strDestTable, links, replace );
    }
    catch ( err )
    {
      throw "CPlaylistHTML::HandleHTMLStream - " + err;
    }
    return retval;
  },
  
  createPlaylist: function( strGUID, strDestTable, links_array, replace )
  {
    try
    {
      retval = false;
      if ( links_array.length > 0 )
      {
        var pQuery = new this.sbIDatabaseQuery();
        pQuery.setAsyncQuery(true);
        pQuery.setDatabaseGUID(strGUID);

        const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
        var pLibrary = new MediaLibrary();
        const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
        var pPlaylistManager = new PlaylistManager();
        
        pLibrary.setQueryObject(pQuery);
        pPlaylist = pPlaylistManager.getPlaylist(strDestTable, pQuery);
        
        dump( "**********\nstrDestTable - " + strDestTable + "\nstrGUID - " + strGUID + "\npPlaylist - " + pPlaylist + "\n" );

        var inserted = new Array();        
        for ( var i in links_array )
        {
          var url = links_array[ i ];
          
          // Don't insert it if we already did.
          var skip = false;
          for ( var j in inserted )
          {
            if ( inserted[ j ] == url )
            {
              skip = true;
              break;
            }
          }
        
          if ( !skip )
          {
            var aMetaKeys = new Array("title");
            var aMetaValues = new Array( this.gPPS.convertURLToDisplayName( url ) );

            var guid = pLibrary.addMedia( url, aMetaKeys.length, aMetaKeys, aMetaValues.length, aMetaValues, replace, true );
            pPlaylist.addByGUID( guid, strGUID, -1, replace, true );
            
            inserted.push( url );
          }
        }
        var ex = pQuery.execute();
        retval = true;
      }
      return retval;
    }
    catch ( err )
    {
      var error = "\nCPlaylistHTML::createPlaylist\n" + strGUID + "\n" + strDestTable + "\n" + links_array + "\n" + replace + "\n" + err;
      dump( error );
      throw error;
    }
  },

  QueryInterface: function(iid)
  {
      if (!iid.equals(Components.interfaces.nsISupports) &&
          !iid.equals(SONGBIRD_PLAYLISTHTML_IID))
          throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
  }
};

/**
 * \class sbPlaylistHTMLModule
 * \brief 
 */
var sbPlaylistHTMLModule = 
{
  //firstTime: true,
  
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    //if(this.firstTime)
    //{
      //this.firstTime = false;
      //throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
    //}
 
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTHTML_CID, 
                                    SONGBIRD_PLAYLISTHTML_CLASSNAME, 
                                    SONGBIRD_PLAYLISTHTML_CONTRACTID, 
                                    fileSpec, 
                                    location,
                                    type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if(!cid.equals(SONGBIRD_PLAYLISTHTML_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if(!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbPlaylistHTMLFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
};

/**
 * \class sbPlaylistHTMLFactory
 * \brief 
 */
var sbPlaylistHTMLFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_PLAYLISTHTML_IID) &&
        !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

    return (new CPlaylistHTML()).QueryInterface(iid);
  }
}; //sbPlaylistHTMLFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistHTMLModule;
} //NSGetModule
