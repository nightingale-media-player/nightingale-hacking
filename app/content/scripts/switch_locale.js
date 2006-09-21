/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
 */

// based on Firefox Locale Switcher, by Benjamin Smedberg (http://benjamin.smedbergs.us/blog/2005-11-29/locale-switcher-15/)

function switchLocale(locale, wantmessagebox) {
  try 
  {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
    var curLocale = "en-US";
    try {
      curLocale = prefs.getCharPref("general.useragent.locale");
    }
    catch (e) { }

    if (locale != curLocale) {
      prefs.setCharPref("general.useragent.locale", locale);
      if (wantmessagebox) {
        var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
        var prop = sbs.createBundle("chrome://songbird/locale/songbird.properties");
        var str = "This setting will take effect after you restart Songbird"; // todo: restart the current window instead !
        var title = "Localization";
        try {
          // These can throw if the strings don't exist.
          str = prop.GetStringFromName("message.needrestart");
          title = prop.GetStringFromName("message.localization");
        } catch (e) { }
        
        sbRestartBox(title, str);
      }
    }
  }
  catch ( err )
  {
    alert( err );
  }
}

function fillLocaleList(menu) {
  try 
  {
    var prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefBranch);
    var curLocale = "en-US";
    try {
      curLocale = prefs.getCharPref("general.useragent.locale");
    }
    catch (e) { }

    var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"].getService(Components.interfaces.nsIToolkitChromeRegistry);

    var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
    var langNames = sbs.createBundle("chrome://global/locale/languageNames.properties");
    var regNames  = sbs.createBundle("chrome://global/locale/regionNames.properties");

    /* clear the existing children */
    var children = menu.childNodes;
    for (var i = children.length; i > 0; --i) {
      menu.removeChild(children[i - 1]);
    }

    var locales = cr.getLocalesForPackage("songbird");
    var elements = Array();
    
    while (locales.hasMore()) {
      var locale = locales.getNext();

      var parts = locale.split(/-/);

      var displayName;
      try {
        displayName = langNames.GetStringFromName(parts[0]);
        if (parts.length > 1) {
          try {
            displayName += " (" + regNames.GetStringFromName(parts[1].toLowerCase()) + ")";
          }
          catch (e) {
            displayName += " (" + parts[1] + ")";
          }
        }
      }
      catch (e) {
        displayName = locale;
      }

      var item = document.createElement("menuitem");
      var className = menu.parentNode.getAttribute("class");

      item.setAttribute("label", displayName);
      item.setAttribute("name", "locale.switch");
      item.setAttribute("type", "radio");
      item.setAttribute("class", className);
      if (curLocale == locale) {
        item.setAttribute("checked", "true");
      }
      item.setAttribute("oncommand", "switchLocale(\"" + locale + "\", true)");
      
      elements.push(item);
    }
    
    elements.sort(sortLanguages);
    for (var i =0;i<elements.length;i++) menu.appendChild(elements[i]);
  }
  catch ( err )
  {
    alert( err );
  }
}

function sortLanguages(a, b) 
{
  var aname = a.getAttribute("label");
  var bname = b.getAttribute("label");
  if (aname == bname) return 0;
  if (aname < bname) return -1;
  return 1;
}
