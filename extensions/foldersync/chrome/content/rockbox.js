/* rockbox.js
 * This will be loaded into the main window, sync code might call foldersync.
 * rockbox.doSync from main thread to perform a rockbox-backsync.
 */

if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Rockbox database file format brief summary.
 * 
 * First 4 bytes are version id must be 0D 48 43 54 for this code to work.
 * 
 * database_idx.tcd is the master index file
 * header is 24 bytes including 4 byte version id
 * 
 * then each track entry is 84 bytes consisting of 21 x 4 byte int
 * int values are little endian so conversion is done here by fgeti
 * 
 * entry 1 is byte offset into database_1.tcd for track album
 * entry 3 is byte offset into database_3.tcd for track title
 * entry 14 is track playcount
 * entry 15 is track rating
 * 
 * the other database files contain records that are referenced
 * from the byte offsets in the master index file. each record
 * is two 4 byte integers and a null terminated string.
 * 
 * we use album+title to find the track in NG and 
 * then update the play count and rating properties only if 
 * a single record is found. 
 * 
 * the rockbox database play counts are local values that never get set
 * from the id3 tags. hence, we sum the count into NG and clear
 * it in rockbox. 
 * 
 */
var HDR_SIZE = 6;
var TAG_COUNT = 21;
var DB_VERSION = 0x5443480d;
var DB_PREFIX = 8;
var TAG = { "Album":1, "Title":3, "PlayCount":14, "Rating":15, "Flags":20 };
var FLAG = { "Deleted":1 };

