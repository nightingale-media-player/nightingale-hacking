// Used to detect the first time the extension is run
pref("extensions.seeqpod.firstrun", true);
// Used to detect when an update has occured that affects the format of the
// profile. See also SEEQPOD_PROFILE_VERSION at the top of main.js
pref("extensions.seeqpod.profileversion", 0);

// Number of results to fetch from Seeqpod (before link checking and de-duping)
pref("extensions.seeqpod.maxresults", 100);
// Override the default search engine
pref('browser.search.defaultenginename', 'SeeqPod');
pref('browser.search.selectedEngine', 'SeeqPod');
