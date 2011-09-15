/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
function songbird_extensionManager_overlay() {
  const Ci = Components.interfaces;
  const Cc = Components.classes;

  window.removeEventListener("load", songbird_extensionManager_overlay, false);
  window.addEventListener("unload",
    function songbird_extensionManager_unload() {
      gExtensionsView.builder.removeListener(TemplateBuilderListener);
    }, false);

  function fixResizers() {
    // Resizers are broken in release mode for some reason... Worthy of further
    // investigation. Do this for now, but remove once bug 2324 is resolved.
    var resizersList = document.getElementsByTagName("resizer");
    var resizerCount = resizersList.length;
    for (var index = 0; index < resizerCount; index++) {
      var resizer = resizersList.item(index);
      if (resizer)
        resizer.dir = resizer.dir;
    }
  }

  // RDF rebuild listener to listen for the richlistbox of addon items being
  // built, and look through the install.rdfs and set hidden on elements as
  // appropriate.
  var TemplateBuilderListener = {
    willRebuild: function(){},
    didRebuild: function SBEMDidRebuild(aBuilder) {
      const RDF_INSTALL_MANIFEST =
        gRDF.GetResource("urn:mozilla:install-manifest");
      const RDF_EM_HIDDEN =
        gRDF.GetResource("http://www.mozilla.org/2004/em-rdf#hidden");
      const RDF_TRUE = gRDF.GetLiteral("true");
      var ios = Cc["@mozilla.org/network/io-service;1"]
                  .getService(Ci.nsIIOService);

      // step through the generated richlistitems (for each addon) and look at
      // which ones should be hidden, by looking into each install.rdf
      Array.forEach(gExtensionsView.children, function(itemElem) {
        if (!(itemElem.hasAttribute("addonID")) ||
            itemElem.hasAttribute("songbird_checked"))
        {
          // not a richlistitem about an addon, or an item we've already checked
          return;
        }
        // mark this item as checked so we don't read install.rdf again (until
        // we reload this window...)
        itemElem.setAttribute("songbird_checked", true);

        // look for the addon install location, and check that it's in a
        // restricted location (app-global, app-system-share)
        var addonID = itemElem.getAttribute("addonID");
        var installLocation = gExtensionManager.getInstallLocation(addonID);
        if (!installLocation.restricted) {
          // install location is not restricted, don't allow hidden addons
          itemElem.setAttribute("restricted", false);
          return;
        }

        // RDF load observer in which to actually do the work, because it's
        // likely that we never had to load install.rdf at all in this run
        var RDFLoadObserver = {
          onBeginLoad: function(){},
          onInterrupt: function(){},
          onResume: function(){},
          onError: function(){/* nothing we can do here, ignore */},
          onEndLoad: function(aSink) {
            aSink.removeXMLSinkObserver(this);
            if (rdfDS.HasAssertion(RDF_INSTALL_MANIFEST,
                                   RDF_EM_HIDDEN,
                                   RDF_TRUE,
                                   true))
            {
              // we have a <em:hidden>true</em:hidden> in the <Description
              // about="urn:mozilla:install-manifest">; hide the richlistitem
              itemElem.setAttribute("hidden", true);
            }
          },
          QueryInterface: XPCOMUtils.generateQI([Ci.nsIRDFXMLSinkObserver])
        };

        // find the install.rdf in the installed extension, and read the
        // datasource
        var installRDF = installLocation.getItemFile(addonID, "install.rdf");
        var rdfDS = gRDF.GetDataSource(ios.newFileURI(installRDF).spec)
                        .QueryInterface(Ci.nsIRDFRemoteDataSource)
                        .QueryInterface(Ci.nsIRDFXMLSink);
        rdfDS.addXMLSinkObserver(RDFLoadObserver);
        if (rdfDS.loaded) {
          // the datasource has already loaded; we won't get an onEndLoad, so
          // fire it manually to trigger the hiding
          RDFLoadObserver.onEndLoad(rdfDS);
        }
      });
    },
    QueryInterface: XPCOMUtils.generateQI([Ci.nsIXULBuilderListener])
  };

  fixResizers();

  // add a rebuild listener so that when the RDF-generated richlistitems get
  // created we go and mark things hidden
  gExtensionsView.builder.addListener(TemplateBuilderListener);
  // force a call in case this was hooked up after the initial build
  TemplateBuilderListener.didRebuild(gExtensionsView.builder);
}

function openURL(url) {
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                      .getService(Components.interfaces.nsIWindowMediator);
  var mainWin = wm.getMostRecentWindow("Songbird:Main");
  if (mainWin && mainWin.window && mainWin.window.gBrowser) {
    mainWin.window.gBrowser.loadURI(url, null, null, null, "_blank");
    mainWin.focus();
    return;
  }

  openURLExternal(url);
}

function openURLExternal(url) {
  var externalLoader = (Components
            .classes["@mozilla.org/uriloader/external-protocol-service;1"]
          .getService(Components.interfaces.nsIExternalProtocolService));
  var nsURI = (Components
          .classes["@mozilla.org/network/io-service;1"]
          .getService(Components.interfaces.nsIIOService)
          .newURI(url, null, null));
  externalLoader.loadURI(nsURI, null);
}

window.addEventListener("load", songbird_extensionManager_overlay, false);
