/* sync.js
 * This will be loaded into the main window, and manage all Sync-related
 * tasks.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Sync controller
 * Controller to manage sync queue and to perform queued syncs. Sends messages
 * to UI listeners, and provides Sync capabilities.
 */
foldersync.sync={
  // The queued SyncQueueObjects (see generateSyncQueueObject)
  _queue: null,
  // The registered listeners (array of function(state,syncqueueobject))
  _listeners: null,
  
  // All threads and thread-related stuff should stay here
  _threads: {
    // The update thread is responsible for calling listeners
    update: {
      // The message that will be sent soon.
      crt: {
        state: null,
        syncqueueobject: null
      },
      
      // The Thread's code
      run: function(){
        try {
          foldersync.central.logEvent("sync-update",
                                      "Update listeners", 5);
          foldersync.sync._notify(this.crt.state, this.crt.syncqueueobject);
          // Reset crt data
          this.crt.state = null;
          this.crt.syncqueueobject = null;
        } catch(e) {
          foldersync.central.logEvent("sync-update",
                                      "Update thread terminated " +
                                      "unexpected:\n\n" + e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
    },
    // The getQueue thread is responsible for getting SyncQueueObjects
    getQueue: {
      // The fetched SyncQueueObject, or null if there is no Object to fetch
      crt: null,
      
      // The Thread's code
      run: function(){
        try {
          foldersync.central.logEvent("sync-getQueue",
                                      "Get element from Queue", 5);
          // Get the next item
          this.crt = null;
          if (foldersync.sync._queue.length)
            this.crt = foldersync.sync._queue.shift();
        } catch(e) {
          foldersync.central.logEvent("sync-getQueue",
                                      "GetQueue thread terminated " +
                                      "unexpected:\n\n" + e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
    },
    // The getProfile thread is responsible for getting Profiles
    getProfile: {
      // The GUID of the Profile to fetch, the fetched Profile, or null
      // The profile has a additional sub-class "defaulttags".
      crt: null,
      
      // The Thread's code
      run: function(){
        try {
          foldersync.central.logEvent("sync-getProfile",
                                      "Get Profile " + this.crt, 5);
          // Get the profile
          this.crt = foldersync.preferences.getProfileByGUID(this.crt);
          // Make sure this.crt is NOT a reference
          // This might be not that nice, but it works. If somebody has a
          // better solution, feel free to contact me (rsjtdrjgfuzkfg).
          this.crt = JSON.parse(JSON.stringify(this.crt));
          
          // Get default tags and append them (include to the profile)
          var defTags = foldersync.preferences.getDefaultTags();
          defTags = JSON.parse(JSON.stringify(defTags));
          this.crt.defaulttags = defTags;
          
          // Get the fallbacks and append them (include to the profile)
          var fallbacks = foldersync.preferences.getFallbacks();
          fallbacks = JSON.parse(JSON.stringify(fallbacks));
          this.crt.fallbacks = fallbacks;
        } catch(e) {
          foldersync.central.logEvent("sync-getProfile",
                                      "GetProfile thread terminated " +
                                      "unexpected:\n\n" + e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
    },
    // The getItems thread is responsible for getting items from playlists
    getItems: {
      /* The Array of playlists we want to fetch, the result or null
       * Result structure: Array of playlists, each:
       * {
       *   name: <string>, // the playlist's name (given by user)
       *   items: [], // all items the playlists has.
       * }
       * a mediaitem has the following structure:
       * {
       *   tags: {}, // the tags, see the code in getProfile.run for details
       *   path: <string>, // the location of the connected file
       * }
       */
      crt: null,
      
      // The Thread's code
      run: function(){
        try {
          foldersync.central.logEvent("sync-getItems",
                                      "Get Items: " + this.crt, 5);
          // Make sure Modules we'll use are loaded
          if (!window.SBProperties) 
            Components.utils.
                       import("resource://app/jsmodules/sbProperties.jsm");
          // Make services avaiable by shorthand
          var ios = Components.classes["@mozilla.org/network/io-service;1"].
                               getService(Components.interfaces.nsIIOService);
          // Prepare result array
          var playlists = this.crt;
          this.crt = [];
          // Go through all playlists
          for each (var guid in playlists){
            var pList = foldersync.central.getPlaylistByGUID(guid);
            foldersync.central.logEvent("sync-getItems",
                                        "Parse playlist " + pList.guid + "\n" +
                                        "Name: " + pList.name, 5);
            // Prepare the playlist's entry
            var plEntry = {name: pList.name, items: []};
            // Go through all items
            for (var i = 0; i < pList.length; i++){
              // Get the item
              var item = pList.getItemByIndex(i);
              // Get the item's path, if any
              var path = null;
              try {
                path = ios.newURI(decodeURI(item.getProperty(
                           SBProperties.contentURL)), null, null).
                           QueryInterface(Components.interfaces.nsIFileURL).
                           file.path;
              } catch (e){}
              // If the item has got a path
              if (path){
                foldersync.central.logEvent("sync-getItems",
                                            "Add item " + item.guid + "\n" +
                                            "Path: " + path, 5);
                // Prepare the item's entry
                var entry = {
                  path: path
                };
                // Add tags we might need to the entry
                entry.tags = {
                  artist: item.getProperty(SBProperties.artistName),
                  title: item.getProperty(SBProperties.trackName),
                  album: item.getProperty(SBProperties.albumName),
                  genre: item.getProperty(SBProperties.genre),
                  rating: item.getProperty(SBProperties.rating),
                  tracknum: item.getProperty(SBProperties.trackNumber),
                  albumartist: item.getProperty(SBProperties.albumArtistName),
                  year: item.getProperty(SBProperties.year),
                  discnumber: item.getProperty(SBProperties.discNumber),
                  disccount: item.getProperty(SBProperties.totalDiscs),
                  producer: item.getProperty(SBProperties.producerName),
                  composer: item.getProperty(SBProperties.composerName),
                  duration: item.getProperty(SBProperties.duration),
                  coverpath: null, // we'll fill that soon, see next lines
                };
                // Get album cover path, if any
                try {
                  entry.tags.coverpath = ios.newURI(decodeURI(item.getProperty(
                                             "http://songbirdnest.com/data/" +
                                             "1.0#primaryImageURL")), null,
                                             null).QueryInterface(Components.
                                             interfaces.nsIFileURL).file.path;
                } catch (e){}
                
                // Push the item's entry into the playlist's entry
                plEntry.items.push(entry);
              }
            }
            // Push the playlist's entry into the result
            this.crt.push(plEntry);
          }
        } catch(e) {
          foldersync.central.logEvent("sync-getItems",
                                      "GetItems thread terminated " +
                                      "unexpected:\n\n" + e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
          this.crt = null;
        }
      },
    },
    // The sync thread is responsible for the sync itself (in separate thread)
    sync: {
      // The thread object we're running in
      _thread: null,
      // The main thread's thread object
      _mainthread: null,
      // True, if the thread is currently running
      _running: false,
      
      // The OS we're running on
      _OS: null,
      
      // The current amount of task points
      _crtPoints: null,
      // The amount of task points the last message was sent
      _lstPoints: null,
      // The maximum amount of task points (100%)
      _maxPoints: null,
      
      // Initializes the sync thread (MAINTHREAD ONLY)
      _init: function(){
        try {
          foldersync.central.logEvent("sync-sync",
                                      "Initialisate sync thread", 5);
          this._thread = Components.classes["@mozilla.org/thread-manager;1"].
                         getService(Components.interfaces.nsIThreadManager).
                         newThread(0);
          this._mainthread =
                         Components.classes["@mozilla.org/thread-manager;1"].
                         getService(Components.interfaces.nsIThreadManager).
                         currentThread;
          this._OS = foldersync.central.getOS();
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Sync thread initialisation " +
                                      "failed:\n\n" + e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
      
      // Start the sync thread. (MAINTHREAD ONLY)
      _start: function(){
        try {
          foldersync.central.logEvent("sync-sync",
                                      "Start sync thread", 5);
          this._thread.dispatch(this, Components.interfaces.nsIThread.
                                      DISPATCH_NORMAL);
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Starting sync thread failed:\n\n" + e,
                                      1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
      
      // Initialize and start if necessary. (MAINTHREAD ONLY)
      ensureRunning: function(){
        if (!this._thread)
          this._init();
        if (!this._running)
          this._start();
      },
      
      // Are we running?
      isSyncing: function(){
        return this._running;
      },
      
      /* Notify listeners of a change
       * state (state): the state to send to listeners
       * syncqueueobject (syncqueueobject): the object to send
       */
      _notify: function(state, syncqueueobject){
        try {
          foldersync.central.logEvent("sync-sync",
                                      "Notify listeners of new state", 5);
          // Write current data to update thread
          foldersync.sync._threads.update.crt.state = state;
          foldersync.sync._threads.update.crt.
                     syncqueueobject = syncqueueobject;
          // Start update thread, we'll wait till it is completed
          this._mainthread.dispatch(foldersync.sync._threads.update,
                                    Components.interfaces.nsIThread.
                                    DISPATCH_SYNC);
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Starting update thread failed:\n\n" + e,
                                      1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
      
      // Get next SyncQueueObject or null if there is no next object
      _getNext: function(){
        try {
          foldersync.central.logEvent("sync-sync",
                                      "Get the next SyncQueueObject", 5);
          // Start getQueue thread, we'll wait till it is completed
          this._mainthread.dispatch(foldersync.sync._threads.getQueue,
                                    Components.interfaces.nsIThread.
                                    DISPATCH_SYNC);
          return foldersync.sync._threads.getQueue.crt;
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Starting getQueue thread failed:\n\n" +
                                      e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
      
      /* Get a specific Profile or null if there is no profile with the GUID
       * guid (string): the GUID of the Profile
       */
      _getProfile: function(guid){
        try {
          foldersync.central.logEvent("sync-sync",
                                      "Get Profile " + guid, 5);
          // Write GUID to getProfile thread
          foldersync.sync._threads.getProfile.crt = guid;
          // Start getProfile thread, we'll wait till it is completed
          this._mainthread.dispatch(foldersync.sync._threads.getProfile,
                                    Components.interfaces.nsIThread.
                                    DISPATCH_SYNC);
          return foldersync.sync._threads.getProfile.crt;
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Starting getProfile thread failed:" +
                                      "\n\n" + e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
      
      /* Get all media items in the playlists with the given GUIDs, see the
       * getItems thread for details on the result.
       * guids (array of string): the GUIDs of the playlists
       */
      _getItems: function(guids){
        try {
          foldersync.central.logEvent("sync-sync",
                                      "Get Items from " + guids, 5);
          // Write GUIDs to getItems thread
          foldersync.sync._threads.getItems.crt = guids;
          // Start getItems thread, we'll wait till it is completed
          this._mainthread.dispatch(foldersync.sync._threads.getItems,
                                    Components.interfaces.nsIThread.
                                    DISPATCH_SYNC);
          return foldersync.sync._threads.getItems.crt;
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Starting getItems thread failed:\n\n" +
                                      e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
        }
      },
      
      /* Get all nsIFiles of the (sub-)folder in 2 arrays
       * folder (nsIFile): the folder we want to enumerate
       * cArray (2 arrays of nsIFile): the files/folders we want to add
       */
      _enumFolder: function(folder, cArray){
        foldersync.central.logEvent("sync-sync", "Enumerate '" + folder.path +
                                                 "'", 5);
        // make sure we have an array
        var result = {};
        result.files = cArray ? (cArray.files ? cArray.files : []) : [];
        result.folders = cArray ? (cArray.folders ? cArray.folders : []) : [];
        // Get all entries
        var entries = null;
        try {
          entries = folder.directoryEntries;
        } catch (e) {
          foldersync.central.logEvent("sync-sync",
                                      "Failed to access '" + folder.path +
                                      "':\n\n" + e, 2,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
          return result;
        }
        // Go through all entries and register them
        while(entries.hasMoreElements())
        {
          var ent = entries.getNext();
          ent.QueryInterface(Components.interfaces.nsIFile);
          if (ent.isFile()) {
            result.files.push(ent);
          }
          if (ent.isDirectory()){
            result = this._enumFolder(ent, result);
            result.folders.push(ent);
          }
        }
        // Return the enumerated files
        return result;
      },
      
      /* Ensures that there are only valid characters in a string, if pref set
       * string (string): the string that should be corrected and returned
       * profile (profile): the profile that should be used
       */
      _ensureCharacters: function(string, profile){
        var aString = new String (string); // the string to check
        var rBuffer = ""; // a buffer to write to
        var result = ""; // the result
        var waitChar = profile.advanced.cutSpaces; // Wait for char, no spaces
        // Go through all characters, and write them to result
        for (var i = 0; i < aString.length; i++){
          var found = false;
          for each (var bChar in profile.advanced.blockedChars)
            if (aString[i] == bChar)
              found = true;
          if (found){
            rBuffer += profile.advanced.replaceChar;
            waitChar = false; // we had a character
          }
          else {
            if (profile.advanced.cutSpaces)
              if (aString[i] == " "){
                // cut spaces on beginning
                if (waitChar)
                  continue;
                // cut spaces on end
                rBuffer += " ";
                continue;
              }
            rBuffer += aString[i];
            // Append buffer on valid chars
            result += rBuffer;
            rBuffer = "";
            waitChar = false; // we had a character
          }
        }
        // We might need replace characters at the string's end
        if (!profile.advanced.cutReplaced){
          // check and drop spaces if pref is set
          if (profile.advanced.cutSpaces){
            var nBuffer = "";
            var sBuffer = "";
            for each (var c in rBuffer)
              if (c == " ")
                sBuffer += c;
              else {
                nBuffer += sBuffer;
                sBuffer = "";
                nBuffer += c;
              }
            result += nBuffer;
          } else
            result += rBuffer;
        }
        return result;
      },

      /* Removes diacritics letters, colliding them to the corresponding letter
       * str (string): the string to be treated
       */
	  _removeDiacritics: function  (str) {

        var defaultDiacriticsRemovalMap = [
          {'base':'A', 'letters':/[\u0041\u24B6\uFF21\u00C0\u00C1\u00C2\u1EA6\u1EA4\u1EAA\u1EA8\u00C3\u0100\u0102\u1EB0\u1EAE\u1EB4\u1EB2\u0226\u01E0\u00C4\u01DE\u1EA2\u00C5\u01FA\u01CD\u0200\u0202\u1EA0\u1EAC\u1EB6\u1E00\u0104\u023A\u2C6F]/g},
          {'base':'AA','letters':/[\uA732]/g},
          {'base':'AE','letters':/[\u00C6\u01FC\u01E2]/g},
          {'base':'AO','letters':/[\uA734]/g},
          {'base':'AU','letters':/[\uA736]/g},
          {'base':'AV','letters':/[\uA738\uA73A]/g},
          {'base':'AY','letters':/[\uA73C]/g},
          {'base':'B', 'letters':/[\u0042\u24B7\uFF22\u1E02\u1E04\u1E06\u0243\u0182\u0181]/g},
          {'base':'C', 'letters':/[\u0043\u24B8\uFF23\u0106\u0108\u010A\u010C\u00C7\u1E08\u0187\u023B\uA73E]/g},
          {'base':'D', 'letters':/[\u0044\u24B9\uFF24\u1E0A\u010E\u1E0C\u1E10\u1E12\u1E0E\u0110\u018B\u018A\u0189\uA779]/g},
          {'base':'DZ','letters':/[\u01F1\u01C4]/g},
          {'base':'Dz','letters':/[\u01F2\u01C5]/g},
          {'base':'E', 'letters':/[\u0045\u24BA\uFF25\u00C8\u00C9\u00CA\u1EC0\u1EBE\u1EC4\u1EC2\u1EBC\u0112\u1E14\u1E16\u0114\u0116\u00CB\u1EBA\u011A\u0204\u0206\u1EB8\u1EC6\u0228\u1E1C\u0118\u1E18\u1E1A\u0190\u018E]/g},
          {'base':'F', 'letters':/[\u0046\u24BB\uFF26\u1E1E\u0191\uA77B]/g},
          {'base':'G', 'letters':/[\u0047\u24BC\uFF27\u01F4\u011C\u1E20\u011E\u0120\u01E6\u0122\u01E4\u0193\uA7A0\uA77D\uA77E]/g},
          {'base':'H', 'letters':/[\u0048\u24BD\uFF28\u0124\u1E22\u1E26\u021E\u1E24\u1E28\u1E2A\u0126\u2C67\u2C75\uA78D]/g},
          {'base':'I', 'letters':/[\u0049\u24BE\uFF29\u00CC\u00CD\u00CE\u0128\u012A\u012C\u0130\u00CF\u1E2E\u1EC8\u01CF\u0208\u020A\u1ECA\u012E\u1E2C\u0197]/g},
          {'base':'J', 'letters':/[\u004A\u24BF\uFF2A\u0134\u0248]/g},
          {'base':'K', 'letters':/[\u004B\u24C0\uFF2B\u1E30\u01E8\u1E32\u0136\u1E34\u0198\u2C69\uA740\uA742\uA744\uA7A2]/g},
          {'base':'L', 'letters':/[\u004C\u24C1\uFF2C\u013F\u0139\u013D\u1E36\u1E38\u013B\u1E3C\u1E3A\u0141\u023D\u2C62\u2C60\uA748\uA746\uA780]/g},
          {'base':'LJ','letters':/[\u01C7]/g},
          {'base':'Lj','letters':/[\u01C8]/g},
          {'base':'M', 'letters':/[\u004D\u24C2\uFF2D\u1E3E\u1E40\u1E42\u2C6E\u019C]/g},
          {'base':'N', 'letters':/[\u004E\u24C3\uFF2E\u01F8\u0143\u00D1\u1E44\u0147\u1E46\u0145\u1E4A\u1E48\u0220\u019D\uA790\uA7A4]/g},
          {'base':'NJ','letters':/[\u01CA]/g},
          {'base':'Nj','letters':/[\u01CB]/g},
          {'base':'O', 'letters':/[\u004F\u24C4\uFF2F\u00D2\u00D3\u00D4\u1ED2\u1ED0\u1ED6\u1ED4\u00D5\u1E4C\u022C\u1E4E\u014C\u1E50\u1E52\u014E\u022E\u0230\u00D6\u022A\u1ECE\u0150\u01D1\u020C\u020E\u01A0\u1EDC\u1EDA\u1EE0\u1EDE\u1EE2\u1ECC\u1ED8\u01EA\u01EC\u00D8\u01FE\u0186\u019F\uA74A\uA74C]/g},
          {'base':'OI','letters':/[\u01A2]/g},
          {'base':'OO','letters':/[\uA74E]/g},
          {'base':'OU','letters':/[\u0222]/g},
          {'base':'P', 'letters':/[\u0050\u24C5\uFF30\u1E54\u1E56\u01A4\u2C63\uA750\uA752\uA754]/g},
          {'base':'Q', 'letters':/[\u0051\u24C6\uFF31\uA756\uA758\u024A]/g},
          {'base':'R', 'letters':/[\u0052\u24C7\uFF32\u0154\u1E58\u0158\u0210\u0212\u1E5A\u1E5C\u0156\u1E5E\u024C\u2C64\uA75A\uA7A6\uA782]/g},
          {'base':'S', 'letters':/[\u0053\u24C8\uFF33\u1E9E\u015A\u1E64\u015C\u1E60\u0160\u1E66\u1E62\u1E68\u0218\u015E\u2C7E\uA7A8\uA784]/g},
          {'base':'T', 'letters':/[\u0054\u24C9\uFF34\u1E6A\u0164\u1E6C\u021A\u0162\u1E70\u1E6E\u0166\u01AC\u01AE\u023E\uA786]/g},
          {'base':'TZ','letters':/[\uA728]/g},
          {'base':'U', 'letters':/[\u0055\u24CA\uFF35\u00D9\u00DA\u00DB\u0168\u1E78\u016A\u1E7A\u016C\u00DC\u01DB\u01D7\u01D5\u01D9\u1EE6\u016E\u0170\u01D3\u0214\u0216\u01AF\u1EEA\u1EE8\u1EEE\u1EEC\u1EF0\u1EE4\u1E72\u0172\u1E76\u1E74\u0244]/g},
          {'base':'V', 'letters':/[\u0056\u24CB\uFF36\u1E7C\u1E7E\u01B2\uA75E\u0245]/g},
          {'base':'VY','letters':/[\uA760]/g},
          {'base':'W', 'letters':/[\u0057\u24CC\uFF37\u1E80\u1E82\u0174\u1E86\u1E84\u1E88\u2C72]/g},
          {'base':'X', 'letters':/[\u0058\u24CD\uFF38\u1E8A\u1E8C]/g},
          {'base':'Y', 'letters':/[\u0059\u24CE\uFF39\u1EF2\u00DD\u0176\u1EF8\u0232\u1E8E\u0178\u1EF6\u1EF4\u01B3\u024E\u1EFE]/g},
          {'base':'Z', 'letters':/[\u005A\u24CF\uFF3A\u0179\u1E90\u017B\u017D\u1E92\u1E94\u01B5\u0224\u2C7F\u2C6B\uA762]/g},
          {'base':'a', 'letters':/[\u0061\u24D0\uFF41\u1E9A\u00E0\u00E1\u00E2\u1EA7\u1EA5\u1EAB\u1EA9\u00E3\u0101\u0103\u1EB1\u1EAF\u1EB5\u1EB3\u0227\u01E1\u00E4\u01DF\u1EA3\u00E5\u01FB\u01CE\u0201\u0203\u1EA1\u1EAD\u1EB7\u1E01\u0105\u2C65\u0250]/g},
          {'base':'aa','letters':/[\uA733]/g},
          {'base':'ae','letters':/[\u00E6\u01FD\u01E3]/g},
          {'base':'ao','letters':/[\uA735]/g},
          {'base':'au','letters':/[\uA737]/g},
          {'base':'av','letters':/[\uA739\uA73B]/g},
          {'base':'ay','letters':/[\uA73D]/g},
          {'base':'b', 'letters':/[\u0062\u24D1\uFF42\u1E03\u1E05\u1E07\u0180\u0183\u0253]/g},
          {'base':'c', 'letters':/[\u0063\u24D2\uFF43\u0107\u0109\u010B\u010D\u00E7\u1E09\u0188\u023C\uA73F\u2184]/g},
          {'base':'d', 'letters':/[\u0064\u24D3\uFF44\u1E0B\u010F\u1E0D\u1E11\u1E13\u1E0F\u0111\u018C\u0256\u0257\uA77A]/g},
          {'base':'dz','letters':/[\u01F3\u01C6]/g},
          {'base':'e', 'letters':/[\u0065\u24D4\uFF45\u00E8\u00E9\u00EA\u1EC1\u1EBF\u1EC5\u1EC3\u1EBD\u0113\u1E15\u1E17\u0115\u0117\u00EB\u1EBB\u011B\u0205\u0207\u1EB9\u1EC7\u0229\u1E1D\u0119\u1E19\u1E1B\u0247\u025B\u01DD]/g},
          {'base':'f', 'letters':/[\u0066\u24D5\uFF46\u1E1F\u0192\uA77C]/g},
          {'base':'g', 'letters':/[\u0067\u24D6\uFF47\u01F5\u011D\u1E21\u011F\u0121\u01E7\u0123\u01E5\u0260\uA7A1\u1D79\uA77F]/g},
          {'base':'h', 'letters':/[\u0068\u24D7\uFF48\u0125\u1E23\u1E27\u021F\u1E25\u1E29\u1E2B\u1E96\u0127\u2C68\u2C76\u0265]/g},
          {'base':'hv','letters':/[\u0195]/g},
          {'base':'i', 'letters':/[\u0069\u24D8\uFF49\u00EC\u00ED\u00EE\u0129\u012B\u012D\u00EF\u1E2F\u1EC9\u01D0\u0209\u020B\u1ECB\u012F\u1E2D\u0268\u0131]/g},
          {'base':'j', 'letters':/[\u006A\u24D9\uFF4A\u0135\u01F0\u0249]/g},
          {'base':'k', 'letters':/[\u006B\u24DA\uFF4B\u1E31\u01E9\u1E33\u0137\u1E35\u0199\u2C6A\uA741\uA743\uA745\uA7A3]/g},
          {'base':'l', 'letters':/[\u006C\u24DB\uFF4C\u0140\u013A\u013E\u1E37\u1E39\u013C\u1E3D\u1E3B\u017F\u0142\u019A\u026B\u2C61\uA749\uA781\uA747]/g},
          {'base':'lj','letters':/[\u01C9]/g},
          {'base':'m', 'letters':/[\u006D\u24DC\uFF4D\u1E3F\u1E41\u1E43\u0271\u026F]/g},
          {'base':'n', 'letters':/[\u006E\u24DD\uFF4E\u01F9\u0144\u00F1\u1E45\u0148\u1E47\u0146\u1E4B\u1E49\u019E\u0272\u0149\uA791\uA7A5]/g},
          {'base':'nj','letters':/[\u01CC]/g},
          {'base':'o', 'letters':/[\u006F\u24DE\uFF4F\u00F2\u00F3\u00F4\u1ED3\u1ED1\u1ED7\u1ED5\u00F5\u1E4D\u022D\u1E4F\u014D\u1E51\u1E53\u014F\u022F\u0231\u00F6\u022B\u1ECF\u0151\u01D2\u020D\u020F\u01A1\u1EDD\u1EDB\u1EE1\u1EDF\u1EE3\u1ECD\u1ED9\u01EB\u01ED\u00F8\u01FF\u0254\uA74B\uA74D\u0275]/g},
          {'base':'oi','letters':/[\u01A3]/g},
          {'base':'ou','letters':/[\u0223]/g},
          {'base':'oo','letters':/[\uA74F]/g},
          {'base':'p','letters':/[\u0070\u24DF\uFF50\u1E55\u1E57\u01A5\u1D7D\uA751\uA753\uA755]/g},
          {'base':'q','letters':/[\u0071\u24E0\uFF51\u024B\uA757\uA759]/g},
          {'base':'r','letters':/[\u0072\u24E1\uFF52\u0155\u1E59\u0159\u0211\u0213\u1E5B\u1E5D\u0157\u1E5F\u024D\u027D\uA75B\uA7A7\uA783]/g},
          {'base':'s','letters':/[\u0073\u24E2\uFF53\u00DF\u015B\u1E65\u015D\u1E61\u0161\u1E67\u1E63\u1E69\u0219\u015F\u023F\uA7A9\uA785\u1E9B]/g},
          {'base':'t','letters':/[\u0074\u24E3\uFF54\u1E6B\u1E97\u0165\u1E6D\u021B\u0163\u1E71\u1E6F\u0167\u01AD\u0288\u2C66\uA787]/g},
          {'base':'tz','letters':/[\uA729]/g},
          {'base':'u','letters':/[\u0075\u24E4\uFF55\u00F9\u00FA\u00FB\u0169\u1E79\u016B\u1E7B\u016D\u00FC\u01DC\u01D8\u01D6\u01DA\u1EE7\u016F\u0171\u01D4\u0215\u0217\u01B0\u1EEB\u1EE9\u1EEF\u1EED\u1EF1\u1EE5\u1E73\u0173\u1E77\u1E75\u0289]/g},
          {'base':'v','letters':/[\u0076\u24E5\uFF56\u1E7D\u1E7F\u028B\uA75F\u028C]/g},
          {'base':'vy','letters':/[\uA761]/g},
          {'base':'w','letters':/[\u0077\u24E6\uFF57\u1E81\u1E83\u0175\u1E87\u1E85\u1E98\u1E89\u2C73]/g},
          {'base':'x','letters':/[\u0078\u24E7\uFF58\u1E8B\u1E8D]/g},
          {'base':'y','letters':/[\u0079\u24E8\uFF59\u1EF3\u00FD\u0177\u1EF9\u0233\u1E8F\u00FF\u1EF7\u1E99\u1EF5\u01B4\u024F\u1EFF]/g},
          {'base':'z','letters':/[\u007A\u24E9\uFF5A\u017A\u1E91\u017C\u017E\u1E93\u1E95\u01B6\u0225\u0240\u2C6C\uA763]/g}
        ];

        for(var i=0; i<defaultDiacriticsRemovalMap.length; i++) {
          str = str.replace(defaultDiacriticsRemovalMap[i].letters, defaultDiacriticsRemovalMap[i].base);
        }

        return str;

      },
      /* Replace tags (%%) with their values
       * crt (string): the user-entered string
       * tags (tags): the track's tags
       * playlist (string): the track's playlist name
       * profile (profile): the profile to use
       */
      _replaceTags: function(crt, tags, playlist, profile){
        // We'll store the result here, start with the given string
        var result = crt;
        // Get the tags
        var artist = tags.artist ? tags.artist : profile.defaulttags.artist;

		var artistfl = this._removeDiacritics(artist.charAt(0).toUpperCase());
        if(!isNaN(artistfl)){
        	artistfl = '0-9';
        }
        
        var title = tags.title ? tags.title : profile.defaulttags.title;
        var album = tags.album ? tags.album : profile.defaulttags.album;
        var albumartist = tags.albumartist ? tags.albumartist :
                                             ( profile.fallbacks.albumartist ?
                                               artist : profile.defaulttags.
                                                                albumartist );
        var discnumber = tags.discnumber ? tags.discnumber :
                                           profile.defaulttags.discnumber;
        var composer = tags.composer ? tags.composer :
                                       profile.defaulttags.composer;
        var genre = tags.genre ? tags.genre : profile.defaulttags.genre;
        var rating = tags.rating ? tags.rating : profile.defaulttags.rating;
        var year = tags.year ? tags.year : profile.defaulttags.year;
        var disccount = tags.disccount ? tags.disccount :
                                         profile.defaulttags.disccount;
        var producer = tags.producer ? tags.producer :
                                       profile.defaulttags.producer;
        var tracknum = tags.tracknum ? tags.tracknum :
                                       profile.defaulttags.tracknumber;
        while (tracknum.length < profile.structure.tnDigits){
          tracknum = "0" + tracknum;
        }
        // Replace 'em
        result = result.replace(/%artist%/g, artist);
        result = result.replace(/%artistfl%/g, artistfl);
        result = result.replace(/%title%/g, title);
        result = result.replace(/%album%/g, album);
        result = result.replace(/%albumartist%/g, albumartist);
        result = result.replace(/%discnumber%/g, discnumber);
        result = result.replace(/%composer%/g, composer);
        result = result.replace(/%genre%/g, genre);
        result = result.replace(/%rating%/g, rating);
        result = result.replace(/%playlist%/g, playlist);
        result = result.replace(/%year%/g, year);
        result = result.replace(/%disccount%/g, disccount);
        result = result.replace(/%producer%/g, producer);
        result = result.replace(/%tracknumber%/g, tracknum);
        // Return the file-name safe form
        return this._ensureCharacters(result, profile);
      },
      
      /* Updates the point value, sends notification, returns true for cancel
       * value (integer): number of task points completed
       * syncqueueobject (syncqueueobject): the object for notification
       */
      _updateTaskPoints: function(value, syncqueueobject){
        this._crtPoints += value;
        if ((this._crtPoints/this._maxPoints)-0.01 >=
            (this._lstPoints/this._maxPoints)){
          this._lstPoints = this._crtPoints;
          this._notify({
                          event: "status-update",
                          value: (this._crtPoints/this._maxPoints)*100
                        }, syncqueueobject);
        }
        return syncqueueobject.cancel;
      },
      
      // The Thread's code
      run: function(){
        try {
          this._running = true;
          foldersync.central.logEvent("sync-sync",
                                      "Sync thread started", 4);
          // Process the whole queue
          var crtObject = this._getNext();
          while (crtObject){
            // the current Sync
            var crtSync = crtObject.sync;
            
            foldersync.central.logEvent("sync-sync",
                                        "Sync started: \n\n" +
                                        "Target: " + crtSync.
                                                     targetFolder + "\n" +
                                        "Profile: " + crtSync.
                                                      profileGUID + "\n", 4);
            // notify listeners that we started a new sync
            this._notify({value:0, event:"started"}, crtObject);
            
            // get the current profile
            var crtProfile = this._getProfile(crtSync.profileGUID);
            // Make sure we got a profile
            if (!crtProfile){
              foldersync.central.logEvent("sync-sync",
                                          "Sync error:\n\n" + "Profile " +
                                          crtSync.profileGUID + " is not " +
                                          "avaiable, Sync cancelled.", 1,
                                          "chrome://foldersync/content/" +
                                          "sync.js");
              crtObject.cancel = true;
              this._notify({value:0, event:"failed-noprofile"}, crtObject);
              // get next Object
              crtObject = this._getNext();
              continue;
            }
            // Get the target folder
            var tFolder = Components.classes["@mozilla.org/file/local;1"].
                          createInstance(Components.interfaces.nsILocalFile);
            try {
              tFolder.initWithPath(crtSync.targetFolder);
            } catch(e){
              foldersync.central.logEvent("sync-sync",
                                          "Sync error:\n\n" + "Folder '" +
                                          crtSync.targetFolder + "' is" +
                                          "invalid. Sync cancelled.", 1,
                                          "chrome://foldersync/content/" +
                                          "sync.js", e.lineNumber);
              this._notify({value:0, event:"failed-invalid"}, crtObject);
              // get next Object
              crtObject = this._getNext();
              continue;
            }
            if (!tFolder.exists()){
              foldersync.central.logEvent("sync-sync",
                                          "Sync error:\n\n" + "Folder '" +
                                          crtSync.targetFolder + "' does" +
                                          "not exist. Sync cancelled.", 1,
                                          "chrome://foldersync/content/" +
                                          "sync.js");
              this._notify({value:0, event:"failed-invalid"}, crtObject);
              // get next Object
              crtObject = this._getNext();
              continue;
            }
            
            // Get files at target
            var tFiles = this._enumFolder(tFolder);
            var tFolders = tFiles.folders;
            tFiles = tFiles.files;
            
            // Get the items to sync
            var crtItems = this._getItems(crtSync.playlists);
            if (!crtItems){
              foldersync.central.logEvent("sync-sync",
                                          "Sync error:\n\n" + "Could not get" +
                                          "media items for playlists, " +
                                          "Sync cancelled.", 1,
                                          "chrome://foldersync/content/" +
                                          "sync.js");
              this._notify({value:0, event:"failed-noitems"}, crtObject);
              // get next Object
              crtObject = this._getNext();
              continue;
            }
            
            //If required, sort items according to the criteria defined in the sorting scheme
            if (crtProfile.playlists.isEnabled && crtProfile.playlists.isSorted){
              //Prepare array with tag names in order of precedence according to sorting scheme, with related sort orders (ascending or descending)
              var tagRankTmp2 = null;
              var tagRankTmp = crtProfile.playlists.sortingScheme.match(/^%[a-z]+(:[ad])?%(,%[a-z]+(:[ad])?%)*$/g);
              var tagRank = [];
              if (!tagRankTmp){
                foldersync.central.logEvent("sync-sync",
                                            "Sync error:\n\n" + "Bad sorting" +
                                            " schema, " +
                                            "Sync cancelled.", 1,
                                            "chrome://foldersync/content/" +
                                            "sync.js");
                this._notify({value:0, event:"failed-badsortingschema"}, crtObject);
                // get next Object
                crtObject = this._getNext();
                continue;
              }
              tagRankTmp = crtProfile.playlists.sortingScheme.match(/%([a-z]+:?[ad]?)%/g);
              for (var i=0;i<tagRankTmp.length;i++){
                tagRankTmp2 = tagRankTmp[i].match(/^%([a-z]+):?([ad])?%$/);
                if ((tagRankTmp2[2] == null) || (tagRankTmp2[2] == "a")){
                  tagRank.push({name : "%"+tagRankTmp2[1]+"%", order : 1});
                }
                else {
                  tagRank.push({name : "%"+tagRankTmp2[1]+"%", order : -1});
                }
              }  
              // Functions that sort items within a media list according to a scheme
              var compStr = function(a, b){
                if (!a && !b) return  0; // null equals null
                if (!a && b)  return -1; // null is lower than everything
                if (!b && a)  return  1; // null is lower than everything
                var as = a;
                var bs = b;
                if (as == bs){return 0;}
                var pos = 0;
                var max = as.length > bs.length ? as.length : bs.length;
                while ((as[pos] == bs[pos]) && (pos < max)) {pos++}
                var result = as.charCodeAt(pos) - bs.charCodeAt(pos);
                if (result == 0)
                  return as.length - bs.length;
                return result;
              };
              var compareItems = function(a, b) {
                var ret = 0;
                for (var i=0;i<tagRank.length;i++) {
                  switch (tagRank[i].name){
                    case "%artist%":
                      ret = compStr(a.tags.artist, b.tags.artist)*tagRank[i].order;
                      break;  
                    case "%title%":
                      ret = compStr(a.tags.title, b.tags.title)*tagRank[i].order;
                      break;  
                    case "%album%":
                      ret = compStr(a.tags.album, b.tags.album)*tagRank[i].order;
                      break;  
                    case "%genre%":
                      ret = compStr(a.tags.genre, b.tags.genre)*tagRank[i].order;
                      break;  
                    case "%rating%":
                      ret = compStr(a.tags.rating, b.tags.rating)*tagRank[i].order;
                      break;  
                    case "%tracknumber%":
                      ret = ((a.tags.tracknum?a.tags.tracknum:0)-(b.tags.tracknum?b.tags.tracknum:0))*tagRank[i].order;
                      break;  
                    case "%playlist%":
                      ret = compStr(a.tags.playlist, b.tags.playlist)*tagRank[i].order;
                      break;  
                    case "%albumartist%":
                      ret = compStr(a.tags.albumartist, b.tags.albumartist)*tagRank[i].order;
                      break;  
                    case "%year%":
                      ret = compStr(a.tags.year, b.tags.year)*tagRank[i].order;
                      break;  
                    case "%discnumber%":
                      ret = ((a.tags.discnumber?a.tags.discnumber:0)-(b.tags.discnumber?b.tags.discnumber:0))*tagRank[i].order;
                      break;  
                    case "%disccount%":
                      ret = ((a.tags.disccount?a.tags.disccount:0)-(b.tags.disccount?b.tags.disccount:0))*tagRank[i].order;
                      break;  
                    case "%composer%":
                      ret = compStr(a.tags.composer, b.tags.composer)*tagRank[i].order;
                      break;  
                    case "%producer%":
                      ret = compStr(a.tags.producer, b.tags.producer)*tagRank[i].order;
                      break;  
                    default:   
                      //Unknown sorting tag name: ignored in tag sorting
                      ret = 0;
                  }
                  if (ret!=0) return ret;
                }   
                return 0;
              };
              for each (var plEntry in crtItems){
                try {
                  plEntry.items.sort(compareItems);
                } catch (e) {
                  foldersync.central.logEvent("sync-sync",
                                      "Sorting playlists failed:\n\n" +
                                      e, 1,
                                      "chrome://foldersync/content/sync.js",
                                      e.lineNumber);
                  throw e;
                }
              }
            }
            
            // Get the prefix for relative paths in playlists
            var plRel = "";
            var plBase = null; // the playlist's folder (nsIFile)
            if (crtProfile.playlists.isEnabled){
              var relParts = crtProfile.playlists.toFolder.split("/");
              var aFile = tFolder.clone(); // a file to track where we are
              for each (var part in relParts){
                // Add inverse parts to plRel
                switch (part){
                case ".":
                  break;
                case "..":
                  if (aFile.parent){
                    aFile = aFile.parent;
                    plRel = aFile.leafName +
                              crtProfile.playlists.splitChar + plRel;
                  } else {
                    foldersync.central.logEvent("sync-sync",
                                                "Relative path for " +
                                                "playlist not " +
                                                "avaiable:\n\n" +
                                                "No parent directory " +
                                                "for '" + aFile.path + "'",
                                                1, "chrome://foldersync/" +
                                                "content/sync.js");
                  }
                  break;
                default:
                  try {
                    aFile.append(part);
                    plRel = ".." + crtProfile.playlists.splitChar +
                              plRel;
                  } catch (e) {
                    foldersync.central.logEvent("sync-sync",
                                                "Path for " +
                                                "playlist not " +
                                                "avaiable:\n\n" + e,
                                                1, "chrome://foldersync/" +
                                                "content/sync.js",
                                                e.lineNumber);
                  }
                }
              }
              // and there we are!
              if (crtProfile.playlists.doRelativePoint)
                plRel = "." + crtProfile.playlists.splitChar + plRel;
              plBase = aFile;
              foldersync.central.logEvent("sync-sync", "Playlist's path " +
                                          "prefix is '" + plRel +  "'", 5);
            }
            
            // This will store error messages here
            var failedFiles = [];
            // These arrays will store what things are to copy/update or delete
            var copyFiles = []; //{from:<nsIFile>, to:<nsIFile>, name:<string>}
            var keepFiles = []; //nsIFile, files we should keep at target
            var deleteFiles = []; //nsIFile
            // This stores the number of songs we enumerated
            var numEntries = 0;
            // Go through all Playlists
            for each (var pList in crtItems){
              foldersync.central.logEvent("sync-sync", "Prepare playlist '" +
                                          pList.name + "'", 5);
              var playlist = null; // This holds the playlist, if any
              // Playlist init code
              if (crtProfile.playlists.isEnabled){
                switch (crtProfile.playlists.format){
                case "m3u":
                  playlist = "";
                  break;
                case "m3uEx":
                  playlist = "#EXTM3U\r\n";
                  break;
                }
              }
              // Go through all Songs
              for each (var song in pList.items){
                foldersync.central.logEvent("sync-sync", "Prepare '" +
                                            song.path + "'", 5);
                // Get the current file
                var crtFile = Components.classes["@mozilla.org/file/local;1"].
                              createInstance(Components.interfaces.
                                                        nsILocalFile);
                try {
                  crtFile.initWithPath(song.path);
                } catch(e){
                  foldersync.central.logEvent("sync-sync",
                                              "File '" + crtFile.path + "' " +
                                              "is invalid:\n\n" + e, 2,
                                              "chrome://foldersync/content/" +
                                              "sync.js", e.lineNumber);
                  // Go to the next one...
                  continue;
                }
                if (!crtFile.exists()){
                  foldersync.central.logEvent("sync-sync",
                                              "File '" + crtFile.path +
                                              "' does not exist.", 2,
                                              "chrome://foldersync/content/" +
                                              "sync.js");
                  // Go to the next one...
                  continue;
                }
                // Get the target sub folder and the target file name
                var tSubFolder = tFolder.clone();
                var tSubFolders = []; // array of sub-folder names for playlist
                var tName = crtFile.leafName;
                if (crtProfile.structure.isEnabled){
                  // Get target folder and file name
                  var tParts = crtProfile.structure.schema.split("/");
                  for (var i = 0; i < tParts.length; i++){
                    var cItem = this._replaceTags(tParts[i], song.tags,
                                                  pList.name, crtProfile);
                    if (i != tParts.length - 1){
                      // Append if it is a folder
                      tSubFolder.append(cItem);
                      tSubFolders.push(cItem);
                      // If it is the cover folder we might copy the cover
                      if (crtProfile.structure.doCovers &&
                          (tParts[i] == crtProfile.structure.coverSchema) &&
                          song.tags.coverpath){
                        // Generate cover file name with extension
                        var coverName = crtProfile.structure.coverFile;
                        var sCover = song.tags.coverpath.split(".");
                        if (sCover.length > 1)
                          coverName += "." + sCover[sCover.length-1];
                        // Get the cover file
                        var coverF = tSubFolder.clone();
                        coverF.append(coverName);
                        // We won't copy if there is already a cover file
                        var exists = coverF.exists();
                        if (!exists)
                          for each (var file in copyFiles){
                            if ((file.to == tSubFolder.path) &&
                                (file.name == coverName)){
                              exists = true;
                              break;
                            }
                          }
                        if (!exists){
                          // Get cover file
                          var coverSF = Components.classes["@mozilla.org/" +
                                                           "file/local;1"].
                                        createInstance(Components.interfaces.
                                                       nsILocalFile);
                          try {
                            coverSF.initWithPath(song.tags.coverpath);
                            if (!coverSF.exists())
                              throw Components.Exception(song.tags.coverpath +
                                                         " does not exist.");
                            // Push the file in the to copy array, if not there
                            var copyFile = {
                                              from: coverSF,
                                              to: tSubFolder.clone(),
                                              name: coverName
                                            };
                            var found = false;
                            for each (var cf in copyFiles)
                              if ((cf.from.path == copyFile.from.path) &&
                                  (cf.to.path == copyFile.to.path) &&
                                  (cf.name == copyFile.name))
                                found = true;
                            if (!found){
                              foldersync.central.logEvent("sync-sync",
                                                          "Cover '" +
                                                          song.tags.coverpath +
                                                          "' will go to '" +
                                                          coverF.path + "'",
                                                          5);
                              copyFiles.push({
                                               from: coverSF,
                                               to: tSubFolder.clone(),
                                               name: coverName
                                              });
                            }
                            
                          } catch (e){
                            foldersync.central.logEvent("sync-sync",
                                                        "Cover file '" + song.
                                                        tags.coverpath + "' " +
                                                        "caused unexpected " + 
                                                        "Problem:\n\n" + e, 1,
                                                        "chrome://foldersync/" +
                                                        "content/sync.js",
                                                        e.lineNumber);
                          }
                        } else {
                          keepFiles.push(coverF); // we won't delete the file
                          foldersync.central.logEvent("sync-sync", "Cover " +
                                                      "exists already at '" +
                                                      coverF.path + "'", 5);
                        }
                      }
                    } else {
                      // Use as file name if it isn't a folder
                      tName = cItem;
                      // Append file extension, if any
                      var sName = crtFile.leafName.split(".");
                      if (sName.length > 1){
                        tName += "." + sName[sName.length-1];
                      }
                      // Maximum file name length
                      if (tName.length > crtProfile.advanced.fMaxLength) {
                        var temp_name = "";
                        for (var z = 0; z < crtProfile.advanced.fMaxLength-22;
                             z++){
                          temp_name += tName[z];
                        }
                        temp_name += crtProfile.advanced.replaceChar;
                        for (var z = tName.length-20; z < tName.length; z++){
                          temp_name+=tName[z];
                        }
                        tName=temp_name;
                      }
                    }
                  }
                }
                // Get the target file
                var tFile = tSubFolder.clone();
                tFile.append(tName);
                
                // Add the item to the playlist, if any
                if (crtProfile.playlists.isEnabled){
                  // Get the relative path for the current song
                  var relPath = plRel;
                  for each (var part in tSubFolders){
                    relPath +=  part + crtProfile.playlists.splitChar;
                  }
                  relPath += tName;
                  // Add the line(s) to the playlist
                  switch (crtProfile.playlists.format){
                  case "m3uEx":
                    playlist += "\r\n#EXTINF:" +
                                Math.round(song.tags.duration / 1000000) +
                                "," + (song.tags.artist ? song.tags.artist +
                                       " - " : "") +
                                (song.tags.title ? song.tags.title :
                                 crtProfile.defaulttags.title) + "\r\n";
                  case "m3u":
                    playlist += relPath + "\r\n";
                    break;
                  }
                }
                
                // Check if the file exists, and decide if we want to overwrite
                if (tFile.exists()){
                  if (crtProfile.flags.doUpdate){
                    /* Compare last changed date, if preference is set,
                     * 2000 because of FAT storing in 2 second steps
                     */
                    if (((tFile.lastModifiedTime + 2000 >
                          crtFile.lastModifiedTime) &&
                         (tFile.lastModifiedTime - 2000 <
                          crtFile.lastModifiedTime))){
                      foldersync.central.logEvent("sync-sync", "File '" +
                                                  tFile.path + "' hasn't " +
                                                  "modifyed, won't copy it.",5);
                      keepFiles.push(tFile); // we won't delete the file
                      // Go to the next one...
                      continue;
                    }
                  } else {
                    foldersync.central.logEvent("sync-sync", "File '" +
                                                tFile.path + "' exists " +
                                                "already, won't copy it.",5);
                    keepFiles.push(tFile); // we won't delete the file
                    // Go to the next one...
                    continue;
                  }
                }
                
                // Add to the copy array, if it is not there (another playlist)
                var copyFile = {from: crtFile, to: tSubFolder, name: tName};
                var found = false;
                for each (var cf in copyFiles)
                  if ((cf.from.path == copyFile.from.path) &&
                      (cf.to.path == copyFile.to.path) &&
                      (cf.name == copyFile.name))
                    found = true;
                if (!found){
                  foldersync.central.logEvent("sync-sync", "File '" +
                                              song.path + "' will go to '" +
                                              tSubFolder.path + "' with " +
                                              "name '" + tName + "'", 5);
                  copyFiles.push(copyFile);
                } else
                  foldersync.central.logEvent("sync-sync", "File '" +
                                              song.path + "' already in " +
                                              "copy array to '" + tSubFolder.
                                              path + "' with name '" +
                                              tName + "'", 5);
                
                // We did a song, check if we want to notify
                numEntries++;
                if (numEntries % 1000 == 0)
                  this._notify({value:0, event:"initing"}, crtObject);
                if (crtObject.cancel)
                  break;
              }
              // Playlist finalize / write code
              if (crtProfile.playlists.isEnabled){
                var extension = "";
                switch (crtProfile.playlists.format){
                case "m3u":
                case "m3uEx":
                  extension = "m3u";
                  break;
                }
                
                // Get the playlist's nsIFile, and make sure it exists
                var pFile = plBase.clone();
                var pFileName = this._ensureCharacters(pList.name, crtProfile);
                pFile.append(pFileName + "." + extension);
                foldersync.central.logEvent("sync-sync",
                                            "Write playlist '" + pFile.path +
                                            "'", 5);
                try {
                  if (!pFile.exists()){
                    pFile.create(Components.interfaces.nsIFile.
                                 NORMAL_FILE_TYPE, 0664);
                  }
                  
                  // Open (and empty) file for writing and init a converter
                  var foStream = Components.classes["@mozilla.org/network/" +
                                                    "file-output-stream;1"].
                                 createInstance(Components.interfaces.
                                                nsIFileOutputStream);
                  foStream.init(pFile, 0x02 | 0x08 | 0x20, 0666, 0);
                  var converter = Components.classes["@mozilla.org/intl/" +
                                                     "converter-output-" +
                                                     "stream;1"].
                                  createInstance(Components.interfaces.
                                                 nsIConverterOutputStream);
                  converter.init(foStream, crtProfile.playlists.encoding,
                                 0, 0);
                  converter.writeString(playlist);
                  converter.close();
                  
                  // Make sure we won't delete the playlist
                  keepFiles.push(pFile);
                } catch (e){
                  failedFiles.push("[+] " + pFile.path);
                  foldersync.central.logEvent("sync-sync",
                                              "Writing playlist failed:\n\n" +
                                              e, 1,
                                              "chrome://foldersync/content/" +
                                              "sync.js",
                                              e.lineNumber);
                }
              }
              if (crtObject.cancel)
                  break;
            }
            
            // Check for files that could get deleted (if pref set)
            // Build sorted lists of file names, to compare more efficient
            var deleteNames = [];//string
            if (crtProfile.flags.doUpdate && (!crtProfile.flags.doDelete))
              for each (var entry in copyFiles){
                var Entry = entry.to.clone();
                Entry.append(entry.name);
                var ePath = Entry.path;
                if (!crtProfile.advanced.compareCase)
                  ePath = ePath.toUpperCase();
                deleteNames.push(ePath);
              }
            var keepNames = [];//string
            if (crtProfile.flags.doDelete)
              for each (var entry in keepFiles){
                var ePath = entry.path;
                if (!crtProfile.advanced.compareCase)
                  ePath = ePath.toUpperCase();
                keepNames.push(ePath);
              }
            var compStr = function(a, b){
              if (!(a && b))
                return 0; // Prevent fail
              var as = a;
              var bs = b;
              if (as == bs){return 0;}
              var pos = 0;
              var max = as.length > bs.length ? as.length : bs.length;
              while ((as[pos] == bs[pos]) && (pos < max)) {pos++}
              var result = as.charCodeAt(pos) - bs.charCodeAt(pos);
              if (result == 0)
                return as.length - bs.length;
              return result;
            };
            keepNames.sort(compStr);
            deleteNames.sort(compStr);
            // Sort target file array
            var compFile = function(a, b){
              if (!(a && b))
                return 0; // Prevent fail
              var as = a.path;
              var bs = b.path;
              if (!crtProfile.advanced.compareCase){
                as = as.toUpperCase();
                bs = bs.toUpperCase();
              }
              return compStr(as, bs);
            };
            tFiles.sort(compFile);
            // Go through all files that are already at the target folder
            var dCur = 0; // Cursor for deleteNames
            var kCur = 0; // Cursor for keepNames
            for each (var file in tFiles){
              // Notify...
              numEntries++;
              if (numEntries % 1000 == 0)
                this._notify({value:0, event:"initing"}, crtObject);
              if (crtObject.cancel)
                  break;
              
              var shouldDelete = false;
              var fPath = file.path;
              if (!crtProfile.advanced.compareCase)
                fPath = fPath.toUpperCase();
              // Look if the file should get keeped
              if (crtProfile.flags.doDelete){
                while ((compStr(keepNames[kCur],fPath) < 0) &&
                       (kCur < keepNames.length -1))
                  kCur++;
                if (keepNames[kCur] == fPath)
                  shouldDelete = false;
                else
                  shouldDelete = true;
              } else {
                // Look if the file should get deleted
                while ((compStr(deleteNames[dCur],fPath) < 0) &&
                       (dCur < deleteNames.length -1))
                  dCur++;
                if (deleteNames[dCur] == fPath)
                  shouldDelete = true;
              }
              foldersync.central.logEvent("sync-sync",
                                          "File '" + file.path + "' " +
                                          (shouldDelete ? "will " : "won't ") +
                                          "get deleted.", 5);
              if (shouldDelete)
                deleteFiles.push(file);
            }
            
            // We have now all information we need; start the sync!
            foldersync.central.logEvent("sync-sync",
                                        "Got all information, start sync.", 4);
            this._notify({value:0, event:"started-sync"}, crtObject);
            
            /* Calculate maximum number of taskPoints for progress bar
             * The following tasks are rated:
             * Delete of a file: 10 Point
             * Copy a file: 50 Points
             * Check if a folder can get deleted and delete it: 1 Point
             *
             * After every task completion with at least 1 % done since the
             * last notification, we'll send a notification to listeners.
             */
            this._maxPoints = deleteFiles.length * 10 +
                              copyFiles.length * 50 +
                              (crtProfile.flags.doDelete ?
                               tFolders.length * 1 : 0);
            this._crtPoints = 0;
            this._lstPoints = 0;
            
            // Delete files
            if (!crtObject.cancel)
              for each (var file in deleteFiles){
                try {
                  foldersync.central.logEvent("sync-sync",
                                              "Delete '" + file.path + "'", 5);
                  file.remove(false);
                } catch(e){
                  failedFiles.push("[-] " + file.path);
                  foldersync.central.logEvent("sync-sync",
                                              "Could not delete file '" +
                                              file.path + "':\n\n" +
                                              e, 1,
                                              "chrome://foldersync/content/" +
                                              "sync.js", e.lineNumber);
                }
                if (this._updateTaskPoints(10,crtObject))
                  break;
              }
            
            // Try to delete folders if option set
            if (!crtObject.cancel)
              if (crtProfile.flags.doDelete){
                for each (var folder in tFolders){
                  try {
                    // Delete folder if empty
                    if (!folder.directoryEntries.hasMoreElements())
                      folder.remove(false);
                  } catch (e){
                    failedFiles.push("[-] " + folder.path);
                    foldersync.central.logEvent("sync-sync",
                                                "Failed to delete " +
                                                "empty folder '" + folder.
                                                path + "':\n\n" + e, 1,
                                                "chrome://foldersync/" +
                                                "content/sync.js",
                                                e.lineNumber);
                  }
                  if (this._updateTaskPoints(1,crtObject))
                    break;
                }
              }
            
            // Copy new Files
            if (!crtObject.cancel)
              for each (var entry in copyFiles){
                try {
                  foldersync.central.logEvent("sync-sync",
                                              "Copy '" + entry.from.path +
                                              "' to '" + entry.to.path + "' " +
                                              "with the name '" + entry.name +
                                              "'", 5);
                  entry.from.copyTo(entry.to, entry.name);
                  // Fix lastModifiedTime problems on non-Windows OS.
                  if (this._OS != "Win32"){
                    var tFile = entry.to.clone();
                    tFile.append(entry.name);
                    tFile.lastModifiedTime = entry.from.lastModifiedTime;
                  }
                } catch(e){
                  failedFiles.push("[+] " + entry.from.path + " > " +
                                   entry.to.path + " (" + entry.name + ")");
                  foldersync.central.logEvent("sync-sync",
                                              "Could not copy file '" +
                                              entry.from.path + "' to '" +
                                              entry.to.path + "':\n\n" +
                                              e, 1,
                                              "chrome://foldersync/content/" +
                                              "sync.js", e.lineNumber);
                }
                if (this._updateTaskPoints(50,crtObject))
                  break;
              }
            
            // Do a Rockbox back-sync, if preference is set
            if ((!crtObject.cancel) && (crtProfile.flags.doRockbox))
              foldersync.rockbox.doSync(tFolder.path);
            
            // If there was a cancel, we'll send a cancelled message
            if (crtObject.cancel){
              foldersync.central.logEvent("sync-sync",
                                          "Sync cancelled \n\n" +
                                          "Target: " + crtSync.
                                                       targetFolder + "\n" +
                                          "Profile: " + crtSync.
                                                        profileGUID + "\n", 4);
              this._notify({
                             value:(this._crtPoints/this._maxPoints)*100,
                             event:"cancelled"
                           }, crtObject);
              
            } else {
              if (failedFiles.length == 0) {
                foldersync.central.logEvent("sync-sync",
                                            "Sync completed: \n\n" +
                                            "Target: " + crtSync.targetFolder +
                                            "\nProfile: " +
                                            crtSync.profileGUID + "\n", 4);
                // notify listeners that we finished the sync
                this._notify({value:100, event:"completed"}, crtObject);
              } else {
                foldersync.central.logEvent("sync-sync",
                                            "Sync completed with errors: " +
                                            "\n\nTarget: " + crtSync.
                                            targetFolder + "\nProfile: " +
                                            crtSync.profileGUID + "\n", 4);
                // notify listeners that we finished the sync
                this._notify({
                               value:100,
                               event:"completed-errors",
                               errors:failedFiles,
                             }, crtObject);
              }
            }
            // get next Object
            crtObject = this._getNext();
          }
          this._running = false;
          foldersync.central.logEvent("sync-sync",
                                      "Sync thread stopped", 4);
        } catch (e) {
          this._running = false;
          this._notify({event:"fatal"}, {sync:{}});
          foldersync.central.logEvent("sync-sync",
                                    "Sync thread terminated " +
                                    "unexpected:\n\n" + e, 1,
                                    "chrome://foldersync/content/sync.js",
                                    e.lineNumber);
        }
      },
    },
  },
  
  /* Notify listeners
   * state (state): the state to send
   * syncqueueobject (syncqueueobject): the queue object to send
   */
  _notify: function(state, syncqueueobject){
    try {
      foldersync.central.logEvent("sync",
                                  "Update listeners:\n\n" +
                                  "State: " + state.event + "\n" +
                                  "       " + state.value + "\n" +
                                  "Cancelled: " + syncqueueobject.
                                                  cancel + "\n" +
                                  "GUID:" + syncqueueobject.guid + "\n" +
                                  "Target: " + syncqueueobject.
                                               sync.targetFolder + "\n" +
                                  "Profile: " + syncqueueobject.
                                                sync.profileGUID, 5);
      for each (var listener in foldersync.sync._listeners){
        try {
          listener(state, syncqueueobject);
        } catch (e){
          foldersync.central.logEvent("sync",
                                      "Wasn't able to notify listener '" +
                                      listener + "':\n\n" + e, 3,
                                      "chrome://foldersync/content/" +
                                      "sync.js", e.lineNumber);
        }
      }
    } catch(e) {
      foldersync.central.logEvent("sync",
                                  "Listener update terminated " +
                                  "unexpected:\n\n" + e, 1,
                                  "chrome://foldersync/content/sync.js",
                                  e.lineNumber);
    }
  },
  
  // Startup code for the sync controller
  onLoad: function(e){
    foldersync.central.logEvent("sync", "Sync controller " +
                                "initialisation started.", 5);
    // Init / Clear arrays for queue and listeners
    this._queue = [];
    this._listeners = [];
    // We're done!
    foldersync.central.logEvent("sync", "Sync controller started.", 4);
  },
  // Shutdown code for the sync controller
  onUnload: function(e){
    foldersync.central.logEvent("sync", "Sync controller " +
                                "shutdown started.", 5);
    foldersync.central.logEvent("sync", "Sync controller stopped.", 4);
  },
  
  /* Generate a new Sync
   * folder (string): the target folder
   * profile (string): the GUID of the profile to use
   * plists (array of string): the GUIDs of the selected playlists
   */
  generateSync: function(folder, profile, plists){
    // return a sync with the given attributes
    return {
      targetFolder: folder, // The target folder
      profileGUID: profile, // The profile's GUID
      playlists: plists, // The playlists' GUIDs to use
    };
  },
  
  /* Generate a new SyncQueueObject
   * sync (sync): the sync the SyncQueueObject should be based on
   */
  generateSyncQueueObject: function(sync){
    // return a SyncQueueObject with the given sync
    return {
      sync: sync, // The sync
      guid: Components.classes["@mozilla.org/uuid-generator;1"].
            getService(Components.interfaces.nsIUUIDGenerator).generateUUID().
            toString(), // The GUID for referencing purposes
      cancel: false, // might be set to true, and the sync will be cancelled
    };
  },
  
  /* Add a SyncQueueObject to the sync queue
   * object (syncqueueobject)
   */
  addToQueue: function(object){
    try{
      foldersync.central.logEvent("sync",
                                  "Append SyncQueueObject:\n\n" +
                                  "GUID:" + object.guid +
                                  "\nTarget: " + object.sync.targetFolder +
                                  "\nProfile: " + object.sync.profileGUID, 4);
      this._queue.push(object);
      this._notify({value: 0, event:"added"}, object);      
    } catch (e){
      foldersync.central.logEvent("sync",
                                  "Adding sync object to queue failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/sync.js",
                                  e.lineNumber);
    }
  },
  
  /* Remove a specific SyncQueueObject from the sync queue
   * object (syncqueueobject): the object to remove
   */
  removeFromQueue: function(object){
    try {
      foldersync.central.logEvent("sync",
                                  "Remove SyncQueueObject:\n\n" +
                                  "GUID:" + object.guid +
                                  "\nTarget: " + object.sync.targetFolder +
                                  "\nProfile: " + object.sync.profileGUID, 4);
      var foundindex = -1;
      for (var i = 0; i < this._queue.length; i++){
        if (this._queue[i].guid == object.guid){
          foundindex = i;
          break;
        }
      }
      if (foundindex != -1){
        this._queue.splice(foundindex,1);
        this._notify({value: 0, event:"removed"}, object);
      } else {
        foldersync.central.logEvent("sync",
                                    "Removing sync object from queue failed:" +
                                    "\n\nObject is not registered." + e, 2,
                                    "chrome://foldersync/content/sync.js");
      }
    } catch (e){
      foldersync.central.logEvent("sync",
                                  "Removing sync object from queue failed:" +
                                  "\n\n" + e, 1,
                                  "chrome://foldersync/content/sync.js",
                                  e.lineNumber);
    }
  },
  
  // Ensure that the the sync queue is being executed
  ensureQueueRunning: function(){
    this._threads.sync.ensureRunning();
  },
  
  // Is there a Sync running?
  isSyncing: function(){
    return this._threads.sync.isSyncing();
  },
  
  /* Add the given sync to queue, perform it and return the queue object's GUID
   * sync (sync): the sync to add
   */
  performSync: function(sync){
    try {
      var so = this.generateSyncQueueObject(sync);
      var result = so.guid;
      this.addToQueue(so);
      this.ensureQueueRunning();
      return result;
    } catch (e){
      foldersync.central.logEvent("sync",
                                  "Preparing queue for sync failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/sync.js",
                                  e.lineNumber);
    }
  },
  
  /* Get avaiable playlist formats, return value: Array of Playlist formats
   * Each playlist format has the structure:
   * {
   *   name: <string>, // the name to pass foldersync.central.getLocaleString
   *   internalName: <string>, // the name to store with a profile
   * }
   */
  getPFormats: function(){
    return [
      { name: "playlistformat.m3u", internalName: "m3u" },
      { name: "playlistformat.m3uEx", internalName: "m3uEx" },
    ];
  },
  
  /* Add the given Listener to the list of listeners
   * listener (function(state,syncqueueobject)): the listener to add
   *   a state has the following structure:
   *   {
   *     value:<integer>, // 0-100 progress
   *     event:<string>, // the reason for a event to be fired, see sync thread
   *   }
   */
  addListener: function(listener){
    try{
      foldersync.central.logEvent("sync",
                                  "Append listener:\n\n" +
                                  listener, 5);
      this._listeners.push(listener);
    } catch (e){
      foldersync.central.logEvent("sync",
                                  "Adding listener to list failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/sync.js",
                                  e.lineNumber);
    }
  },
  
  /* Remove the given Listener
   * listener (function(state,syncqueueobject)): the listener to remove
   */
  removeListener: function(listener){
    try{
      foldersync.central.logEvent("sync",
                                  "Remove listener:\n\n" +
                                  listener, 5);
      var removed = false;
      // Search for the listener and remove it
      for (var i = 0; i < this._listeners.length; i++){
        if (this._listeners[i] == listener){
          foldersync.central.logEvent("sync",
                                      "Removed listener:\n\n" +
                                      this._listeners[i] + "\n" +
                                      "Index: " + i, 5);
          this._listeners.splice(i,1);
          removed = true;
        }
      }
      // Warn if the listener wasn't registered
      if (!removed)
        foldersync.central.logEvent("sync",
                                    "Removing listener from list failed:\n\n" +
                                    "'" + listener + "'" +
                                    "is no registered listener.", 2,
                                    "chrome://foldersync/content/sync.js");
    } catch (e){
      foldersync.central.logEvent("sync",
                                  "Removing listener from list failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/sync.js",
                                  e.lineNumber);
    }
  },
  
  /* Generate a Error Message out of a state object
   * state (state): the sync state object
   * returns null if there is no error within this state, or structure:
   * {
   *   error:<string>, // the error's short description
   *   message:<string>, // the detailed error message, if any
   *   fatal:<bool>, // if the error was fatal
   * }
   */
  generateErrorMessage: function(state){
    var result = null;
    if (state.event.split("failed-").length > 1){
      result = {};
      result.error = foldersync.central.getLocaleString("status.failed");
      switch (state.event){
      case "failed-noprofile":
        result.message = foldersync.central.getLocaleString("error.profile");
        break;
      case "failed-invalid":
        result.message = foldersync.central.getLocaleString("error.invalid");
        break;
      case "failed-noitems":
        result.message = foldersync.central.getLocaleString("error.noitems");
        break;
      case "failed-badsortingschema":
        result.message = foldersync.central.getLocaleString("error.badsortingschema");
        break;
      default:
        result.message = null;
      }
      result.fatal = true;
    }
    if (state.event == "completed-errors"){
      result = {};
      result.error = foldersync.central.getLocaleString("status.errors");
      result.message = foldersync.central.getLocaleString("error.some") + "\n";
      result.message += state.errors.join("\n");
      result.fatal = false;
    }
    if (state.event == "fatal"){ // This should NOT happen...
      result = {};
      result.error = foldersync.central.getLocaleString("status.failed");
      result.message = "";
      result.fatal = true;
    }
    return result;
  },
  
  // Get the current sync queue.
  getQueue: function(){
    // We send only a copy. See also note at getProfile thread
    return JSON.parse(JSON.stringify(this._queue));
  },
};