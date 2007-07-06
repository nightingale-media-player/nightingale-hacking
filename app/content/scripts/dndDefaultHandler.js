// JScript source code
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


var firstDrop;
var importingDrop = false;


var SBDropObserver = 
{
  canHandleMultipleItems: true,
  
  getSupportedFlavours : function () 
  {
    var flavours = new FlavourSet();
    flavours.appendFlavour("application/x-moz-file","nsIFile");
    flavours.appendFlavour("text/x-moz-url");
    flavours.appendFlavour("text/unicode");
    flavours.appendFlavour("text/plain");
    return flavours;
  },
  
  onDragOver: function ( evt, flavour, session )
  {
  },
  
  onDrop: function ( evt, dropdata, session )
  {
    dropFiles(evt, dropdata, session, null, null);
  }
};

var _drop_filelist = null;

function dropFiles(evt, dropdata, session, targetdb, targetpl) {
  var dataList = dropdata.dataList;
  var dataListLength = dataList.length;
  var lcase = 0;
  
  if (getPlatformString() == "Windows_NT") lcase = 1;
  
  if (!_drop_filelist) _drop_filelist = Array();

  for (var i = 0; i < dataListLength; i++) 
  {
    var item = dataList[i].first;
    var prettyName;
    var rawData = item.data;
    
    if (item.flavour.contentType == "application/x-moz-file")
    {
      var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
      var fileHandler = ioService.getProtocolHandler("file")
                        .QueryInterface(Components.interfaces.nsIFileProtocolHandler);
      rawData = fileHandler.getURLSpecFromFile(rawData);
    } 
    else
    {
      if (rawData.toLowerCase().indexOf("http://") < 0) continue;
    }

    var separator = rawData.indexOf("\n");
    if (separator != -1) 
    {
      prettyName = rawData.substr(separator+1);
      rawData = rawData.substr(0,separator);
    }
    
    if (lcase) rawData = rawData.toLowerCase();
    _drop_filelist.push(rawData);
  }
  
  if (!importingDrop) {
    firstDrop = true;
    importingDrop = true;
    setTimeout( SBDropped, 10 ); // Next frame
    importingDrop = false;
  }
}

function SBDropped() 
{
  var url;
  try {
    if (_drop_filelist.length > 0) {
      url = _drop_filelist[0];
      _drop_filelist.splice(0, 1);
      if (url != "") {
        var ios = Components.classes["@mozilla.org/network/io-service;1"]
                            .getService(Components.interfaces.nsIIOService);
        var uri = ios.newURI(url, null, null);
        theDropPath = uri.spec;
        var fileUrl;
        try {
          fileUrl = uri.QueryInterface(Components.interfaces.nsIFileURL);
        } catch (e) { 
          fileUrl = null; 
        }
        if (fileUrl) {
          theDropIsDir = fileUrl.file.isDirectory();
          if(theDropIsDir) {
            theDropPath = fileUrl.file.path;
          }
        }
        else {
          theDropIsDir = false;
        }
      }
      SBDroppedEntry();
      setTimeout( SBDropped, 10 ); // Next frame
    }
  } catch (e) {
    dump(e);
    setTimeout( SBDropped, 10 ); // Try next frame
  }
}

var theDropPath = "";
var theDropIsDir = false;

function SBDroppedEntry()
{
  if ( theDropIsDir )
  {
    theFileScanIsOpen.boolValue = true;
    
    // otherwise, fire off the media scan page.
    var media_scan_data = new Object();
    media_scan_data.URL = theDropPath;
    media_scan_data.retval = "";

    // Open the modal dialog
    SBOpenModalDialog( "chrome://songbird/content/xul/media_scan.xul", "media_scan", "chrome,centerscreen", media_scan_data ); 
    theFileScanIsOpen.boolValue = false;
  } 
  else if (gPPS.isMediaURL( theDropPath )) {
    
    //Import the track into the main library.
    var item = SBImportURLIntoMainLibrary(theDropPath);
    
    if(firstDrop) {
      firstDrop = false;
      
      var libraryManager = Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
                                  .getService(Components.interfaces.sbILibraryManager);
                                  
      SBDisplayViewForListAndPlayItem(libraryManager.mainLibrary, item);
    }
  }
}
