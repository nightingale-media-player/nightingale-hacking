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

// Update service URL - see branding prefs
// pref("app.update.url", ****);

// URL user can browse to manually if for some reason all update installation
// attempts fail.  - see branding prefs
// pref("app.update.url.manual", ****);

// A default value for the "More information about this update" link
// supplied in the "An update is available" page of the update wizard.
// - see branding prefs
// pref("app.update.url.details", ****);

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
// - see branding prefs
// pref("extensions.update.url", ****);
pref("extensions.update.interval", 86400);  // Check for updates to Extensions and
                                            // Feathers every day
// Non-symmetric (not shared by extensions) extension-specific [update] preferences
// - see branding prefs
// pref("extensions.getMoreExtensionsURL", ****);
// pref("extensions.getMoreThemesURL", ****);

pref("extensions.blocklist.enabled", true);
pref("extensions.blocklist.interval", 86400);
// see branding prefs
// pref("extensions.blocklist.url", ****);
// pref("extensions.blocklist.detailsURL", ****);
pref("extensions.ignoreMTimeChanges", false);

// We whitelist addons and translate - see branding prefs
// pref("xpinstall.whitelist.add", ****);
// pref("xpinstall.whitelist.add.0", ****);

// We may want to whitelist addons.mozilla.org in the future.
//pref("xpinstall.whitelist.add.1", "addons.mozilla.org");

pref("nightingale.recommended_addons.update.enabled", true);
// Interval: Time in seconds between checks for a new add-on bundle.
//           default=1 day
pref("nightingale.recommended_addons.update.interval", 86400);

