/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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

// Open functions
//
// This file is not standalone


try
{

  function SBFileOpen( )
  {
    var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                           .getService(Components.interfaces.sbIPlaylistPlayback);
    // Make a filepicker thingie
    var fp = Components.classes["@mozilla.org/filepicker;1"]
            .createInstance(Components.interfaces.nsIFilePicker);

    // get some text for the filepicker window
    var sel = "Select";
    try
    {
      sel = theSongbirdStrings.getString("faceplate.select");
    } catch(e) {}

    // initialize the filepicker with our text, a parent and the mode
    fp.init(window, sel, nsIFilePicker.modeOpen);

    // Tell it what filters to be using
    var mediafiles = "Media Files";
    try
    {
      mediafiles = theSongbirdStrings.getString("open.mediafiles");
    } catch(e) {}

    // ask the playback core for supported extensions
    var files = "";
    var eExtensions = PPS.getSupportedFileExtensions(); 
    while (eExtensions.hasMore()) {
      files += ( "*." + eExtensions.getNext() + "; ");
    }

    // add a filter to show only supported media files
    fp.appendFilter(mediafiles, files);

    // Show the filepicker
    var fp_status = fp.show();
    if ( fp_status == nsIFilePicker.returnOK )
    {
      // And if we're good, play it.
      seen_playing.boolValue = false;
      theTitleText.stringValue = fp.file.leafName;
      theArtistText.stringValue = "";
      theAlbumText.stringValue = "";

      // Use a nsIURI because it is safer and contains the scheme etc...
      var ios = Components.classes["@mozilla.org/network/io-service;1"]
                          .getService(Components.interfaces.nsIIOService);
      var uri = ios.newFileURI(fp.file, null, null);
      PPS.playAndImportURL(uri.spec);

      // put the url into the search widget history???
      if (document.__SEARCHWIDGET__) document.__SEARCHWIDGET__.loadSearchStringForCurrentUrl();
    }
  }

  function SBUrlOpen( )
  {
    // Make a magic data object to get passed to the dialog
    var url_open_data = new Object();
    url_open_data.URL = URL.stringValue;
    url_open_data.retval = "";
    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/open_url.xul", "open_url", "chrome,modal=yes,centerscreen", url_open_data );
    if ( url_open_data.retval == "ok" )
    {
      // And if we're good, play it.
      theTitleText.stringValue = url_open_data.URL;
      theArtistText.stringValue = "";
      theAlbumText.stringValue = "";
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.playAndImportURL(url_open_data.URL);
      if (document.__SEARCHWIDGET__) document.__SEARCHWIDGET__.loadSearchStringForCurrentUrl();
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
      var sel = "Open Playlist";
      try
      {
        sel = theSongbirdStrings.getString("faceplate.open.playlist");
      } catch(e) {}
      fp.init(window, sel, nsIFilePicker.modeOpen);
      
      // Tell it what filters to be using
      var filterlist = "";
      var extensionCount = new Object;
      var extensions = thePlaylistReader.supportedFileExtensions(extensionCount);
      
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
      
      var playlistfiles = "Playlist Files";
      try
      {
        playlistfiles = theSongbirdStrings.getString("open.playlistfiles");
      } catch(e) {}
      fp.appendFilter(playlistfiles, filterlist);
      
      // Show it
      var fp_status = fp.show();
      if ( fp_status == nsIFilePicker.returnOK )
      {
        SBScanServiceTreeNewEntryEditable(); // Do this right before you add to the servicelist?

        // And if we're good, play it.
        var plsFile = "file:///" + fp.file.path;
        var readableName = fp.file.leafName;
        var success = thePlaylistReader.autoLoad(fp.fileURL.spec, "songbird", readableName, "user", plsFile, "", null);
        
        if ( ( success == true ) || ( success == 1 ) )
        {
          SBScanServiceTreeNewEntryStart(); // Do this right after you know you have added to the servicelist?  
        }

        if (theLibraryPlaylist) theLibraryPlaylist.syncPlaylistIndex(true);
      }
    }
    catch(err)
    {
      alert(err);
    }
  }

  function log(str)
  {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage( str );
  }

  function SBUrlExistsInDatabase( the_url )
  {
    var retval = false;
    try
    {
      aDBQuery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"];
      if (aDBQuery)
      {
        aDBQuery = aDBQuery.createInstance();
        aDBQuery = aDBQuery.QueryInterface(Components.interfaces.sbIDatabaseQuery);
        
        if ( ! aDBQuery )
        {
          return false;
        }
        
        aDBQuery.setAsyncQuery(false);
        aDBQuery.setDatabaseGUID("testdb-0000");
        aDBQuery.addQuery('select * from test where url="' + the_url + '"' );
        var ret = aDBQuery.execute();
        
        resultset = aDBQuery.getResultObject();
       
        // we didn't find anything that matches our url
        if ( resultset.getRowCount() != 0 )
        {
          retval = true;
        }
      }
    }
    catch(err)
    {
      alert(err);
    }
    return retval;
  }


  function SBPlayPlaylistIndex( index, playlist )
  {
    try
    {
      if (!playlist) playlist = document.__CURRENTPLAYLIST__;
      if (!playlist) playlist = document.__CURRENTWEBPLAYLIST__;
      if (!playlist) return;
      
      var PPS = Components.classes["@songbirdnest.com/Songbird/PlaylistPlayback;1"].getService(Components.interfaces.sbIPlaylistPlayback);
      PPS.playRef(playlist.ref, index);
    }
    catch ( err )
    {
      alert( err );
    }
  }
}
catch (e)
{
  alert(e);
}




