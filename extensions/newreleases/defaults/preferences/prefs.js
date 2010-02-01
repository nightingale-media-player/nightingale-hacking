// geographical location preferences
pref("extensions.newreleases.country", "ALL");

// "artist" or "date"
pref("extensions.newreleases.groupby", "date");

// Used to detect the first time the extension is run 
pref("extensions.newreleases.firstrun", true);

// Filter the library artists, or show all newreleases
pref("extensions.newreleases.filterLibraryArtists", true);

// Our flag for whether or not the last concert data retrieval failed
pref("extensions.newreleases.networkfailure", false);

// Temp variables for holding progress values
pref("extensions.newreleases.progress.pct", 0);
pref("extensions.newreleases.progress.txt", "");

// Debugging preference
pref("extensions.newreleases.debug", false);

// Musicbrainz only provides data up to four months in advance
pref("extensions.newreleases.futuremonths", 4);

// Show release type and track count
pref("extensions.newreleases.showExtendedInfo", false);
pref("extensions.newreleases.showgcal", false);
