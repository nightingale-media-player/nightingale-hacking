// Whether or not app updates are enabled
pref("app.update.enabled", true);

// This preference turns on app.update.mode and allows automatic download and
// install to take place. We use a separate boolean toggle for this to make
// the UI easier to construct.
pref("app.update.auto", true);

// Defines how the Application Update Service notifies the user about updates:
//
// AUM Set to:        Minor Releases:     Major Releases:
// 0                  download no prompt  download no prompt
// 1                  download no prompt  download no prompt if no incompatibilities
// 2                  download no prompt  prompt
//
// See chart in nsUpdateService.js.in for more details
//
pref("app.update.mode", 1);

// If set to true, the Update Service will present no UI for any event.
pref("app.update.silent", false);

// Update service URL:
pref("app.update.url", "https://updates.songbirdnest.com/update/1/%PRODUCT%/%VERSION%/%BUILD_ID%/%BUILD_TARGET%/%LOCALE%/%CHANNEL%/update.xml");

// URL user can browse to manually if for some reason all update installation
// attempts fail.  TODO: Change this URL
pref("app.update.url.manual", "http://www.songbirdnest.com/home");

// A default value for the "More information about this update" link
// supplied in the "An update is available" page of the update wizard. 
pref("app.update.url.details", "http://www.songbirdnest.com/home");

// User-settable override to app.update.url for testing purposes.
//pref("app.update.url.override", "");

// Interval: Time between checks for a new version (in seconds)
//           default=1 day
pref("app.update.interval", 86400);

// Interval: Time before prompting the user to download a new version that 
//           is available (in seconds) default=1 day
pref("app.update.nagTimer.download", 86400);

// Interval: Time before prompting the user to restart to install the latest
//           download (in seconds) default=30 minutes
pref("app.update.nagTimer.restart", 1800);

// Interval: When all registered timers should be checked (in milliseconds)
//           default=5 seconds
pref("app.update.timer", 600000);

// Whether or not we show a dialog box informing the user that the update was
// successfully applied. This is off in Firefox by default since we show a 
// upgrade start page instead! Other apps may wish to show this UI, and supply
// a whatsNewURL field in their brand.properties that contains a link to a page
// which tells users what's new in this new update.
pref("app.update.showInstalledUI", false);

// 0 = suppress prompting for incompatibilities if there are updates available
//     to newer versions of installed addons that resolve them.
// 1 = suppress prompting for incompatibilities only if there are VersionInfo
//     updates available to installed addons that resolve them, not newer
//     versions.
pref("app.update.incompatible.mode", 0);

// Symmetric (can be overridden by individual extensions) update preferences.
// e.g.
//  extensions.{GUID}.update.enabled
//  extensions.{GUID}.update.url
//  extensions.{GUID}.update.interval
//  .. etc ..
//
pref("extensions.update.enabled", true);
pref("extensions.update.url", "https://extensions.songbirdnest.com/update/versioncheck/?reqVersion=%REQ_VERSION%&id=%ITEM_ID%&version=%ITEM_VERSION%&maxAppVersion=%ITEM_MAXAPPVERSION%&appID=%APP_ID%&appVersion=%APP_VERSION%&appOS=%APP_OS%&appABI=%APP_ABI%)");
pref("extensions.update.interval", 86400);  // Check for updates to Extensions and 
                                            // Feathers every week
// Non-symmetric (not shared by extensions) extension-specific [update] preferences
pref("extensions.getMoreExtensionsURL", "https://extensions.songbirdnest.com/extensions/");
pref("extensions.getMoreThemesURL", "https://extensions.songbirdnest.com/feathers/");

pref("xpinstall.whitelist.add", "extensions.songbirdnest.com");
pref("xpinstall.whitelist.add.103", "extensions.songbirdnest.com");
