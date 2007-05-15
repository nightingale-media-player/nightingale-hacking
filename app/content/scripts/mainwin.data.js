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
//  This file binds UI objects from the mainwin.xul
//  to data remotes that will automatically update them to reflect the proper value.
//

try
{
  function onSBMainwinDataLoad()
  {
    //SB_LOG("onSBMainwinDataLoad", "");
    try
    {
      // Do the magic binding stuff here.
      //
      // SBDataBindElementProperty Param List
      //  1 - The data ID to bind upon
      //  2 - The element ID to be bound
      //  3 - The name of the property or attribute to change
      //  4 - Optionally assign the data as BOOLEAN data (true/false props, "true"/"false" attrs)
      //  5 - Optionally assign the data as a boolean NOT of the value
      //  6 - Optionally apply an eval string where `value = eval( "eval_string" );`

      // Playlist Items
      MainwinAdd( SBDataBindElementAttribute( "playlist.shuffle", "control.shuf", "checked", true ) );

      MainwinAdd( SBDataBindElementAttribute( "playlist.repeat", "control.repx", "checked", true, true, "parseInt( value ) != 0" ) );
      MainwinAdd( SBDataBindElementAttribute( "playlist.repeat", "control.rep1", "checked", true, true, "parseInt( value ) != 1" ) );
      MainwinAdd( SBDataBindElementAttribute( "playlist.repeat", "control.repa", "checked", true, true, "parseInt( value ) != 2" ) );      
     
      // Faceplate Items    
      
      MainwinAdd( SBDataBindElementAttribute( "faceplate.loading", "spinner_stopped", "hidden", true ) );
      MainwinAdd( SBDataBindElementAttribute( "faceplate.loading", "spinner_spin", "hidden", true, true ) );
      
//      MainwinAdd( SBDataBindElementProperty ( "faceplate.loading", "status_progress", "mode", false, false, "if ( value == '1' ) value = 'undetermined'; else value = ''; value;" ) ); // <sb-status-progressmeter/>
      
      // Browser Items
      MainwinAdd( SBDataBindElementAttribute( "browser.cangoback", "browser_back", "disabled", true, true ) );
      MainwinAdd( SBDataBindElementAttribute( "browser.cangofwd", "browser_fwd", "disabled", true, true ) );
      MainwinAdd( SBDataBindElementAttribute( "faceplate.loading", "browser_stop", "disabled", true, true ) );
        
      MainwinAdd( SBDataBindElementAttribute( "jumpto.visible", "control.jumpto", "checked", true ) );
      
      
      // Options
//      MainwinAdd( SBDataBindElementAttribute( "option.htmlbar", "file.htmlbar", "checked", true ) );


      
//      SBInitPlayerLoop();
    }
    catch ( err )
    {
      alert( err );
    }
  }
      
  // The bind function returns a remote, 
  // we keep it here so there's always a ref
  // and we stay bound till we unload.
  var MainwinDataRemotes = new Array();
  function MainwinAdd( remote )
  {
    if ( remote )
    {
      MainwinDataRemotes.push( remote );
    }
  }

  function onSBMainwinDataUnload()
  {
    //SB_LOG("onSBMainwinDataUnload", "");
    try
    {
      // Unbind the observers!
      for ( var i = 0; i < MainwinDataRemotes.length; i++ )
      {
        MainwinDataRemotes[ i ].unbind();
        MainwinDataRemotes[ i ] = null;
      }
    }  
    catch ( err )
    {
      alert( err );
    }
  }
  

  // END
}
catch ( err )
{
  alert( err );
}




