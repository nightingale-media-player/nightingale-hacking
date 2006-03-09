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
// QT Wrapper
//

// The QT ActiveX Control Wrapper
function CQTCore()
{
  //--------------------------------------------
  //Variables
  //--------------------------------------------
  this.m_Playing = false;
  this.m_Paused  = false;
  
  //--------------------------------------------
  //Functions
  //--------------------------------------------
  // The instance is bound from an <object> declared outside of this system.
  this.bindInstance = function bindInstance( instance )
  {
    if ( typeof( instance ) != 'undefined' )
    {
      // Assign the variable
      theQTCore.m_Instance = instance;
      
      // Set the starting conditions
      
      var text = "";
      for ( var i in instance )
      {
        text += "core_qt." + i + " = " + instance[ i ] + "\t";
      }

      theQTCore.m_Instance.SetAutoPlay( false );
    }
    else
    {
      alert( "why are you passing a blank instance?" );
    }
  }

  // When another core wrapper takes over, stop.
  this.onSwappedOut = function onSwappedOut()
  {
    if ( typeof( theQTCore.m_Instance ) != 'undefined' )
    {
      theQTCore.m_Instance.Stop();
      theQTCore.m_Instance.Rewind();
    }   
  } 
  
  // Set the url and tell it to just play it.  Eventually this talks to the media library object to make a temp playlist.
  this.doPlayImmediate = function doPlayImmediate( url )
  {
    var myUrl = "file:///" + url; 
    
    if ( typeof( theQTCore.m_Instance ) != 'undefined' )
    { 
      // Set it and go! who knows what will happen?
      theQTCore.m_Instance.SetURL(myUrl);
      
      setTimeout('theQTCore.doPlay()', 250);
    }
  }
  // Here's the loop that runs every 250 ms until our playing variable is cleared.

  // Then we declare our API, which calls whatever is needed to get the job done.
  // This object has to interact with the playlist API to know what files to play.
  this.doPlay      = function doPlay()
  {
    theQTCore.m_Paused = false;
    theQTCore.m_Playing = true;
    
    return theQTCore.Play();
  }
  
  this.doPause     = function doPause()
  {
    if ( theQTCore.isPaused() )
    {
      return theQTCore.doPlay();
    }
    
    theQTCore.m_Paused = true;
    theQTCore.m_Playing = false;

    return theQTCore.Stop();
  }
  
  this.doStop      = function doStop()
  {
    theQTCore.m_Paused = false;
    theQTCore.m_Playing = false;
    
    theQTCore.m_Instance.Stop();
    return theQTCore.m_Instance.Rewind();
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
    return theQTCore.m_Playing;
  }
  
  this.isPaused    = function isPaused()
  {
    return theQTCore.m_Paused;
  }
  
  this.getLength = function getLength()
  {
    return theQTCore.m_Instance.GetEndTime();
  }
  
  this.getPosition = function getPosition()
  {
    return theQTCore.m_Instance.GetTime();
  }
  
  this.setPosition = function setPosition( pos )
  {
    theQTCore.m_Instance.SetTime( pos );
    theQTCore.doPlay();
  }
  
  this.getVolume   = function getVolume()
  {
    return ( theQTCore.m_Instance.GetVolume() );
  }
  
  this.setVolume   = function setVolume( vol )
  {
    theQTCore.m_Instance.SetVolume( vol );
    theQTCore.onPlaybackEvent( "volume", theQTCore.getVolume() );
  }
  
  this.getMute     = function getMute()
  {
    return theQTCore.m_Instance.GetMute();
  }
  
  this.setMute     = function setMute( mute )
  {
    theQTCore.m_Instance.SetMute( mute );
    theQTCore.onPlaybackEvent( "mute", theQTCore.getMute() );
  }

  this.onPlaybackEvent = function onPlaybackEvent( key, value )
  {
    if ( key == "status" )
    {
      // Do interesting things based on the core status changing
      if ( value == UNDEFINED )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == STOPPED )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == PAUSED )
      {
        theQTCore.m_Paused = true;
      }
      else if ( value == PLAYING )
      {
        theQTCore.m_Playing = true;
        theQTCore.m_Paused = false;
      }
      else if ( value == SCANFORWARD )
      {
        theQTCore.m_Playing = true;
        theQTCore.m_Paused = false;
      }
      else if ( value == SCANREVERSE )
      {
        theQTCore.m_Playing = true;
        theQTCore.m_Paused = false;
      }
      else if ( value == BUFFERING )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == WAITING )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == ENDED )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == TRANSITIONING )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == READY )
      {
        theQTCore.m_Playing = false;
        theQTCore.m_Paused = false;
      }
      else if ( value == RECONNECTING )
      {
      }
    
      // Determine the status string.
      var new_value = "Undefined - Windows Media Player is in an undefined state.";
      if ( value >= 0 && value <= 11 )
      {
        new_value = theQTPlayerStateArray[ value ];
      }
      new_value + " | " + theQTCore.m_Instance.status;
      thePlayerRepeater.onPlaybackEvent( "status_text", new_value )      
    }
  
    return thePlayerRepeater.onPlaybackEvent( key, value );
  }  
}

// Cheezy-ass global variables!  Yee-haw!
var theQTCore = new CQTCore();

//-----------------------------------------------
//-----------------------------------------------
var theQTPlayerStateArray = new Array(12);
theQTPlayerStateArray[0] = "Undefined - Windows Media Player is in an undefined state.";
theQTPlayerStateArray[1] = "Stopped - Playback of the current media clip is stopped."; 
theQTPlayerStateArray[2] = "Paused - Playback of the current media clip is paused. When media is paused, resuming playback begins from the same location.";
theQTPlayerStateArray[3] = "Playing - The current media clip is playing."; 
theQTPlayerStateArray[4] = "ScanForward - The current media clip is fast forwarding.";
theQTPlayerStateArray[5] = "ScanReverse - The current media clip is fast rewinding."; 
theQTPlayerStateArray[6] = "Buffering - The current media clip is getting additional data from the server.";
theQTPlayerStateArray[7] = "Waiting - Connection is established, however the server is not sending bits. Waiting for session to begin.";
theQTPlayerStateArray[8] = "MediaEnded - Media has completed playback and is at its end.";  
theQTPlayerStateArray[9] = "Transitioning - Preparing new media."; 
theQTPlayerStateArray[10] = "Ready - Ready to begin playing."; 
theQTPlayerStateArray[11] = "Reconnecting - Reconnecting to stream.";
