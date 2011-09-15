/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

function runTest() {

  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var testItemURL = ios.newURI("file:///alpha.mp3", null, null);
  var testItemProperties =
    SBProperties.createArray({ artistName:  "A",
                               albumName:   "Alpha",
                               trackName:   "Track Alpha",
                               trackNumber: "1",
                               year:        "2000" });
  
  var history = Cc["@songbirdnest.com/Songbird/PlaybackHistoryService;1"]
                  .getService(Ci.sbIPlaybackHistoryService);
                  
  history.clear();
  
  var entry = Cc["@songbirdnest.com/Songbird/PlaybackHistoryEntry;1"]
                .createInstance(Ci.sbIPlaybackHistoryEntry);
               
  var library = createLibrary("test_playbackhistoryservice", null, false);
  
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  libraryManager.registerLibrary(library, false);
  
  var item = library.createMediaItem(testItemURL, testItemProperties);
  
  var itemPlayedAt = new Date();
  var itemPlayDuration = 1000 * 1000 * 1000;
  
  entry.init(item, itemPlayedAt, itemPlayDuration, null);
  
  assertEqual(entry.item, item);
  assertEqual(entry.timestamp, itemPlayedAt.getTime());
  assertEqual(entry.duration, itemPlayDuration);
  
  history.addEntry(entry);

  var itemPlayedAt2 = new Date().getTime() + 10000;
  var itemPlayDuration2 = 1000 * 1000 * 999;
  
  var annotations = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                      .createInstance(Ci.sbIPropertyArray);
  annotations.appendProperty("http://songbirdnest.com/data/1.0#scrobbled", 
                             "scrobbled");
  
  var entry2 = history.createEntry(item, 
                                   itemPlayedAt2, 
                                   itemPlayDuration2, 
                                   annotations);
  history.addEntry(entry2);
  
  entry2.setAnnotation("http://songbirdnest.com/data/1.0#iheartthis", "true");
  
  var getByAnnotationArray = history.getEntriesByAnnotation("http://songbirdnest.com/data/1.0#iheartthis", "true");
  assertEqual(getByAnnotationArray.length, 1);
  var getByAnnotationEntry = getByAnnotationArray.queryElementAt(0, Ci.sbIPlaybackHistoryEntry);
  assertEqual(getByAnnotationEntry.getAnnotation("http://songbirdnest.com/data/1.0#iheartthis"),
              "true");
              
  entry2.setAnnotation("http://songbirdnest.com/data/1.0#ashamedDoNotScrobble", "true");

  var annotations2 = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                      .createInstance(Ci.sbIPropertyArray);
  annotations2.appendProperty("http://songbirdnest.com/data/1.0#iheartthis", 
                              "true");
  annotations2.appendProperty("http://songbirdnest.com/data/1.0#ashamedDoNotScrobble", 
                              "true");

  var getByAnnotationsArray = history.getEntriesByAnnotations(annotations2, 1);
  assertEqual(getByAnnotationsArray.length, 1);
  var getByAnnotationsEntry = getByAnnotationsArray.queryElementAt(0, Ci.sbIPlaybackHistoryEntry);
  assertEqual(getByAnnotationsEntry.getAnnotation("http://songbirdnest.com/data/1.0#iheartthis"), 
              "true");
  assertEqual(getByAnnotationsEntry.getAnnotation("http://songbirdnest.com/data/1.0#ashamedDoNotScrobble"), 
              "true");
    
  entry2.removeAnnotation("http://songbirdnest.com/data/1.0#iheartthis");
  
  log("The playback history service has " + history.entryCount + " entries.");
  assertEqual(history.entryCount, 2);
  
  var entriesArray = [entry2, entry];
  var enumEntries = history.entries;

  // I hate you javascript scoping.    
  {
    let i = 0;
    while(enumEntries.hasMoreElements()) {
      let entry = enumEntries.getNext()
                             .QueryInterface(Ci.sbIPlaybackHistoryEntry); 
      assertEqual(entry.item, entriesArray[i].item);
      assertEqual(entry.timestamp, entriesArray[i].timestamp);
      assertEqual(entry.duration, entriesArray[i].duration);
      
      ++i;
    }
  }
  
  {
    let entries = history.getEntriesByTimestamp(itemPlayedAt, itemPlayedAt + 1);
    assertEqual(entries.length, 1);
    
    let entry = entries.queryElementAt(0, Ci.sbIPlaybackHistoryEntry);
    assertEqual(entry.item, item);
    assertEqual(entry.timestamp, itemPlayedAt.getTime());
    assertEqual(entry.duration, itemPlayDuration);
  }
  
  var entry_fromGetEntry = history.getEntryByIndex(0);
  assertEqual(entry_fromGetEntry.item, item);
  assertEqual(entry_fromGetEntry.timestamp, itemPlayedAt2);
  assertEqual(entry_fromGetEntry.duration, itemPlayDuration2);
  
  var entries = history.getEntriesByIndex(0, 2);
  assertEqual(entries.length, 2);

  history.removeEntry(entry);
  log("After removing one entry, the history service has " + history.entryCount + " entry");
  assertEqual(history.entryCount, 1);
  
  var remainingEntry = entries.queryElementAt(0, Ci.sbIPlaybackHistoryEntry);
  assertEqual(remainingEntry.item, item);
  assertEqual(remainingEntry.timestamp, itemPlayedAt2);
  assertEqual(remainingEntry.duration, itemPlayDuration2);
  
  return;
}
