/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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

try
{

var thePlayerRepeater = null;

function CNullStub()
{
//  alert( "ERROR: someone called the nullstub." );
}

function onPlaybackEventStub( key, value )
{
  // Doesn't do anything, just a stub.
}

//
// Player Repeater
//

// The player repeater object is a single global thunk layer between the player remotes and the embedded playback cores.
function CPlayerRepeater()
{
  // Hook it up
  var jsLoader = Components.classes["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader);
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/songbird_interfaces.js", null );
  jsLoader.loadSubScript( "chrome://songbird/content/scripts/sbIDataRemote.js", null );

  // First we need our internal API, originally stubbed to NULL
  this.doPlayImmediate_i  = CNullStub;
  this.doPlay_i           = CNullStub;
  this.doPause_i          = CNullStub;
  this.doStop_i           = CNullStub;
  this.doNext_i           = CNullStub;
  this.doPrev_i           = CNullStub;
  this.isPlaying_i        = CNullStub;
  this.isPaused_i         = CNullStub;
  this.getLength_i        = CNullStub;
  this.getPosition_i      = CNullStub;
  this.setPosition_i      = CNullStub;
  this.getVolume_i        = CNullStub;
  this.setVolume_i        = CNullStub;
  this.getMute_i          = CNullStub;
  this.setMute_i          = CNullStub;
  this.getMetadata_i      = CNullStub;
  this.getURLMetadata_i   = CNullStub;
  
  // Then we declare our external API, which encodes the function call through the dataremote.
  this.doPlayImmediate = function doPlayImmediate( url )
  {
    SBDataSetValue( "thePlayerRepeater.doPlayImmediate.url", url );
    SBDataFireEvent( "thePlayerRepeater.doPlayImmediate" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.doPlay          = function doPlay()
  {
    SBDataFireEvent( "thePlayerRepeater.doPlay" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.doPause         = function doPause()
  {
    SBDataFireEvent( "thePlayerRepeater.doPause" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.doStop          = function doStop()
  {
    SBDataFireEvent( "thePlayerRepeater.doStop" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.doNext          = function doNext()
  {
    SBDataFireEvent( "thePlayerRepeater.doNext" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.doPrev          = function doPrev()
  {
    SBDataFireEvent( "thePlayerRepeater.doPrev" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.isPlaying       = function isPlaying()
  {
    SBDataFireEvent( "thePlayerRepeater.isPlaying" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" ) != 0;
  }
  this.isPaused        = function isPaused()
  {
    SBDataFireEvent( "thePlayerRepeater.isPaused" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" ) != 0;
  }
  this.getLength     = function getLength()
  {
    SBDataFireEvent( "thePlayerRepeater.getLength" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.getPosition     = function getPosition()
  {
    SBDataFireEvent( "thePlayerRepeater.getPosition" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.setPosition     = function setPosition( pos )
  {
    SBDataSetValue( "thePlayerRepeater.setPosition.pos", pos );
    SBDataFireEvent( "thePlayerRepeater.setPosition" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.getVolume       = function getVolume()
  {
    SBDataFireEvent( "thePlayerRepeater.getVolume" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.setVolume       = function setVolume( vol )
  {
    SBDataSetValue( "thePlayerRepeater.setVolume.vol", vol );
    SBDataFireEvent( "thePlayerRepeater.setVolume" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.getMute         = function getMute()
  {
    SBDataFireEvent( "thePlayerRepeater.getMute" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" ) != 0;
  }
  this.setMute         = function setMute( mute )
  {
    SBDataSetValue( "thePlayerRepeater.setMute.mute", mute );
    SBDataFireEvent( "thePlayerRepeater.setMute" );
    return SBDataGetIntValue( "thePlayerRepeater.retval" );
  }
  this.getMetadata     = function getMetadata( key )
  {
    SBDataSetValue( "thePlayerRepeater.getMetadata.key", key );
    SBDataFireEvent( "thePlayerRepeater.getMetadata" );
    return SBDataGetValue( "thePlayerRepeater.retval" );
  }
  this.getURLMetadata  = function getURLMetadata( url, keys )
  {
    SBDataSetValue( "thePlayerRepeater.getURLMetadata.url", url );
    SBDataSetValue( "thePlayerRepeater.getURLMetadata.keys.length", keys.length );
    for ( var i = 0; i < keys.length; i++ )
    {
      SBDataSetValue( "thePlayerRepeater.getURLMetadata.keys." + i, keys[ i ] );
    }
    SBDataFireEvent( "thePlayerRepeater.getURLMetadata" );
    var num = SBDataGetIntValue( "thePlayerRepeater.retval.length" );
    var retval = new Array();
    for ( var i = 0; i < num; i++ )
    {
      retval.push( SBDataGetValue( "thePlayerRepeater.retval." + i ) );
    }
    return retval;
  }
  
  
  // Then specify the middle callback API that pumps the calls to the inner API
  this.cb_doPlayImmediate = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    var url = SBDataGetValue( "thePlayerRepeater.doPlayImmediate.url" );
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.doPlayImmediate_i( url ) );
  }
  this.cb_doPlay          = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.doPlay_i() );
  }
  this.cb_doPause         = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.doPause_i() );
  }
  this.cb_doStop          = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.doStop_i() );
  }
  this.cb_doNext          = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.doNext_i() );
  }
  this.cb_doPrev          = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.doPrev_i() );
  }
  this.cb_isPlaying       = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.isPlaying_i() );
  }
  this.cb_isPaused        = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.isPaused_i() );
  }
  this.cb_getLength       = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.getLength_i() );
  }
  this.cb_getPosition     = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.getPosition_i() );
  }
  this.cb_setPosition     = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    var pos = SBDataGetValue( "thePlayerRepeater.setPosition.pos" );
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.setPosition_i( pos ) );
  }
  this.cb_getVolume       = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.getVolume_i() );
  }
  this.cb_setVolume       = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    var vol = SBDataGetValue( "thePlayerRepeater.setVolume.vol" );
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.setVolume_i( vol ) );
  }
  this.cb_getMute         = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.getMute_i() );
  }
  this.cb_setMute         = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    var mute = SBDataGetValue( "thePlayerRepeater.setMute.mute" );
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.setMute_i( mute ) );
  }
  this.cb_getMetadata     = function()
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    var key = SBDataGetValue( "thePlayerRepeater.getMetadata.key" );
    return SBDataSetValue( "thePlayerRepeater.retval", thePlayerRepeater.getMetadata_i( key ) );
  }
  this.cb_getURLMetadata  = function( url, keys )
  {
    if ( !thePlayerRepeater || thePlayerRepeater.binding ) return;
    var url = SBDataGetValue( "thePlayerRepeater.getURLMetadata.url" );
    var num = SBDataGetValue( "thePlayerRepeater.getURLMetadata.keys.length" );
    var keys = new Array();
    for ( var i = 0; i < num; i++ )
    {
      keys.push( SBDataGetValue( "thePlayerRepeater.getURLMetadata.keys." + i ) );
    }
    var values = thePlayerRepeater.getURLMetadata_i( url, keys );
    SBDataSetValue( "thePlayerRepeater.retval.length", values.length );
    for ( var i = 0; i < values.length; i++ )
    {
      SBDataSetValue( "thePlayerRepeater.retval." + i, values[ i ] );
    }
  }
  
  // Then bind the callback API into a set of dataremotes
  this.remotes = new Array();
  this.bind = function( func, key )
  {
    var remote = new sbIDataRemote( key );
    remote.BindCallbackFunction( func );
    this.remotes.push( remote );
  }
  this.unbind = function()
  {
    for ( var i in this.remotes )
    {
      this.remotes[ i ].Unbind();
    }
  }

  // setPlaybackCore maps the function refs from the passed core into the internal API refs
  this.setPlaybackCore = function setPlaybackCore( core )
  {
    // Inform the current core that it is going away.
    if ( typeof( thePlayerRepeater.m_CurrentCore ) != 'undefined' )
    {
      this.m_CurrentCore.onSwappedOut();
    }
    // Copy this core in.
    this.m_CurrentCore     = core;
    this.doPlayImmediate_i = core.doPlayImmediate;
    this.doPlay_i          = core.doPlay;
    this.doPause_i         = core.doPause;
    this.doStop_i          = core.doStop;
    this.doNext_i          = core.doNext;
    this.doPrev_i          = core.doPrev;
    this.isPlaying_i       = core.isPlaying;
    this.isPaused_i        = core.isPaused;
    this.getLength_i       = core.getLength;
    this.getPosition_i     = core.getPosition;
    this.setPosition_i     = core.setPosition;
    this.getVolume_i       = core.getVolume;
    this.setVolume_i       = core.setVolume;
    this.getMute_i         = core.getMute;
    this.setMute_i         = core.setMute;
    this.getMetadata_i     = core.getMetadata;
    this.getURLMetadata_i  = core.getURLMetadata;
    
    this.binding = true;
    if ( this.remotes.length == 0 )
    {
      this.bind( this.cb_doPlayImmediate, "thePlayerRepeater.doPlayImmediate" );
      this.bind( this.cb_doPlay, "thePlayerRepeater.doPlay" );
      this.bind( this.cb_doPause, "thePlayerRepeater.doPause" ); 
      this.bind( this.cb_doStop, "thePlayerRepeater.doStop" );
      this.bind( this.cb_doNext, "thePlayerRepeater.doNext" );
      this.bind( this.cb_doPrev, "thePlayerRepeater.doPrev" );
      this.bind( this.cb_isPlaying, "thePlayerRepeater.isPlaying" );
      this.bind( this.cb_isPaused, "thePlayerRepeater.isPaused" );
      this.bind( this.cb_getLength, "thePlayerRepeater.getLength" );
      this.bind( this.cb_getPosition, "thePlayerRepeater.getPosition" );
      this.bind( this.cb_setPosition, "thePlayerRepeater.setPosition" );
      this.bind( this.cb_getVolume, "thePlayerRepeater.getVolume" );
      this.bind( this.cb_setVolume, "thePlayerRepeater.setVolume" );
      this.bind( this.cb_getMute, "thePlayerRepeater.getMute" );
      this.bind( this.cb_setMute, "thePlayerRepeater.setMute" );
      this.bind( this.cb_getMetadata, "thePlayerRepeater.getMetadata" );
      this.bind( this.cb_getURLMetadata, "thePlayerRepeater.getURLMetadata" );
    }
    this.binding = false;
  }
  
  // We need an array of player instances, to route the onPlaybackEvent call.
  this.m_RemotesArray = new Array();
  
  // Here's the functions for putting remotes into and out of the array.
  this.registerRemote = function registerRemote( remote )
  {
    this.m_RemotesArray[ this.m_RemotesArray.length ] = remote;
  }
  
  this.deregisterRemote = function deregisterRemote( remote )
  {
    for ( var i in this.m_RemotesArray )
    {
      // Find it
      if ( this.m_RemotesArray[ i ] == remote )
      {
        // Inform it that it got killed
        this.m_RemotesArray[ i ].onPlaybackEvent( "shutdown", 1 );
        
        // And cut it
        this.m_RemotesArray.splice( i, 1 );
      }
    }
  }
  
  // And, lastly the callback repeater
  this.onPlaybackEvent = function onPlaybackEvent( key, value )
  {
    for ( var i = 0; i < this.m_RemotesArray.length; i++ )
    {
      this.m_RemotesArray[ i ].onPlaybackEvent( key, value );
    }
  }  
}
// Woo, then we instantiate the global instance of the repeater 
// (Don't touch the global instance, tho, just instantiate your own CPlayerRemote)
thePlayerRepeater = new CPlayerRepeater();

