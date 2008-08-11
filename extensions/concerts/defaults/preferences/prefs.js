// geographical location preferences
pref("extensions.concerts.country", 1);
pref("extensions.concerts.state", 4);
pref("extensions.concerts.city", 26330);

// "artist" or "date"
pref("extensions.concerts.groupby", "artist");

// Used to detect the first time the extension is run 
pref("extensions.concerts.firstrun", true);

// Filter the library artists, or show all concerts
pref("extensions.concerts.filterLibraryArtists", true);

// Our flag for whether or not the last concert data retrieval failed
pref("extensions.concerts.networkfailure", false);

// Temp variables for holding progress values
pref("extensions.concerts.progress.pct", 0);
pref("extensions.concerts.progress.txt", "");

// Debugging preference
pref("extensions.concerts.debug", false);
