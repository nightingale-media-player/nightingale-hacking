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

  //--------------------------------------------
  //Functions
  //--------------------------------------------
  this.addToPlaylist = function addToPlaylist( url )
  {
    if( typeof( bsVLCCore.m_Instance ) != 'undefined' )
    {
      bsVLCCore.m_Instance.add_item( url );
      bsVLCCore.m_Instance.next();
    }
  }
  
  this.clearPlaylist = function clearPlaylist()
  {
    if( typeof( bsVLCCore.m_Instance ) != 'undefined')
    {
      bsVLCCore.m_Instance.clear_playlist();
    }
  }
  
  // The instance is bound from an <object> declared outside of this system.
  this.bindInstance = function bindInstance( instance )
  {
    if ( typeof( instance ) != 'undefined' )
    {
      // Assign the variable
      bsVLCCore.m_Instance = instance;
    }
    else
    {
      alert( "why are you passing a blank instance?" );
    }
  }

  // When another core wrapper takes over, stop.
  this.onSwappedOut = function onSwappedOut()
  {
    if ( typeof( bsVLCCore.m_Instance ) != 'undefined' )
    {
      return bsVLCCore.doStop();
    }   
  }
  
  // Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
  this.doPlayImmediate = function doPlayImmediate( url )
  {
    if ( typeof( bsVLCCore.m_Instance ) != 'undefined' )
    { 
      bsVLCCore.m_URL = url;
      bsVLCCore.m_Instance.playmrl( bsVLCCore.m_URL );

      bsVLCCore.m_Paused = false;
      bsVLCCore.m_Playing = true;

      return true;
    }
  }

  // Then we declare our API, which calls whatever is needed to get the job done.
  // This object has to interact with the playlist API to know what files to play.
  this.doPlay      = function doPlay()
  {
    if( !bsVLCCore.isPaused() )
    {
      bsVLCCore.m_Instance.play();
    }
    else
    {
      bsVLCCore.m_Instance.pause();
    }
    
    if( bsVLCCore.m_Instance.isplaying() ) 
    {
      bsVLCCore.m_Paused = false;
      bsVLCCore.m_Playing = true;
    }

    return bsVLCCore.m_Playing;
  }
  
  this.doPause     = function doPause()
  {
    if( bsVLCCore.m_Paused )
    {
      return bsVLCCore.doPlay();
    }
    
    bsVLCCore.m_Instance.pause();

    bsVLCCore.m_Paused = true;
    bsVLCCore.m_Playing = false;
    
    return bsVLCCore.m_Paused;
  }
  
  this.doStop      = function doStop()
  {
    bsVLCCore.m_Paused = false;
    bsVLCCore.m_Playing = false;
    bsVLCCore.m_URL = "";
    
    bsVLCCore.m_Instance.stop();
    bsVLCCore.clearPlaylist();

    return true;
  }
  
  this.doNext      = function doNext()
  {
    return CNullStub();
  }
  
  this.doPrev      = function doPrev()
  {
    return CNullStub();
  }
  
  this.isPlaying   = function isPlaying()
  {
    return bsVLCCore.m_Instance.isplaying() || bsVLCCore.isPaused() || bsVLCCore.m_Playing;
  }
  
  this.isPaused    = function isPaused()
  {
    return bsVLCCore.m_Paused;
  }
  
  this.getLength = function getLength()
  {
    if(!bsVLCCore.m_Playing && !bsVLCCore.m_Paused) return 0;
    return bsVLCCore.m_Instance.get_length() * 1000.0;
  }
  
  this.getPosition = function getPosition()
  {
    if(!bsVLCCore.m_Playing && !bsVLCCore.m_Paused) return 0;
    return bsVLCCore.m_Instance.get_time() * 1000.0;
  }
  
  this.setPosition = function setPosition( pos )
  {
    bsVLCCore.m_Instance.seek( pos / 1000.0 , false );
  }
  
  this.getVolume   = function getVolume()
  {
    return ( bsVLCCore.m_Instance.get_volume() * 255.0 ) / 50.0;
  }
  
  this.setVolume   = function setVolume( vol )
  {
    bsVLCCore.m_Instance.set_volume( ( vol / 255.0 ) * 50.0 );
  }
  
  this.getMute     = function getMute()
  {
    return bsVLCCore.m_Muted;
  }
  
  this.setMute     = function setMute( mute )
  {
    if( mute != bsVLCCore.m_muted )
    {
      bsVLCCore.m_Muted = mute;
      bsVLCCore.m_Instance.mute();
    }
  }
  
  this.getMetadata = function getMetadata( key )
  {
    var retval = "";
    switch ( key )
    {
      case "album":
        retval += bsVLCCore.m_Instance.get_metadata_str( "Meta-information", "Album/movie/show title" );
      break;
      
      case "artist":
        retval += bsVLCCore.m_Instance.get_metadata_str( "Meta-information", "Artist" );
      break;

      case "genre":
        retval += bsVLCCore.m_Instance.get_metadata_str( "Meta-information", "Genre" );
      break;
      
      case "length":
        retval += bsVLCCore.m_Instance.get_metadata_str( "General", "Duration" );
      break;
      
      case "title":
        retval += bsVLCCore.m_Instance.get_metadata_str( "Meta-information", "Title" );
      break;
      
      case "url":
        retval += bsVLCCore.m_Instance.get_metadata_str( "Meta-information", "URL" );
      break;
     
    }
    return retval;
  }
  
  this.onPlaybackEvent = function onPlaybackEvent( key, value )
  {
    return thePlayerRepeater.onPlaybackEvent( key, value );
  }
}

var bsVLCCore = new CVLCCore();
