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

// Debugging Tool
function listProperties(obj, objName) 
{
    var columns = 3;
    var count = 0;
    var result = "";
    for (var i in obj) 
    {
        result += objName + "." + i + " = " + obj[i] + "\t\t\t";
        count = ++count % columns;
        if ( count == columns - 1 )
        {
          result += "\n";
        }
    }
    alert(result);
}

//
// VLC Wrapper
//

// The VLC Mozilla Plugin Wrapper
function CVLCCore()
{
  //--------------------------------------------
  //Variables
  //--------------------------------------------
  this.m_Playing = false;
  this.m_Paused  = false;
  this.m_Muted = false;
  this.m_URL = "";
  this.m_LastPosition = 0;

  //--------------------------------------------
  //Functions
  //--------------------------------------------
  this.addToPlaylist = function addToPlaylist( url )
  {
  try
  {
    if( typeof( theVLCCore.m_Instance ) != 'undefined' )
    {
      theVLCCore.m_Instance.add_item( url );
      theVLCCore.m_Instance.next();
    }
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.clearPlaylist = function clearPlaylist()
  {
  try
  {
    if( typeof( theVLCCore.m_Instance ) != 'undefined')
    {
      theVLCCore.m_Instance.clear_playlist();
    }
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  // The instance is bound from an <object> declared outside of this system.
  this.bindInstance = function bindInstance( instance )
  {
  try
  {
    if ( typeof( instance ) != 'undefined' )
    {
      // Assign the variable
      theVLCCore.m_Instance = instance;
    }
    else
    {
      alert( "why are you passing a blank instance?" );
    }
  }
  catch ( err )
  {
    alert( err );
  }
  }

  // When another core wrapper takes over, stop.
  this.onSwappedOut = function onSwappedOut()
  {
  try
  {
    if ( typeof( theVLCCore.m_Instance ) != 'undefined' )
    {
      return theVLCCore.doStop();
    }   
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  // Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
  this.doPlayImmediate = function doPlayImmediate( url )
  {
  try
  {
    if ( typeof( theVLCCore.m_Instance ) != 'undefined' )
    { 
      theVLCCore.m_URL = url;
      theVLCCore.m_Instance.playmrl( theVLCCore.m_URL );

      theVLCCore.m_Paused = false;
      theVLCCore.m_Playing = true;
      this.m_LastPosition = 0;

      return true;
    }
  }
  catch ( err )
  {
    alert( err );
  }
  }

  // Then we declare our API, which calls whatever is needed to get the job done.
  // This object has to interact with the playlist API to know what files to play.
  this.doPlay      = function doPlay()
  {
  try
  {
    if( !theVLCCore.isPaused() )
    {
      theVLCCore.m_Instance.play();
      this.m_LastPosition = 0;
    }
    else
    {
      theVLCCore.m_Instance.pause();
    }
    
    if( theVLCCore.m_Instance.isplaying() ) 
    {
      theVLCCore.m_Paused = false;
      theVLCCore.m_Playing = true;
    }

    return theVLCCore.m_Playing;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.doPause     = function doPause()
  {
  try
  {
    if( theVLCCore.m_Paused )
    {
      return theVLCCore.doPlay();
    }
    
    theVLCCore.m_Instance.pause();

    theVLCCore.m_Paused = true;
    theVLCCore.m_Playing = false;
    
    return theVLCCore.m_Paused;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.doStop      = function doStop()
  {
  try
  {
    theVLCCore.m_Paused = false;
    theVLCCore.m_Playing = false;
    theVLCCore.m_URL = "";
    
    theVLCCore.m_Instance.stop();
    theVLCCore.clearPlaylist();

    return true;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.doNext      = function doNext()
  {
  try
  {
    return CNullStub();
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.doPrev      = function doPrev()
  {
  try
  {
    return CNullStub();
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.isPlaying   = function isPlaying()
  {
  try
  {
    return theVLCCore.m_Instance.isplaying() || theVLCCore.isPaused() || theVLCCore.m_Playing;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.isPaused    = function isPaused()
  {
  try
  {
    return theVLCCore.m_Paused;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.getLength = function getLength()
  {
  try
  {
    if(!theVLCCore.m_Playing && !theVLCCore.m_Paused) return 0;
    return theVLCCore.m_Instance.get_length() * 1000.0;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.getPosition = function getPosition()
  {
  try
  {
    if(!theVLCCore.m_Playing && !theVLCCore.m_Paused) return 0;
    
    // Lame hack to stop it when it plays the next track
    var time = theVLCCore.m_Instance.get_time();
    if ( time < this.m_LastPosition )
    {
      theVLCCore.m_Instance.stop();
    }
    this.m_LastPosition = time;
    
    return time * 1000.0;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.setPosition = function setPosition( pos )
  {
  try
  {
    this.m_LastPosition = 0;
    theVLCCore.m_Instance.seek( pos / 1000.0 , false );
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.getVolume   = function getVolume()
  {
  try
  {
    return ( theVLCCore.m_Instance.get_volume() * 255.0 ) / 50.0;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.setVolume   = function setVolume( vol )
  {
  try
  {
    theVLCCore.m_Instance.set_volume( ( vol / 255.0 ) * 50.0 );
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.getMute     = function getMute()
  {
  try
  {
    return theVLCCore.m_Muted;
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.setMute     = function setMute( mute )
  {
  try
  {
    if( mute != theVLCCore.m_muted )
    {
      theVLCCore.m_Muted = mute;
      theVLCCore.m_Instance.mute();
    }
  }
  catch ( err )
  {
    alert( err );
  }
  }
  
  this.getMetadata = function getMetadata( key )
  {
  try
  {
    var retval = "";
    switch ( key )
    {
      case "album":
        retval += theVLCCore.m_Instance.get_metadata_str( "Meta-information", "Album/movie/show title" );
      break;
      
      case "artist":
        retval += theVLCCore.m_Instance.get_metadata_str( "Meta-information", "Artist" );
      break;

      case "genre":
        retval += theVLCCore.m_Instance.get_metadata_str( "Meta-information", "Genre" );
      break;
      
      case "length":
        retval += theVLCCore.m_Instance.get_metadata_str( "General", "Duration" );
      break;
      
      case "title":
        retval += theVLCCore.m_Instance.get_metadata_str( "Meta-information", "Title" );
      break;
      
      case "url":
        retval += theVLCCore.m_Instance.get_metadata_str( "Meta-information", "URL" );
      break;
     
    }
    return retval;
  }
  catch ( err )
  {
    alert( err );
  }
  }
 
  this.getURLMetadata = function getURLMetadata( url, keys )
  {
    retval = new Array();
    try
    {
      var theCats = new Array;
      var theKeys = new Array;
      var theOutSize = new Object;
      
      for(var k = 0; k < keys.length; k++)
      {
        switch(keys[k])
        {
          case "album": theCats.push("Meta-information"); theKeys.push("Album/movie/show title"); break;
          case "artist": theCats.push("Meta-information"); theKeys.push("Artist"); break;
          case "genre": theCats.push("Meta-information"); theKeys.push("Genre"); break;
          case "length": theCats.push("General"); theKeys.push("Duration"); break;
          case "title": theCats.push("Meta-information"); theKeys.push("Title"); break;
          case "url": theCats.push("Meta-information"); theKeys.push("URL"); break;
        }
      }
      retval = theVLCCore.m_Instance.get_url_metadata( url, theCats.length, theCats, theKeys.length, theKeys, theOutSize );
    }
    catch ( err )
    {
      alert( err );
    }
    return retval;
  }
  
  this.onPlaybackEvent = function onPlaybackEvent( key, value )
  {
  try
  {
    return thePlayerRepeater.onPlaybackEvent( key, value );
  }
  catch ( err )
  {
    alert( err );
  }
  }
}

var theVLCCore = new CVLCCore();