//
// Player Remote
//

function CPlayerRemote( onPlaybackEventHandler )
{
  // The user code should specify an event handler.  If not, it gets the stub.  
  // Yay for the magic "OR" operator that does something completely different than you'd expect
  // (if the first operand is NULL, assign the second)
  this.onPlaybackEvent = onPlaybackEventHandler || onPlaybackEventStub;
  
  // All the methods on the remote simply call the methods on the singleton.
  this.doPlayImmediate = thePlayerRepeater.doPlayImmediate;
  this.doPlay          = thePlayerRepeater.doPlay;
  this.doPause         = thePlayerRepeater.doPause;
  this.doStop          = thePlayerRepeater.doStop;
  this.doNext          = thePlayerRepeater.doNext;
  this.doPrev          = thePlayerRepeater.doPrev;
  this.isPlaying       = thePlayerRepeater.isPlaying;
  this.isPaused        = thePlayerRepeater.isPaused;
  this.getLength       = thePlayerRepeater.getLength;
  this.getPosition     = thePlayerRepeater.getPosition;
  this.setPosition     = thePlayerRepeater.setPosition;
  this.getVolume       = thePlayerRepeater.getVolume;
  this.setVolume       = thePlayerRepeater.setVolume;
  this.getMute         = thePlayerRepeater.getMute;
  this.setMute         = thePlayerRepeater.setMute;
  this.getMetadata     = thePlayerRepeater.getMetadata;
  this.getURLMetadata  = thePlayerRepeater.getURLMetadata;
  
  // Make the explicit destruct method (hooray for pissy javascript!)
  this.destruct = function destruct()
  {
    // When we are (explicitly!) destructificated, remove ourselves
    thePlayerRepeater.deregisterRemote( this );
  }
  
  // Lastly register our remote with the repeater
  thePlayerRepeater.registerRemote( this );
  
}

var UNDEFINED = 0;
var STOPPED = 1 
var PAUSED = 2;
var PLAYING = 3 
var SCANFORWARD = 4;
var SCANREVERSE = 5 
var BUFFERING = 6;
var WAITING = 7;
var ENDED = 8 
var TRANSITIONING = 9 
var READY = 10
var RECONNECTING = 11;

}
catch ( err )
{
  var text = "\r\n";
  for ( var i in err )
  {
    text += err[ i ] + "\r\n";
  }
  alert( "player_repeater.js - init - " + err + text );
}