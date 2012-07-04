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
                                 NORMAL_FILE_TYPE, 0x664);
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