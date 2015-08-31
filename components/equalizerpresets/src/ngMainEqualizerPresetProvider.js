/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

"use strict";
const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;
const Cr = Components.results;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DebugUtils.jsm");
Cu.import("resource://app/jsmodules/DOMUtils.jsm");

const LOG = DebugUtils.generateLogFunction("ngMainEqualizerPresetProvider", 2);

const PRESETS_STORAGE_FILE_PATH = "equalizer_presets.xml";

/**
 * \class ngMainEqualizerPresetProvider
 * \cid {5e503ed9-1c8c-4135-8d25-61b0835475b4}
 * \contractid @getnightingale.com/equalizer-presets/main-provider;1
 * \implements ngIMainEqualizerPresetProvider
 * \implements ngIEqualizerPresetCollection
 */
function ngMainEqualizerPresetProvider() {
    if(this._createStorageIfNotExisting())
        this._presets = this._getPresets();
    else
        this._presets = [];

    XPCOMUtils.defineLazyServiceGetter(this, "_observerService",
                    "@mozilla.org/observer-service;1", "nsIObserverService");
}

ngMainEqualizerPresetProvider.prototype = {
    classDescription: "Main provider responsible for saving user set presets",
    classID:          Components.ID("{5e503ed9-1c8c-4135-8d25-61b0835475b4}"),
    contractID:       "@getnightingale.com/equalizer-presets/main-provider;1",
    QueryInterface:   XPCOMUtils.generateQI([Ci.ngIMainEqualizerPresetProvider,
                                             Ci.ngIEqualizerPresetCollection]),

    _observerService: null,

    _presets: null,
    _xml: null,

    get presets() {
        if(this._presets)
            return ArrayConverter.nsIArray(this._presets);
        else
            throw Cr.NS_ERROR_UNEXPECTED;
    },
    getPresetByName: function(aName) {
        var index = this._getPresetIndex(aName);
        if(index >= 0)
            return this._presets[index];
        return null;
    },
    hasPresetNamed: function(aName) {
        return this._presetExists(aName);
    },

    deletePreset: function(aName) {
        if(aName.length == 0) {
            throw Cr.NS_ERROR_ILLEGAL_VALUE;
        }
        
        if(this._presetExists(aName)) {
            this._presets.splice(this._getPresetIndex(aName), 1);

            this._xml.getElementsByTagName("presets")[0].removeChild(this._getXMLPresetByName(aName, this._xml));
            this._savePresets();

            // notify everybody who's interested about the deletion of the preset
            this._observerService.notifyObservers(this, "equalizer-preset-deleted", aName);
        }
    },
    savePreset: function(aName, aValues) {
        if(aName.length == 0 || aValues.length != 10)
            throw Cr.NS_ERROR_ILLEGAL_VALUE;
    
        if(this._presetExists(aName))
            this._overwritePreset(aName, aValues);
        else
            this._createPreset(aName, aValues);

        // notify everybody who's interested about the preset that's been saved
        this._observerService.notifyObservers(this, "equalizer-preset-saved", aName);
    },

    _getPresets: function() {
        return this._getPresetsFromXML(this._xml);
    },
    _savePresets: function() {
        var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
		var path = this._getFilePathInProfile(PRESETS_STORAGE_FILE_PATH);
        this._saveXMLDocument(this._xml, path);
    },
    _presetExists: function(aName) {
        return this._presets.some(function(item) {
            return item.name == aName;
        });
    },
    _getPresetIndex: function(aName) {
        var index = -1;
        this._presets.some(function(item, i) {
            if(item.name == aName) {
                index = i;
                return true;
            }
            return false;
        });
        return index;
    },
    _createPreset: function(aName, aValues) {
        var mutablePreset = Cc["@getnightingale.com/equalizer-presets/mutable;1"]
                            .createInstance(Ci.ngIMutableEqualizerPreset);
        mutablePreset.setName(aName);
        mutablePreset.setValues(aValues);

        this._presets.push(mutablePreset.QueryInterface(Ci.ngIEqualizerPreset));

        this._xml.getElementsByTagName("presets")[0]
            .appendChild(this._createXMLPreset(aName, ArrayConverter.JSArray(aValues)));
        this._savePresets();
    },
    _overwritePreset: function(aName, aValues) {
        var preset = this._presets[this._getPresetIndex(aName)]
                        .QueryInterface(Ci.ngIMutableEqualizerPreset);
        preset.setValues(aValues);

        var oldNode = this._getXMLPresetByName(aName, this._xml);
        this._xml.getElementsByTagName("presets")[0]
            .replaceChild(this._createXMLPreset(aName, ArrayConverter.JSArray(aValues)), oldNode);
        this._savePresets();
    },
    _createStorageIfNotExisting: function() {
		// Get the file object
		var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
		var path = this._getFilePathInProfile(PRESETS_STORAGE_FILE_PATH);
		file.initWithPath(path);
		
		// Let's verify that the file actually exists, and if it doesn't create it
		if(!file.exists()) {
            LOG("XML isn't existing yet");
            this._createPresetStorage(file);
        }

        this._xml = this._readXMLDocument(path);
        return file.exists();
    },

    // From hereon be XML & file madness
    _createPresetStorage: function(file) {
        file.create("text/XML",0777);
	
        // get a DOM parser
        var parser = Cc["@mozilla.org/xmlextras/domparser;1"]
                        .createInstance(Ci.nsIDOMParser);
			
        // create the file content
        var data = this._createXMLString([]);
	
        // Create the document
        var DOMDoc = parser.parseFromString(data,"text/xml");

        // safe the file in the profile
        LOG("Saving the newly created XML file to the profile");
        this._saveXMLDocument(DOMDoc, file.path);
	},
    _createXMLString: function(aPresets) {
        var data = "<presets>";
        for(var preset in aPresets){
            data += this._presetToXML(preset, aPreset[preset]);
        }
        data += "</presets>";
        return data;
	},
    _presetToXML: function(aPresetName, aBands) {
		var data = "<preset name='" + DOMUtils.encodeTextForXML(aPresetName) + "'>\n";
		aBands.forEach(function(band) {
			data +="<band>" + DOMUtils.encodeTextForXML(band) + "</band>\n";
		});
		data += "</preset>\n";
		return data;
	},
    _createXMLPreset: function(aName, aBands) {
        var presetNode = this._xml.createElement("preset");
        presetNode.setAttribute("name", aName);

        aBands.forEach(function(band) {
            var bandNode = this._xml.createElement("band"),
                bandValue = this._xml.createTextNode(band
                                .QueryInterface(Ci.nsISupportsDouble).data);
            bandNode.appendChild(bandValue);
            presetNode.appendChild(bandNode);
        }, this)
        return presetNode;
    },
    _readXMLDocument: function(aPath) {
		return DOMUtils.loadDocument("file://"+aPath);
	},
    _getXMLPresetByName: function(aName, aDocument) {
        LOG("Getting the XML preset node for the preset named " + aName);
        var presetArr = DOMUtils.getElementsByAttribute(aDocument, "name", aName);
        if(presetArr.length > 0) {
            return presetArr[0];
        }
        return null;        
    },
    _getPresetsFromXML: function(aDocument) {
        var presets = [];
        var presetNodes = aDocument.getElementsByTagName("preset");

        for(var i = 0; i < presetNodes.length; ++i) {
            presets.push(this._getPresetFromXML(presetNodes[i]));
        }

        return presets;
    },
    _getPresetFromXML: function(aPreset) {
        var mutablePreset = Cc["@getnightingale.com/equalizer-presets/mutable;1"]
                            .createInstance(Ci.ngIMutableEqualizerPreset);

        mutablePreset.setName(aPreset.getAttribute("name"));
        var bands = [];
        var bandNodes = aPreset.getElementsByTagName("band");
        for(var i = 0; i < bandNodes.length; ++i) {
            bands.push(bandNodes[i].firstChild.nodeValue);
        }
        mutablePreset.setValues(ArrayConverter.nsIArray(bands));

        return mutablePreset.QueryInterface(Ci.ngIEqualizerPreset);
    },
    _getFilePathInProfile: function(aRelativePath) {
        // get a file to represent the profile directory
        var file = Cc["@mozilla.org/file/directory_service;1"]
                            .getService(Ci.nsIProperties)
                            .get("ProfD", Ci.nsIFile);
        // add the relative path to the profiledir
        var path = aRelativePath.split("/");
        for (var i = 0, sz = path.length; i < sz; i++) {
            if (path[i] != "")
                file.append(path[i]);
        }
        return file.path;
	},
    _saveXMLDocument: function(aDomDoc, aPath) {
        // get folder to write to
        var file = Cc["@mozilla.org/file/local;1"]
                    .createInstance(Ci.nsILocalFile);
        file.initWithPath(aPath);
        // get a filestream
        var stream = Cc["@mozilla.org/network/file-output-stream;1"]
                    .createInstance(Ci.nsIFileOutputStream);
        stream.init(file, -1, -1, 0);
		
        // write the xml document into the stream
        var serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"]
                            .createInstance(Ci.nsIDOMSerializer);
        serializer.serializeToStream(aDomDoc, stream, "UTF-8");
    }
};

var NSGetModule = XPCOMUtils.generateNSGetModule([ngMainEqualizerPresetProvider]);