foldersync.rockbox = {
  /* Do a Rockbox back-sync
   * path (string): the sync target path
   */
  doSync:function(path){
    foldersync.central.logEvent("Rockbox", "Sync started.", 4);
    
    // get the main NG library list
    var Cc = Components.classes;  
    var Ci = Components.interfaces;
    var songlib = LibraryUtils.mainLibrary;
    foldersync.central.logEvent("Rockbox", "mainLibrary: "+songlib, 5);
    var rockpath = this._findpath(path);
    
    // continue only if required rockbox database files exist
    var idxhdr = [];
    var idxdb = this._fopen(rockpath+"/database_idx.tcd");
    if(idxdb == null) {
        foldersync.central.logEvent("Rockbox", "idx database not found.", 1,
                                    "chrome://foldersync/content/rockbox.js");
        return;
    }
    var adb = this._fopen(rockpath+"/database_1.tcd");
    if(adb == null) {
        foldersync.central.logEvent("Rockbox", "album database (1) not found.",
                                    1,
                                    "chrome://foldersync/content/rockbox.js");
        idxdb.fclose();        
        return;
    }
    var tdb = this._fopen(rockpath+"/database_3.tcd");
    if(tdb == null) {
        foldersync.central.logEvent("Rockbox", "title database (3) not found.",
                                    1,
                                    "chrome://foldersync/content/rockbox.js");
        adb.fclose(); idxdb.fclose();        
        return;
    } 
    // read the idx header   
    for(var i = 0; i < HDR_SIZE; i++) 
        idxhdr[i] = this._swapi(idxdb.fgeti());
        
    // make sure database is correct version
    if(idxhdr[0] != DB_VERSION)
        foldersync.central.logEvent("Rockbox", "Incompatible file version.",
                                    1,
                                    "chrome://foldersync/content/rockbox.js");
    else {
        // scan the rockbox database
        while( idxdb.available() >= TAG_COUNT*4 ) {
            var idxtags = [];        
            for(var i = 0; i < TAG_COUNT; i++)
                idxtags[i] = this._swapi(idxdb.fgeti());
            if( (idxtags[TAG.Flags] & FLAG.Deleted) != 0 )
                continue;
            adb.fseek(DB_PREFIX + idxtags[TAG.Album]);
            var album = adb.fgets();
            tdb.fseek(DB_PREFIX + idxtags[TAG.Title]);
            var title = tdb.fgets();

            /* update NG only if track has been played and isn't marked
             * deleted
             */
            if( idxtags[TAG.PlayCount] > 0 ) {
                var cnv = Cc['@mozilla.org/intl/scriptableunicodeconverter'].
                          getService(Ci.nsIScriptableUnicodeConverter);
                cnv.charset = "UTF-8";
                album = cnv.ConvertToUnicode(album); cnv.Finish();
                title = cnv.ConvertToUnicode(title); cnv.Finish();
                foldersync.central.logEvent("Rockbox", album + ":" + title +
                                            " [" + idxtags[TAG.PlayCount] +
                                            "]", 5);
                
                var key = Cc["@getnightingale.com/Nightingale/Properties/" +
                             "MutablePropertyArray;1"].
                             createInstance(Ci.sbIPropertyArray);
                key.appendProperty(SBProperties.albumName, album);
                key.appendProperty(SBProperties.trackName, title);
                try {
                    var item = songlib.getItemsByProperties(key);
                } catch(e) { 
                    foldersync.central.logEvent("Rockbox", "Missing: " +
                                                album + ":" + title + " [" +
                                                idxtags[TAG.PlayCount] + "]",
                                                3, "chrome://foldersync/" +
                                                "content/rockbox.js"); 
                    continue; 
                    }
                // only update if we found a single matching NG item
                if(item.length == 1) {
                    // sum the count into NG count
                    var media = item.enumerate().getNext();
                    media.setProperty(SBProperties.playCount,
                                      Number(media.getProperty(SBProperties.
                                                               playCount)) +
                                      idxtags[TAG.PlayCount]);
                    var pos = idxdb.ftell();
                    idxdb.fseek( pos-( TAG_COUNT - TAG.PlayCount )*4 ); 
                    idxdb.fputi(0); // clear the rockbox count value
                    idxdb.fseek(pos);
                    if(idxtags[TAG.Rating] != 0)
                      // rockbox 0-10 -> NG 0-5
                      media.setProperty(SBProperties.rating,
                                        idxtags[TAG.Rating]/2);
                    }
                }
            }
        }
    tdb.fclose();        
    adb.fclose();
    idxdb.fclose();
    foldersync.central.logEvent("Rockbox", "Sync finished.", 4);
  },
  
  _findpath:function(path) {
    var path = path + "/";
    var chk = Components.classes["@mozilla.org/file/local;1"].
                         createInstance(Components.interfaces.nsILocalFile);
    while(path.length > 0) {
        chk.initWithPath(path+".rockbox");
        foldersync.central.logEvent("Rockbox", "Chk:"+chk.path, 5);
        if( chk.exists() ) 
            return chk.path;
        path = (path.split(/(\/)/).slice(0, -3).join(""));
        }
  return "";
  },
  
  _swapi:function(i) {
    return ( i>>>24 | (i>>>8 & 0xFF00) | (i<<8 & 0xFF0000) | i<<24 );
  },
   
  _fopen:function(filename) {
    try {
        var f = Components.classes["@mozilla.org/file/local;1"].
                           createInstance(Components.interfaces.nsILocalFile);
        f.initWithPath(filename);
        if(!f.exists())
            return null;     

        var is = Components.classes["@mozilla.org/network/" +
                                    "file-input-stream;1"].
                            createInstance(Components.interfaces.
                                                      nsIFileInputStream);
        is.init(f, 0x04, -1, null);
        is.QueryInterface(Components.interfaces.nsISeekableStream);

        var bis = Components.classes["@mozilla.org/binaryinputstream;1"].
                             createInstance(Components.interfaces.
                                                       nsIBinaryInputStream);
        bis.setInputStream(is);
        
        var os = Components.classes["@mozilla.org/network/" +
                                    "file-output-stream;1"].
                            createInstance(Components.interfaces.
                                                      nsIFileOutputStream);
        os.init(f, 0x04, -1, null);
        os.QueryInterface(Components.interfaces.nsISeekableStream);
        
        var bos = Components.classes["@mozilla.org/binaryoutputstream;1"].
                             createInstance(Components.interfaces.
                                                       nsIBinaryOutputStream);
        bos.setOutputStream(os);
    } catch (e) {
      foldersync.central.logEvent("Rockbox", "File exception:\n\n" + e, 1,
                                  "chrome://foldersync/content/rockbox.js",
                                  e.lineNumber);
    }
   
    return {
      "is": is, "os": os,
      "bis": bis, "bos": bos,
      "available": function() { return this.is.available(); },
      "fclose": function() { this.bis.close(); this.is.close(); this.bos.close(); this.os.close(); },
      "ftell": function() { return this.is.tell(); },
      "fseek": function(pos) { this.is.seek(0,pos); this.os.seek(0,pos); },
      "fgetc": function() { return this.bis.read8(); },
      "fgeti": function() { return this.bis.read32(); },
      "fgets": function() { rs=""; while((rc=this.bis.read8())!=0){rs=rs+String.fromCharCode(rc);} return rs; },
      "fputi": function(i) { this.bos.write32(i); },
    };
  }
}

