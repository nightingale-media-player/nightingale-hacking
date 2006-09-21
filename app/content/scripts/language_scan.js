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
 
try 
{
  // Builds from makefiles use an extra "../"
  var USING_MSVC = 1;
  
  // Go find the current directory (the xulrunner directory)
  var lsXulrunnerFolder = Components.classes["@mozilla.org/file/directory_service;1"]
                     .getService(Components.interfaces.nsIProperties)
                     .get("CurProcD", Components.interfaces.nsIFile);
                     
  var lsSlash = "/";
  if ( lsXulrunnerFolder.path.indexOf( "\\" ) != -1 )
    lsSlash = "\\";
  
  var lsLocaleFolder = ""
  if ( USING_MSVC )  
    lsLocaleFolder = lsXulrunnerFolder.path + lsSlash + ".." + lsSlash + ".." + lsSlash + "locales";
  else
    lsLocaleFolder = lsXulrunnerFolder.path + lsSlash + ".." + lsSlash + ".." + lsSlash + ".." + lsSlash + "locales";
  
  var lsMasterFolder = lsLocaleFolder + lsSlash + "en-US" + lsSlash;
  
  var language_scan_text = ""; // Output.
  
  function LSRunScan()
  {
  try
  {
    alert( "scanning property and dtd files for missing entries" );

    // Scan the master folder to know what to compare against.
    var lsMasterFolderFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    lsMasterFolderFile.initWithPath( lsMasterFolder );
    var lsMasterMap = LSScanFolder( lsMasterFolderFile );
    
    // Go through the locale folder
    var localeFolder = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    localeFolder.initWithPath( lsLocaleFolder );
    var subfolders = localeFolder.directoryEntries;
    while ( subfolders.hasMoreElements() )
    {
      // Process folders that have a dash in their name
      var folder = subfolders.getNext().QueryInterface( Components.interfaces.nsILocalFile );      
      if ( folder.isDirectory() && folder.leafName.indexOf( '-' ) != -1 )
      {
        // Scan all the files in the folder
        var localMap = LSScanFolder( folder );
        
        // Then for every file in the master map
        for ( var key in lsMasterMap )
        {
          // See if that file also exists in the local map
          if ( localMap[ key ] )
          {
            // If so, create a path to the file.
            var filePath = folder.path + key;
            // And make a file object.            
            var file = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
            file.initWithPath( filePath );
            
            // And process as a dtd or prop.
            var dtd = key.indexOf( '.dtd' );
            var prop = key.indexOf( '.properties' );
            if ( dtd != -1 && dtd == key.length - 4 )
            {
              LSFixDTD( lsMasterMap[ key ], localMap[ key ], file );
            }
            if ( prop != -1 && prop == key.length - 11 )
            {
              LSFixProp( lsMasterMap[ key ], localMap[ key ], file );
            }
          }
        }
      }
    }
    
    if ( language_scan_text.length > 0 )
      alert( "Language Scan:\n" + language_scan_text );
    else
      alert( "Language Scan:\n" + "No changes found!" );
  }
  catch ( err )   
  {
    alert( "language_scan.js\n\n" + err );
  }
  
  }

  // This function takes a nsILocalFile to a folder and returns a map of filenames against scan arrays
  function LSScanFolder( file, key_root )
  {
    var retval = {};
    // Construct a key for this file.
    var key = ( key_root == null ) ? "" : key_root + lsSlash + file.leafName;
    // If we really are a folder  
    if ( file.isDirectory() )
    {
      var children = file.directoryEntries;
      while ( children.hasMoreElements() )
      {
        // Process all the children
        var child = children.getNext().QueryInterface( Components.interfaces.nsILocalFile );      
        var return_array = LSScanFolder( child, key );
        // Stuff the values into our own map
        for ( var i in return_array )
        {
          retval[ i ] = return_array[ i ];
        }
      }
    }
    // Otherwise, we're a real gosh-darn file.
    else
    {
      // So process as a dtd or prop or nothing.
      var dtd = key.indexOf( '.dtd' );
      var prop = key.indexOf( '.properties' );
      if ( dtd != -1 && dtd == key.length - 4 )
      {
        retval[ key ] = LSScanDTD( file );
      }
      if ( prop != -1 && prop == key.length - 11 )
      {
        retval[ key ] = LSScanProp( file );
      }
    }
    return retval;
  }

  function LSFixDTD( lsDTDMasterArray, lsLocalArray, file )
  {
    // Then map scan the arrays to know which ones are missing.
    var anything = false;
    var lsFixArray = {};
    for ( var i in lsDTDMasterArray )
    {
      if ( typeof( lsLocalArray[ i ] ) == "undefined" )
      {
/*
          alert( "file: " + file.path + "\n" + 
                "\tmaster - " + i + ": " + lsDTDMasterArray[ i ] + "\n" +
                "\tlocal - " + i + ": " + lsLocalArray[ i ] + "\n"  );
*/
        lsFixArray[ i ] = lsDTDMasterArray[ i ];
        anything = true;
      }
    }

    if ( anything )
    {
      language_scan_text += "file: " + file.path + "\n";
      for ( var i in lsFixArray )
      {
        language_scan_text += "\tkey: '" + i + "'   value: '" + lsFixArray[ i ] + "'\n";   
      }
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
        var append = '<!ENTITY ' + i + ' "' + lsFixArray[ i ] + '" >\n';
        aFileWriter.write( append, append.length );
      }
      aFileWriter.write( spacer, spacer.length );
      aFileWriter.close();    
    }
  }
  
  function LSStripMultispace( aStr )
  {
    var split = -1;
    while ( ( split = aStr.indexOf("  ") ) != -1 ) // Search for 2 spaces
    {
      aStr = aStr.substr( 0, split + 1 ) + aStr.substr( split + 2, aStr.length ); // Remove the extra space
    }
    return aStr;  
  };

  function LSScanDTD( aLocalFile )
  {
    var retval = {};
    
    if ( aLocalFile.isFile )
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
        // If it's a valid entity line
        var line = out.value;
        if ( line.indexOf( "ENTITY" ) != -1 )
        {
          // Split by quotes to get the value
          var qsplit = line.split( '"' );
          // Strip all the extra spaces out
          var raw_key = LSStripMultispace( qsplit[ 0 ] );
          if ( qsplit.length > 1 )
          {
            // Split by space to get the key
            var ssplit = raw_key.split( ' ' );
            var key = ssplit[ 1 ];           
            retval[ key ] = qsplit[ 1 ];
            if ( key.indexOf("testing.") != -1 ) alert( aLocalFile.path + ":\n" + key + ": " + retval[ key ] );
          }
        };
      }
      aFileReader.close();
    }
    
    return retval;
  }

  function LSFixProp( lsPropMasterArray, lsLocalArray, file )
  {
    // Then map scan the arrays to know which ones are missing.
    var anything = false;
    var lsFixArray = {};
    for ( var i in lsPropMasterArray )
    {
      if ( typeof( lsLocalArray[ i ] ) == "undefined" )
      {
/*
        alert( "file: " + file.path + "\n" + 
              "\tmaster - " + i + ": " + lsPropMasterArray[ i ] + "\n" +
              "\tlocal - " + i + ": " + lsLocalArray[ i ] + "\n"  );
*/
        lsFixArray[ i ] = lsPropMasterArray[ i ];
        anything = true;
      }
    }

    if ( anything )
    {
      language_scan_text += "file: " + file.path + "\n";
      for ( var i in lsFixArray )
      {
        language_scan_text += "\tkey: '" + i + "'   value: '" + lsFixArray[ i ] + "'\n";   
      }
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
        var append = i + '=' + lsFixArray[ i ] + '\n';
        aFileWriter.write( append, append.length );
      }
      aFileWriter.write( spacer, spacer.length );
      aFileWriter.close();    
    }
  }

  function LSScanProp( aLocalFile )
  {
    var retval = {};
    
    if ( aLocalFile.isFile )
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
        // If it's a valid properties line
        var line = out.value;
        if ( line.length > 0 && line[ 0 ] != "#" && line.indexOf( '=' ) != -1 )
        {
          // Split by equals to get the key and value
          var qsplit = line.split( '=' );
          var key = qsplit[ 0 ];           
          retval[ key ] = qsplit[ 1 ];
        }
      }
      aFileReader.close();
    }
    
    return retval;
  }

  // Run it!
  LSRunScan();

// EOF
}
catch ( err )
{
  alert( "Language Scan\n\n" + err );
}

