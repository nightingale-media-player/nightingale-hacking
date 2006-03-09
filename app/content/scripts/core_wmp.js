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
// WMP Wrapper
//

// The WMP ActiveX Control Wrapper
function CWMPCore()
{
  // "Declare" (or whatever) some state variables.
  this.m_PosLoopDuration = 250;
  
  // The instance is bound from an <object> declared outside of this system.
  this.bindInstance = function bindInstance( instance )
  {
    if ( typeof( instance ) != 'undefined' )
    {
      // Assign the variable
      theWMPCore.m_Instance = instance;

      // Set the starting conditions
      theWMPCore.m_Instance.uiMode = "none";
      theWMPCore.m_Instance.stretchToFit = true;
      theWMPCore.m_Instance.settings.autoStart = false;
    }
    else
    {
      alert( "why are you passing a blank instance?" );
    }
  }

  // When another core wrapper takes over, stop.
  this.onSwappedOut = function onSwappedOut()
  {
    if ( typeof( theWMPCore.m_Instance ) != 'undefined' )
    {
      theWMPCore.m_Instance.controls.stop(); 
    }   
  }
  
  // Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
  this.doPlayImmediate = function doPlayImmediate( url )
  {
    if ( typeof( theWMPCore.m_Instance ) != 'undefined' )
    { 
      // Set it and go! who knows what will happen?
      theWMPCore.m_Instance.controls.stop();
      theWMPCore.m_Instance.URL = url;
      theWMPCore.m_Instance.controls.play();
    }
  }
  // Here's the loop that runs every 250 ms until our playing variable is cleared.

  // Then we declare our API, which calls whatever is needed to get the job done.
  // This object has to interact with the playlist API to know what files to play.
  this.doPlay      = function doPlay()
  {
    return theWMPCore.m_Instance.controls.play();
  }
  
  this.doPause     = function doPause()
  {
    if ( theWMPCore.isPaused() )
    {
      return theWMPCore.m_Instance.controls.play();
    }
    return theWMPCore.m_Instance.controls.pause();
  }
  
  this.doStop      = function doStop()
  {
    return theWMPCore.m_Instance.controls.stop();
  }
  
  this.doNext      = function doNext()
  {
    return alert( "theWMPInstance.doNext()" );
  }
  
  this.doPrev      = function doPrev()
  {
    return alert( "theWMPInstance.doPrev()" );
  }
  
  this.isPlaying   = function isPlaying()
  {
    if ( theWMPCore.m_Instance.playState == 3 )
      return true;
    return false;
  }
  
  this.isPaused    = function isPaused()
  {
    if ( theWMPCore.m_Instance.playState == 2 )
      return true;
    return false;
  }
  
  this.getLength = function getLength()
  {
    return theWMPCore.m_Instance.currentMedia.duration * 1000.0;
  }
  
  this.getPosition = function getPosition()
  {
    return theWMPCore.m_Instance.controls.currentPosition * 1000.0;
  }
  
  this.setPosition = function setPosition( pos )
  {
    theWMPCore.m_Instance.controls.currentPosition = pos / 1000.0;
  }
  
  this.getVolume   = function getVolume()
  {
    return ( theWMPCore.m_Instance.settings.volume * 255 ) / 100;
  }
  
  this.setVolume   = function setVolume( vol )
  {
    theWMPCore.m_Instance.settings.volume = ( vol / 255 ) * 100;
    theWMPCore.onPlaybackEvent( "volume", theWMPCore.getVolume() );
  }
  
  this.getMute     = function getMute()
  {
    return theWMPCore.m_Instance.settings.mute;
  }
  
  this.setMute     = function setMute( mute )
  {
    theWMPCore.m_Instance.settings.mute = mute;
    theWMPCore.onPlaybackEvent( "mute", theWMPCore.getMute() );
  }

  this.onPlaybackEvent = function onPlaybackEvent( key, value )
  {
    if ( key == "status" )
    {
      // Do interesting things based on the core status changing
      if ( value == UNDEFINED )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == STOPPED )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == PAUSED )
      {
        theWMPCore.m_Paused = true;
      }
      else if ( value == PLAYING )
      {
        theWMPCore.m_Playing = true;
        theWMPCore.m_Paused = false;
      }
      else if ( value == SCANFORWARD )
      {
        theWMPCore.m_Playing = true;
        theWMPCore.m_Paused = false;
      }
      else if ( value == SCANREVERSE )
      {
        theWMPCore.m_Playing = true;
        theWMPCore.m_Paused = false;
      }
      else if ( value == BUFFERING )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == WAITING )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == ENDED )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == TRANSITIONING )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == READY )
      {
        theWMPCore.m_Playing = false;
        theWMPCore.m_Paused = false;
      }
      else if ( value == RECONNECTING )
      {
      }
    
      // Determine the status string.
      var new_value = "Undefined - Windows Media Player is in an undefined state.";
      if ( value >= 0 && value <= 11 )
      {
        new_value = theWMPPlayerStateArray[ value ];
      }
      new_value + " | " + theWMPCore.m_Instance.status;
      thePlayerRepeater.onPlaybackEvent( "status_text", new_value )      
    }
  
    return thePlayerRepeater.onPlaybackEvent( key, value );
  }  
}

// Cheezy-ass global variables!  Yee-haw!
var theWMPCore = new CWMPCore();

//-----------------------------------------------
// v7+ onPlayStateChange state options array (just temporary till we setup an agnostic set of values).
//-----------------------------------------------
var theWMPPlayerStateArray = new Array(12);
theWMPPlayerStateArray[0] = "Undefined - Windows Media Player is in an undefined state.";
theWMPPlayerStateArray[1] = "Stopped - Playback of the current media clip is stopped."; 
theWMPPlayerStateArray[2] = "Paused - Playback of the current media clip is paused. When media is paused, resuming playback begins from the same location.";
theWMPPlayerStateArray[3] = "Playing - The current media clip is playing."; 
theWMPPlayerStateArray[4] = "ScanForward - The current media clip is fast forwarding.";
theWMPPlayerStateArray[5] = "ScanReverse - The current media clip is fast rewinding."; 
theWMPPlayerStateArray[6] = "Buffering - The current media clip is getting additional data from the server.";
theWMPPlayerStateArray[7] = "Waiting - Connection is established, however the server is not sending bits. Waiting for session to begin.";
theWMPPlayerStateArray[8] = "MediaEnded - Media has completed playback and is at its end.";  
theWMPPlayerStateArray[9] = "Transitioning - Preparing new media."; 
theWMPPlayerStateArray[10] = "Ready - Ready to begin playing."; 
theWMPPlayerStateArray[11] = "Reconnecting - Reconnecting to stream.";

