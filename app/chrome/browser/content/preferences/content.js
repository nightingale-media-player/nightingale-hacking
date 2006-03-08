//@line 37 "/cygdrive/c/builds/tinderbox/Fx-Mozilla1.8/WINNT_5.2_Depend/mozilla/browser/components/preferences/content.js"


const kDefaultFontType          = "font.default.%LANG%";
const kFontNameFmtSerif         = "font.name.serif.%LANG%";
const kFontNameFmtSansSerif     = "font.name.sans-serif.%LANG%";
const kFontNameListFmtSerif     = "font.name-list.serif.%LANG%";
const kFontNameListFmtSansSerif = "font.name-list.sans-serif.%LANG%";
const kFontSizeFmtVariable      = "font.size.variable.%LANG%";

var gContentPane = {
  _pane: null,
  
  init: function ()
  {
    this._pane = document.getElementById("paneContent");
    this._rebuildFonts();
    var menulist = document.getElementById("defaultFont");
    if (menulist.selectedIndex == -1) {
      menulist.insertItemAt(0, "", "", "");
      menulist.selectedIndex = 0;
    }
  },
  
  _rebuildFonts: function ()
  {
    var langGroupPref = document.getElementById("font.language.group");
    this._selectLanguageGroup(langGroupPref.value, 
                              this._readDefaultFontTypeForLanguage(langGroupPref.value) == "serif");
  },
  
  _selectLanguageGroup: function (aLanguageGroup, aIsSerif)
  {
    var prefs = [{ format   : aIsSerif ? kFontNameFmtSerif : kFontNameFmtSansSerif,
                   type     : "unichar", 
                   element  : "defaultFont",      
                   fonttype : aIsSerif ? "serif" : "sans-serif" },
                 { format   : aIsSerif ? kFontNameListFmtSerif : kFontNameListFmtSansSerif,
                   type     : "unichar", 
                   element  : null,               
                   fonttype : aIsSerif ? "serif" : "sans-serif" },
                 { format   : kFontSizeFmtVariable,      
                   type     : "int",     
                   element  : "defaultFontSize",  
                   fonttype : null }];
    var preferences = document.getElementById("contentPreferences");
    for (var i = 0; i < prefs.length; ++i) {
      var preference = document.getElementById(prefs[i].format.replace(/%LANG%/, aLanguageGroup));
      if (!preference) {
        preference = document.createElement("preference");
        var name = prefs[i].format.replace(/%LANG%/, aLanguageGroup);
        preference.id = name;
        preference.setAttribute("name", name);
        preference.setAttribute("type", prefs[i].type);
        preferences.appendChild(preference);
      }
      
      if (!prefs[i].element)
        continue;
        
      var element = document.getElementById(prefs[i].element);
      if (element) {
        element.setAttribute("preference", preference.id);
      
        if (prefs[i].fonttype)
          FontBuilder.buildFontList(aLanguageGroup, prefs[i].fonttype, element);

        preference.setElementValue(element);
      }
    }
  },

  _readDefaultFontTypeForLanguage: function (aLanguageGroup)
  {
    var defaultFontTypePref = kDefaultFontType.replace(/%LANG%/, aLanguageGroup);
    var preference = document.getElementById(defaultFontTypePref);
    if (!preference) {
      preference = document.createElement("preference");
      preference.id = defaultFontTypePref;
      preference.setAttribute("name", defaultFontTypePref);
      preference.setAttribute("type", "string");
      preference.setAttribute("onchange", "gContentPane._rebuildFonts();");
      document.getElementById("contentPreferences").appendChild(preference);
    }  
    return preference.value;
  },

  writeEnableImagesPref: function ()
  { 
    var enableImages = document.getElementById("enableImages");
    if (enableImages.checked) {
      var enableRestricted = document.getElementById("enableRestricted");
      return enableRestricted.checked ? 3 : 1;
    }
    return 2;
  },
  
  readEnableImagesPref: function ()
  {
    var pref = document.getElementById("permissions.default.image");
    var enableRestricted = document.getElementById("enableRestricted");    
    enableRestricted.disabled = pref.value == 2;
    return (pref.value == 1 || pref.value == 3);
  },
  
  readEnableRestrictedPref: function ()
  {
    var pref = document.getElementById("permissions.default.image");
    return (pref.value == 3);
  },
  
  _exceptionsParams: {
    install: { blockVisible: false, sessionVisible: false, allowVisible: true, prefilledHost: "", permissionType: "install" },
    popup:   { blockVisible: false, sessionVisible: false, allowVisible: true, prefilledHost: "", permissionType: "popup"   },
    image:   { blockVisible: true,  sessionVisible: false, allowVisible: true, prefilledHost: "", permissionType: "image"   }
  },
  
  showExceptions: function (aPermissionType)
  {
    var bundlePreferences = document.getElementById("bundlePreferences");
    var params = this._exceptionsParams[aPermissionType];
    params.windowTitle = bundlePreferences.getString(aPermissionType + "permissionstitle");
    params.introText = bundlePreferences.getString(aPermissionType + "permissionstext");
    document.documentElement.openWindow("Browser:Permissions",
                                        "chrome://browser/content/preferences/permissions.xul",
                                        "", params);
  },
  
  updateButtons: function (aButtonID, aPreferenceID)
  {
    var button = document.getElementById(aButtonID);
    var preference = document.getElementById(aPreferenceID);
    button.disabled = preference.value != true;
    return undefined;
  },
  
  showAdvancedJS: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/advanced-scripts.xul",
                                           "", null);  
  },

  showFonts: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/fonts.xul",
                                           "", null);  
  },

  showColors: function ()
  {
    document.documentElement.openSubDialog("chrome://browser/content/preferences/colors.xul",
                                           "", null);  
  }
};
