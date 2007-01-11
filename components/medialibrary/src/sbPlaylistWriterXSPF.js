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
// sbIPlaylistWriter Object (XSPF)
//

const SONGBIRD_PLAYLISTWRITERXSPF_CONTRACTID = "@songbirdnest.com/Songbird/Playlist/Writer/XSPF;1";
const SONGBIRD_PLAYLISTWRITERXSPF_CLASSNAME = "Songbird XSPF Playlist Interface";
const SONGBIRD_PLAYLISTWRITERXSPF_CID = Components.ID("{5FD80C2D-FA1B-4d4c-A118-AA571B16E307}");
const SONGBIRD_PLAYLISTWRITERXSPF_IID = Components.interfaces.sbIPlaylistWriter;

function CPlaylistWriterXSPF()
{
  this.m_guid = null;
  this.m_table = null;
  this.m_playlistmgr = null;
  this.m_playlist = null;
  this.m_library = null;
  this.m_query = null;
  this.m_listener = null;
  this.m_document = null;
};

/* I actually need a constructor in this case. */
CPlaylistWriterXSPF.prototype.constructor = CPlaylistWriterXSPF();

/* the CPlaylistWriterXSPF class def */
CPlaylistWriterXSPF.prototype.createEntry = function()
{
  return null; 
};

CPlaylistWriterXSPF.prototype.writeXSPF = function(playlist, url, contenttype)
{
  playlist.getAllEntries();
  
};

CPlaylistWriterXSPF.prototype.write = function(aGUID, aSourcePlaylist, aOutputURL, aOutputContentType, aErrorCode)
{
  const MediaLibrary = new Components.Constructor("@songbirdnest.com/Songbird/MediaLibrary;1", "sbIMediaLibrary");
  const PlaylistManager = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistManager;1", "sbIPlaylistManager");
  const DatabaseQuery = new Components.Constructor("@songbirdnest.com/Songbird/DatabaseQuery;1", "sbIDatabaseQuery");

  aErrorCode.value = 0;
  
  if(!aGUID) throw "CPlaylistWriterXSPF::write aGUID cannot be null!";
  if(!aSourcePlaylist) throw "CPlaylistWriterXSPF::write aSourcePlaylist cannot be null!";
  
  this.m_guid = aGUID;
  this.m_table = aSourcePlaylist;
  
  this.m_document = document.implementation.createDocument("", "", null);
  
  this.m_query = new DatabaseQuery();
  this.m_query.setAsyncQuery(false);
  this.m_query.setDatabaseGUID(this.m_guid);

  this.m_library = new MediaLibrary();
  this.m_playlistmgr = new PlaylistManager();
  
  this.m_library.setQueryObject(this.m_query);
  var playlist = this.m_playlistmgr.getPlaylist(this.m_table, this.m_query);
  
  if(playlist)
  {
    aErrorCode.value = this.writeXSPF(playlist, aOutputURL, aOutputContentType);
    return true;
  }
  
  return false;
};

CPlaylistWriterXSPF.prototype.vote = function( strURL )
{
  try
  {
    return 10000;
  }
  catch ( err )
  {
    throw "CPlaylistWriterXSPF::vote - " + err;
  }
};
  
CPlaylistWriterXSPF.prototype.name = function()
{
  try
  {
    return "XSPF Playlist Writer"; // ????
  }
  catch ( err )
  {
    throw "CPlaylistWriterXSPF::name - " + err;
  }
};
  
CPlaylistWriterXSPF.prototype.description = function()
{
  try
  {
    return "XSPF Playlist Writer"; // ????
  }
  catch ( err )
  {
    throw "CPlaylistWriterXSPF::description - " + err;
  }
};
  
CPlaylistWriterXSPF.prototype.supportedMIMETypes = function( /* out */ nMIMECount )
{
  var retval = new Array;
  nMIMECount.value = 0;
  
  try
  {
    retval.push( "application/xspf+xml" );
    nMIMECount.value = retval.length;
  }
  catch ( err )
  {
    throw "CPlaylistWriterXSPF::supportedMIMETypes - " + err;
  }
  return retval;
};
  
CPlaylistWriterXSPF.prototype.supportedFileExtensions = function( /* out */ nExtCount )
{
  var retval = new Array;
  nExtCount.value = 0;
  
  try
  {
    retval.push( "xspf" );
    retval.push( "xml" );
    nExtCount.value = retval.length;
  }
  catch ( err )
  {
    throw "CPlaylistWriterXSPF::supportedFileExtensions - " + err;
  }
  
  return retval;
};
  
  
CPlaylistWriterXSPF.prototype.QueryInterface = function(iid)
{
    if (!iid.equals(Components.interfaces.nsISupports) &&
        !iid.equals(SONGBIRD_PLAYLISTWRITERXSPF_IID))
        throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
};

/**
 * \class sbPlaylistXSPFModule
 * \brief 
 */
var sbPlaylistWriterXSPFModule = 
{
  registerSelf: function(compMgr, fileSpec, location, type)
  {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compMgr.registerFactoryLocation(SONGBIRD_PLAYLISTWRITERXSPF_CID, 
                                    SONGBIRD_PLAYLISTWRITERXSPF_CLASSNAME, 
                                    SONGBIRD_PLAYLISTWRITERXSPF_CONTRACTID, 
                                    fileSpec, 
                                    location,
                                    type);
  },

  getClassObject: function(compMgr, cid, iid) 
  {
    if(!cid.equals(SONGBIRD_PLAYLISTWRITERXSPF_CID))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    if(!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return sbPlaylistWriterXSPFFactory;
  },

  canUnload: function(compMgr)
  { 
    return true; 
  }
}; //sbPlaylistWriterXSPFModule

/**
 * \class sbPlaylistWriterXSPFFactory
 * \brief 
 */
var sbPlaylistWriterXSPFFactory =
{
  createInstance: function(outer, iid)
  {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    if (!iid.equals(SONGBIRD_PLAYLISTWRITERXSPF_IID) &&
        !iid.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_INVALID_ARG;

    return (new CPlaylistWriterXSPF()).QueryInterface(iid);
  }
}; //sbPlaylistXSPFFactory

/**
 * \function NSGetModule
 * \brief 
 */
function NSGetModule(comMgr, fileSpec)
{ 
  return sbPlaylistWriterXSPFModule;
} //NSGetModule
