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

// Open functions
//
// This file is not standalone


try
{

  function SBFileOpen( )
  {
    // Make a filepicker thingie
    var nsIFilePicker = Components.interfaces.nsIFilePicker;
    var fp = Components.classes["@mozilla.org/filepicker;1"]
            .createInstance(nsIFilePicker);
    var sel = "Select";
    try
    {
      sel = theSongbirdStrings.getString("faceplate.select");
    } catch(e) {}
    fp.init(window, sel, nsIFilePicker.modeOpen);
    // Tell it what filters to be using
    fp.appendFilter("Media Files","*.wav; *.ogg; *.flac; *.m4a; *.mp3; *.mp4; *.wma; *.wmv; *.avi; *.asf; *.asx; *.mov;");
    // Show it
    var fp_status = fp.show();
    if ( fp_status == nsIFilePicker.returnOK )
    {
      // And if we're good, play it.
      seen_playing.SetValue(false);
      URL.SetValue(fp.file.path);
      
      theTitleText.SetValue( fp.file.leafName );
      theArtistText.SetValue( "" );
      theAlbumText.SetValue( "" );
      playCurrentUrl( false );
    }
  }

  function SBUrlOpen( )
  {
    // Make a magic data object to get passed to the dialog
    var url_open_data = new Object();
    url_open_data.URL = URL.GetValue();
    url_open_data.retval = "";
    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/open_url.xul", "open_url", "chrome,modal=yes,centerscreen", url_open_data );
    if ( url_open_data.retval == "ok" )
    {
      // And if we're good, play it.
      URL.SetValue(url_open_data.URL);
      theTitleText.SetValue( url_open_data.URL );
      theArtistText.SetValue( "" );
      theAlbumText.SetValue( "" );
      playCurrentUrl( true );
    }  
  }

  function SBPlaylistOpen()
  {
    try
    {
      // Make a filepicker thingie
      var nsIFilePicker = Components.interfaces.nsIFilePicker;
      var fp = Components.classes["@mozilla.org/filepicker;1"]
              .createInstance(nsIFilePicker);
      var sel = "Select";
      try
      {
        sel = theSongbirdStrings.getString("faceplate.select");
      } catch(e) {}
      fp.init(window, sel, nsIFilePicker.modeOpen);
      
      // Tell it what filters to be using
      var filterlist = "";
      var extensionCount = new Object;
      var extensions = thePlaylistReader.SupportedFileExtensions(extensionCount);
      
      var first = true;
      for(var i = 0; i < extensions.length; i++)
      {
        var ext_list = extensions[i].split(",");
        for(var j = 0; j < ext_list.length; j++)
        {
          var ext = ext_list[j];
          if (ext.length > 0) {
            if (!first) // skip the first one
              filterlist += ";";
            first = false;
            filterlist += "*." + ext;
          }
        }
      }
      
      fp.appendFilter("Playlist Files", filterlist);
      
      // Show it
      var fp_status = fp.show();
      if ( fp_status == nsIFilePicker.returnOK )
      {
        SBScanServiceTreeNewEntryEditable(); // Do this right before you add to the servicelist?
        
        // And if we're good, play it.
        var plsFile = "file:///" + fp.file.path;
        var readableName = fp.file.leafName;
        var success = thePlaylistReader.AutoLoad(plsFile, "songbird", readableName, "user", plsFile, "", null);
        
        if ( ( success == true ) || ( success == 1 ) )
        {
          SBScanServiceTreeNewEntryStart(); // Do this right after you know you have added to the servicelist?
        }

        SBSyncPlaylistIndex();
      }
    }
    catch(err)
    {
      alert(err);
    }
  }

}
catch (e)
{
  alert(e);
}

