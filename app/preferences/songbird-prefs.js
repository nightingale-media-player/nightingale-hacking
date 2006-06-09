//Songbird Default Prefs.
pref("toolkit.defaultChromeURI", "chrome://songbird/content/xul/songbird.xul");
pref("general.useragent.extra.songbird", "Songbird/0.2(dev)");
pref("general.skins.selectedSkin", "rubberducky/0.2");
pref("config.trim_on_minimize", true); // Dump the current ram footprint when minimized.

pref("browser.sessionhistory.max_total_viewers", 0);

pref("browser.dom.window.dump.enabled", true);

//Scan for some extra plugins.
pref("plugin.scan.SunJRE", "1.3");
pref("plugin.scan.Acrobat", "5.0");
pref("plugin.scan.Quicktime", "5.0");

pref("plugin.scan.WindowsMediaPlayer", "7.0");
pref("plugin.scan.WindowsMediaPlayer", "8.0");
pref("plugin.scan.WindowsMediaPlayer", "9.0");
pref("plugin.scan.WindowsMediaPlayer", "10.0");

pref("general.useragent.vendorComment", "ax");
pref("security.xpconnect.activex.global.hosting_flags", 9);
pref("security.classID.allowByDefault", false);

/* Windows Media Player */
pref("capability.policy.default.ClassID.CID6BF52A52-394A-11D3-B153-00C04F79FAA6", "AllAccess");
/* QuickTime Player */
pref("capability.policy.default.ClassID.CID22D6F312-B0F6-11D0-94AB-0080C74C7E95", "AllAccess");

/* password manager */
pref("signon.rememberSignons", true);
pref("signon.expireMasterPassword", false);
pref("signon.SignonFileName", "signons.txt");