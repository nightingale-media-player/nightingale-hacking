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
// sbIPlaylistWriter Object (XSPF)
//

const SONGBIRD_PLAYLISTWRITERXSPF_CONTRACTID = "@songbirdnest.com/Songbird/Playlist/Writer/XSPF;1";
const SONGBIRD_PLAYLISTWRITERXSPF_CLASSNAME = "Songbird XSPF Playlist Interface";
const SONGBIRD_PLAYLISTWRITERXSPF_CID = Components.ID("{5FD80C2D-FA1B-4d4c-A118-AA571B16E307}");
const SONGBIRD_PLAYLISTWRITERXSPF_IID = Components.interfaces.sbIPlaylistWriter;

function CPlaylistWriterXSPF()
{
  this.m_document = null;
  this.m_playlistmgr = null;
  this.m_playlist = null;
  this.m_library = null;
  this.m_query = null;
};

/* I actually need a constructor in this case. */
CPlaylistWriterXSPF.prototype.constructor = CPlaylistWriterXSPF();

/* the CPlaylistWriterXSPF class def */
CPlaylistWriterXSPF.prototype.write = function(aGUID, aDestTable, aReplace, aErrorCode)
{
  aErrorCode.value = 0;
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
    retval.push( "" );
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
