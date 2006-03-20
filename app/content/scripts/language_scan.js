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
  // Don't turn this on unless you change the locale folder to your own hd.
  var ENABLE_LANGUAGESCAN = 1;
  
  var lsLocaleFolder = "c:\\projects\\svn\\locales";
  
  var lsDTDFileName = "rmp_demo.dtd";
  var lsDTDMasterFile = lsLocaleFolder + "\\en-US\\" + lsDTDFileName;

  var lsPropFileName = "songbird.properties";
  var lsPropMasterFile = lsLocaleFolder + "\\en-US\\" + lsPropFileName;
  
  var language_scan_text = ""; // Output.

  function LSRunScan()
  {
  try
  {
    alert( "scanning property and dtd files for missing entries" );
    
    // Scan the master file to know what to compare against.
    var lsDTDMasterArray = LSScanDTD( lsDTDMasterFile );
    var lsPropMasterArray = LSScanProp( lsPropMasterFile );

    // Go through the locale folder
    var aLocaleFolder = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    aLocaleFolder.initWithPath( lsLocaleFolder );
    var subfolders = aLocaleFolder.directoryEntries;
    while ( subfolders.hasMoreElements() )
    {
      var folder = subfolders.getNext().QueryInterface( Components.interfaces.nsILocalFile );      
      if ( folder.isDirectory() && folder.leafName.indexOf( '-' ) != -1 )
      {
        var files = folder.directoryEntries;
        while ( files.hasMoreElements() )
        {
          var file = files.getNext().QueryInterface( Components.interfaces.nsILocalFile );
          if ( file.leafName.indexOf( '.dtd' ) != -1 )
          {
            LSFixDTD( lsDTDMasterArray, file );
          }
          if ( file.leafName.indexOf( '.properties' ) != -1 )
          {
            LSFixProp( lsPropMasterArray, file );
          }
        }
      }
    }
    
    alert( language_scan_text );

/*
    language_scan_text += lsDTDMasterArray.length + " items.\n\n";
    for ( var i in lsDTDMasterArray )
    {
      language_scan_text += "key: '" + lsDTDMasterArray[ i ].key + "'   value: '" + lsDTDMasterArray[ i ].value + "'\n";   
    }
    alert( language_scan_text );
*/ 
  }
  catch ( err )   
  {
    alert( "language_scan.js\n\n" + err );
  }
  
  }

  function LSFixDTD( lsDTDMasterArray, file )
  {
    // Scan the file to know what it has.
    var lsLocalArray = LSScanDTD( file.path );
    
    // Then doublescan the arrays to know which ones are missing.
    var lsFixArray = new Array();
    for ( var i in lsDTDMasterArray )
    {
      var found = false;
      for ( var j in lsLocalArray )
      {
        if ( lsLocalArray[ j ].key == lsDTDMasterArray[ i ].key )
        {
          found = true;
          break;
        }
      }
      if ( !found )
      {
        lsFixArray.push( lsDTDMasterArray[ i ] );
      }
    }

    if ( lsFixArray.length )
    {
      language_scan_text += file.path + " needs " + lsFixArray.length + " items.\n\n";
      for ( var i in lsFixArray )
      {
        language_scan_text += "key: '" + lsFixArray[ i ].key + "'   value: '" + lsFixArray[ i ].value + "'\n";   
      }
      language_scan_text += "\nHit OK to append these items to the dtd file.";
/*
      alert( language_scan_text );
*/    
      var aFileWriter = Components.classes["@mozilla.org/network/file-output-stream;1"].createInstance(Components.interfaces.nsIFileOutputStream);      
      aFileWriter.init( file, /* PR_RDWR | PR_APPEND */ 0x04 | 0x10, 0, null );
      var spacer = "\n\n";
      aFileWriter.write( spacer, spacer.length );  
      // Let's append!
      for ( var i in lsFixArray )
      {
        var append = '<!ENTITY ' + lsFixArray[ i ].key + ' "' + lsFixArray[ i ].value + '" >\n';
        aFileWriter.write( append, append.length );
      }
      aFileWriter.write( spacer, spacer.length );
      aFileWriter.close();    
    }
  }

  function LSScanDTD( path )
  {
    var retval = new Array();
    
    // Get the requested file
    var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    aLocalFile.initWithPath( path );
    
    if ( aLocalFile.isFile() )
    {
      // Get a file reader for it
      var aFileReader = (Components.classes["@mozilla.org/network/file-input-stream;1"]).createInstance(Components.interfaces.nsIFileInputStream);
      aFileReader.init( aLocalFile, /* PR_RDONLY */ 1, 0, /*nsIFileInputStream::CLOSE_ON_EOF*/ 0 );
      
      // Change it to a line reader so we can read it via script.
      var aLineReader = aFileReader.QueryInterface( Components.interfaces.nsILineInputStream );
      
      // And loop through it to pull all the key/value pairs
      var out = {};
      while ( aLineReader.readLine( out ) )
      {
        var line = out.value;        
        // Split by quotes to get the value
        var qsplit = line.split( '"' );        
        if ( qsplit.length > 1 )
        {
          // Split by space to get the key
          var ssplit = qsplit[ 0 ].split( ' ' );
          var obj = {};
          obj.value = qsplit[ 1 ];
          obj.key = ssplit[ 1 ];           
          retval.push( obj );
        }
      }
      aFileReader.close();
    }
    
    return retval;
  }

  function LSFixProp( lsPropMasterArray, file )
  {
    // Scan the file to know what it has.
    var lsLocalArray = LSScanProp( file.path );
    
    // Then doublescan the arrays to know which ones are missing.
    var lsFixArray = new Array();
    for ( var i in lsPropMasterArray )
    {
      var found = false;
      for ( var j in lsLocalArray )
      {
        if ( lsLocalArray[ j ].key == lsPropMasterArray[ i ].key )
        {
          found = true;
          break;
        }
      }
      if ( !found )
      {
        lsFixArray.push( lsPropMasterArray[ i ] );
      }
    }

    if ( lsFixArray.length )
    {
      language_scan_text += file.path + " needs " + lsFixArray.length + " items.\n\n";
      for ( var i in lsFixArray )
      {
        language_scan_text += "key: '" + lsFixArray[ i ].key + "'   value: '" + lsFixArray[ i ].value + "'\n";   
      }
      language_scan_text += "\nHit OK to append these items to the Prop file.";
/*
      alert( language_scan_text );
*/    
      var aFileWriter = Components.classes["@mozilla.org/network/file-output-stream;1"].createInstance(Components.interfaces.nsIFileOutputStream);      
      aFileWriter.init( file, /* PR_RDWR | PR_APPEND */ 0x04 | 0x10, 0, null );
      var spacer = "\n\n";
      aFileWriter.write( spacer, spacer.length );  
      // Let's append!
      for ( var i in lsFixArray )
      {
        var append = lsFixArray[ i ].key + '=' + lsFixArray[ i ].value + '\n';
        aFileWriter.write( append, append.length );
      }
      aFileWriter.write( spacer, spacer.length );
      aFileWriter.close();    
    }
  }

  function LSScanProp( path )
  {
    var retval = new Array();
    
    // Get the requested file
    var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    aLocalFile.initWithPath( path );
    
    if ( aLocalFile.isFile() )
    {
      // Get a file reader for it
      var aFileReader = (Components.classes["@mozilla.org/network/file-input-stream;1"]).createInstance(Components.interfaces.nsIFileInputStream);
      aFileReader.init( aLocalFile, /* PR_RDONLY */ 1, 0, /*nsIFileInputStream::CLOSE_ON_EOF*/ 0 );
      
      // Change it to a line reader so we can read it via script.
      var aLineReader = aFileReader.QueryInterface( Components.interfaces.nsILineInputStream );
      
      // And loop through it to pull all the key/value pairs
      var out = {};
      while ( aLineReader.readLine( out ) )
      {
        var line = out.value;        
        // Split by equals to get the key and value
        var qsplit = line.split( '=' );
        var obj = {};        
        obj.value = qsplit[ 1 ];
        obj.key = qsplit[ 0 ];           
        retval.push( obj );
      }
      aFileReader.close();
    }
    
    return retval;
  }

  // If we're supposed to, run it!
  if ( ENABLE_LANGUAGESCAN )
    LSRunScan();

// EOF
}
catch ( err )
{
  alert( "Language Scan\n\n" + err );
}

