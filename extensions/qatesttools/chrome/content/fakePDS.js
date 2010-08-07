/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

var DialogController =
{
  _dialogClosing: false,
  
  _httpServer: null,
  _httpServerPort: 8080,
  _httpServerEnabled: false,
  _httpServerRunning: false,
  _httpServerRestart: false,
  
  _baseURL: "http://localhost:",
  
  _enableImageRequest: true,
  _enableDLLRequest: true,
  _enableDITRequest: true,
  
  _firmwareImageVersion: "1.0",
  _firmwareImageFile: null,
  
  _firmwareDLLVersion: "1.0",
  _firmwareDLLFile: null,
  
  _ditFile: null,

  _registeredImagePath: null,
  _registeredDLLPath: null,
  _registeredDITPath: null,
  
  _logRequestEnabled: false,
  _logResponseEnabled: false,
  
  //
  // Public Methods
  //
  onDialogLoad: function() {
    this._httpServer = Cc["@mozilla.org/server/jshttp;1"]
                         .createInstance(Ci.nsIHttpServer);
  },
  
  onDialogClose: function() {
    this._httpServerEnabled = false;
    this._dialogClosing = true;
    this._overrideServer();
    this._stopHttpServer();
    return true;
  },
  
  onEnableOverrideServer: function() {
    var checkbox = document.getElementById("fakepds-override-server");
    this._httpServerEnabled = checkbox.checked;
    this._overrideServer();
    this._restartHttpServer();
  },
  
  onChangeOverrideServerPort: function() {
    var textbox = document.getElementById("fakepds-override-server-port");
    this._httpServerPort = parseInt(textbox.value);
    this._restartHttpServer();    
  },
  
  onEnableFirmwareImageRequest: function() {
    var checkbox = document.getElementById("fakepds-firmware-image-enabled");
    this._enableImageRequest = checkbox.checked;
    this._restartHttpServer();
  },
  
  onEnableFirmwareDLLRequest: function() {
    var checkbox = document.getElementById("fakepds-firmware-dll-enabled");
    this._enableDLLRequest = checkbox.checked;
    this._restartHttpServer();
  },
  
  onChangeFirmwareImageVersion: function() {
    var textbox = document.getElementById("fakepds-firmware-image-version");
    this._firmwareImageVersion = textbox.value;
  },
  
  onChangeFirmwareDLLVersion: function() {
    var textbox = document.getElementById("fakepds-firmware-dll-version");
    this._firmwareDLLVersion = textbox.value;
  },
  
  onBrowseFirmwareImageFile: function() {
    var file = this._openFilePicker("Choose Firmware Image", 
                                    null, 
                                    [{title: "Firmware Image Files", ext: "*.zip"}]);
    if(file) {
      var fileField = document.getElementById("fakepds-firmware-image-file");
      fileField.file = file;
      
      this._firmwareImageFile = file;
    }
    
    this._restartHttpServer();
  },
  
  onBrowseFirmwareDLLFile: function() {
    var file = this._openFilePicker("Choose Firmware DLL", 
                                    null, 
                                    [{title: "DLL or Zipped DLL", ext: "*.dll;*.zip"},
                                     {title: "DLL Files", ext: "*.dll"},
                                     {title: "ZIP Files", ext: "*.zip"}]);
    if(file) {
      var fileField = document.getElementById("fakepds-firmware-dll-file");
      fileField.file = file;
      
      this._firmwareDLLFile = file;
    }
    
    this._restartHttpServer();
  },
  
  onEnableDITRequest: function() {
    var checkbox = document.getElementById("fakepds-dit-enabled");
    this._enableDITRequest = checkbox.checked;
    
    this._restartHttpServer();
  },
  
  onBrowseDITFile: function() {
    var file = this._openFilePicker("Choose a DIT",
                                    Ci.nsIFilePicker.filterXML);
    if(file) {
      var fileField = document.getElementById("fakepds-dit-file");
      fileField.file = file;
      
      this._ditFile = file;
    }
    
    this._restartHttpServer();
  },
  
  onEnableLogRequest: function() {
    var checkbox = document.getElementById("fakepds-log-request-enabled");
    this._logRequestEnabled = checkbox.checked;
  },
  
  onEnableLogResponse: function() {
    var checkbox = document.getElementById("fakepds-log-response-enabled");
    this._logResponseEnabled = checkbox.checked;
  },
  
  //
  // nsIHttpServerStoppedCallback
  //
  onStopped: function() {
    if(this._httpServerRestart) {
      this._httpServerRestart = false;
      this._startHttpServer();
      return;
    }
    
    if(this._dialogClosing) {
      window.close();
      return;
    }
  },
  
  //
  // nsIHttpRequestHandler
  //
  handle: function(aRequest, aResponseData) {
    this._parseRequest(aRequest, aResponseData);
  },
  
  //
  // Internal Methods
  //
  _startHttpServer: function() {
    const requestPath = "/download_request";
    const firmwareImagePath = "/firmware/image/";
    const firmwareDLLPath = "/firmware/dll/";
    const ditFilePath = "/dit/";
    
    if(this._httpServerRunning || !this._httpServerEnabled) {
      return;
    }
    
    this._httpServer.start(this._httpServerPort);
    this._httpServer.registerPathHandler(requestPath, this);
    
    if(this._enableImageRequest && this._firmwareImageFile) {
      let path = firmwareImagePath + this._firmwareImageFile.leafName;
      this._httpServer.registerFile(path, this._firmwareImageFile);
      this._registeredImagePath = path;
    }
    
    if(this._enableDLLRequest && this._firmwareDLLFile) {
      let path = firmwareDLLPath + this._firmwareDLLFile.leafName;
      this._httpServer.registerFile(path, this._firmwareDLLFile);
      this._registeredDLLPath = path;
    }
    
    if(this._enableDITRequest && this._ditFile) {
      let path = ditFilePath + this._ditFile.leafName;
      this._httpServer.registerFile(path, this._ditFile);
      this._registeredDITPath = path;
    }
    
    this._httpServerRunning = true;
  },
  
  _stopHttpServer: function() {
    if(!this._httpServerRunning) {
      if(this._httpServerRestart) {
        this._httpServerRestart = false;
        this._startHttpServer();
      }
      return;
    }
    
    if(this._registeredImagePath) {
      this._httpServer.registerFile(this._registeredImagePath, null);
      this._registeredImagePath = null;
    }
    
    if(this._registeredDLLPath) {
      this._httpServer.registerFile(this._registeredDLLPath, null);
      this._registeredDLLPath = null;
    }
    
    if(this._registeredDITPath) {
      this._httpServer.registerFile(this._registeredDITPath, null);
      this._registeredDITPath = null;
    }
    
    this._httpServer.stop(this);
    this._httpServerRunning = false;
  },
  
  _restartHttpServer: function() {
    this._httpServerRestart = true;
    this._stopHttpServer();
  },
  
  _overrideServer: function() {
    const overrideServerPref = "songbird.pds.override.url";
    const prefs = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication)
                    .prefs;
    
    if(this._httpServerEnabled) {
      let url = "http://localhost:" + 
                this._httpServerPort + 
                "/download_request";
                
      prefs.setValue(overrideServerPref, url);
    }
    else {
      let pref = prefs.get(overrideServerPref);
      pref.reset();
    }
  },
  
  _openFilePicker: function(aTitle, aFilter, aFilterMap) {
     var filePicker = Cc["@mozilla.org/filepicker;1"]
                        .createInstance(Ci.nsIFilePicker);
    filePicker.init(window, aTitle, Ci.nsIFilePicker.modeOpen);

    if(aFilter) {
      filePicker.appendFilters(aFilter);
    }
    
    if(aFilterMap) {
      for each (let filter in aFilterMap)  {
        filePicker.appendFilter(filter.title, filter.ext);
      }
    }

    if (filePicker.show() == Ci.nsIFilePicker.returnOK) {
      return filePicker.file;
    }
    
    return null;
  },
  
  _parseRequest: function(aRequest, aResponseData) {
    const dataToken = "data=";
    // Only accept POST requests for 'download_request'.
    if(aRequest.method != "POST") {
      return this._invalidRequest(aResponseData);
    }

    var inputStream = Cc["@mozilla.org/scriptableinputstream;1"]
                        .createInstance(Ci.nsIScriptableInputStream);
    inputStream.init(aRequest.bodyInputStream);
    
    // We expect the full request data to be available.
    var bytes = inputStream.available();
    if(!bytes) {
      return this._invalidRequest(aResponseData);
    }
    
    // URI decode the body
    var rawData = inputStream.read(bytes);
    if(rawData.indexOf(dataToken) != 0) {
      return this._invalidRequest(aResponseData);
    }
    
    var uriDecodedData = decodeURIComponent(rawData.substr(dataToken.length));
    
    // Base 64 decode the body
    var decodedData = window.atob(uriDecodedData);
    this._logRequest(decodedData);
    
    // Load body into DOM parser
    var parser = new DOMParser();
    var requestXML = parser.parseFromString(decodedData, "text/xml");
    if(requestXML.documentElement.nodeName != "DownloadRequest") {
      return this._invalidRequest(aResponseData);
    }
    
    // Find the request type
    var requestType = null;
    var children = requestXML.documentElement.childNodes;
    for(let i = 0; i < children.length; i++) {
      let childNode = children.item(i);
      if(childNode.nodeName == "RequestType" &&
         childNode.firstChild.nodeType == Node.TEXT_NODE) {
        requestType = childNode.firstChild.nodeValue;
        break;
      }
    }
    
    if(requestType == null) {
      return this._invalidRequest(aResponseData);
    }

    switch(requestType) {
      case "HF1":
        return this._handleFirmwareImageRequest(requestXML, aResponseData);
      break;
      
      case "HF2":
        return this._handleFirmwareRelNotesRequest(requestXML, aResponseData);
      break;
      
      case "HF3":
        return this._handleFirmwareDLLRequest(requestXML, aResponseData);
      break;
      
      case "HF4":
        return this._handleDITRequest(requestXML, aResponseData);
      break;
      
      default:
        return this._invalidRequest(aResponseData);
    }
  },
  
  _handleFirmwareImageRequest: function(aRequestXML, aResponseData) {
    var pavctn = this._getPAVCTN(aRequestXML);
    var requestVersion = this._getRequestVersion(aRequestXML);
    var offerNewVersion = 
      this._compareVersions(requestVersion, this._firmwareImageVersion) < 0;
    
    if(!this._enableImageRequest || !this._registeredImagePath || !offerNewVersion) {
      let responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      responseXML += "<PDSResponse>\n";
      responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
      responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
      responseXML += "  <DocType>HF1</DocType>\n";
      responseXML += "  <Response>NoDownloadAvailable</Response>\n";
      responseXML += "</PDSResponse>\n";
      
      aResponseData.setStatusLine(null, 200, "OK");
      aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
      this._logResponse(responseXML);
    }
    else if(this._enableImageRequest && this._registeredImagePath && offerNewVersion) {
      let responseURL = this._baseURL + 
                        this._httpServerPort + 
                        this._registeredImagePath;
      
      let responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      responseXML += "<PDSResponse>\n";
      responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
      responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
      responseXML += "  <Response>DownloadAvailable</Response>\n";
      responseXML += "  <DocType>HF1</DocType>\n";
      responseXML += "  <Version>" + this._firmwareImageVersion + "</Version>\n";
      responseXML += "  <DownloadURL>" + responseURL + "</DownloadURL>\n";
      responseXML += "  <DocDesc>Firmware Image</DocDesc>\n";
      responseXML += "  <FileExtn>zip</FileExtn>\n";
      responseXML += "</PDSResponse>\n";
      
      aResponseData.setStatusLine(null, 200, "OK");
      aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
      this._logResponse(responseXML);
    }
    else {
      this._invalidRequest(aResponseData);
    }
  },
  
  _handleFirmwareRelNotesRequest: function(aRequestXML, aResponseData) {
    var pavctn = this._getPAVCTN(aRequestXML);
    var responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    responseXML += "<PDSResponse>\n";
    responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
    responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
    responseXML += "  <DocType>HF2</DocType>\n";
    responseXML += "  <Response>NoDownloadAvailable</Response>\n";
    responseXML += "</PDSResponse>\n";
    
    aResponseData.setStatusLine(null, 200, "OK");
    aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
    this._logResponse(responseXML);
  },
  
  _handleFirmwareDLLRequest: function(aRequestXML, aResponseData) {
    var pavctn = this._getPAVCTN(aRequestXML);
    if(!this._enableDLLRequest || !this._registeredDLLPath) {
      let responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      responseXML += "<PDSResponse>\n";
      responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
      responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
      responseXML += "  <DocType>HF3</DocType>\n";
      responseXML += "  <Response>NoDownloadAvailable</Response>\n";
      responseXML += "</PDSResponse>\n";
      
      aResponseData.setStatusLine(null, 200, "OK");
      aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
      this._logResponse(responseXML);
    }
    else if(this._enableDLLRequest && this._registeredDLLPath) {
      let responseURL = this._baseURL + 
                        this._httpServerPort + 
                        this._registeredDLLPath;
      
      let responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      responseXML += "<PDSResponse>\n";
      responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
      responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
      responseXML += "  <Response>DownloadAvailable</Response>\n";
      responseXML += "  <DocType>HF3</DocType>\n";
      responseXML += "  <Version>" + this._firmwareDLLVersion + "</Version>\n";
      responseXML += "  <DownloadURL>" + responseURL + "</DownloadURL>\n";
      responseXML += "  <DocDesc>Firmware Flashing DLL</DocDesc>\n";
      responseXML += "  <FileExtn>dll</FileExtn>\n";
      responseXML += "</PDSResponse>\n";
      
      aResponseData.setStatusLine(null, 200, "OK");
      aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
      this._logResponse(responseXML);
    }
    else {
      this._invalidRequest(aResponseData);
    }
  },
  
  _handleDITRequest: function(aRequestXML, aResponseData) {
    var pavctn = this._getPAVCTN(aRequestXML);
    if(!this._enableDITRequest || !this._registeredDITPath) {
      let responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      responseXML += "<PDSResponse>\n";
      responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
      responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
      responseXML += "  <DocType>HF4</DocType>\n";
      responseXML += "  <Response>NoDownloadAvailable</Response>\n";
      responseXML += "</PDSResponse>\n";
      
      aResponseData.setStatusLine(null, 200, "OK");
      aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
      this._logResponse(responseXML);
    }
    else if(this._enableDITRequest && this._registeredDITPath) {
      let responseURL = this._baseURL + 
                        this._httpServerPort + 
                        this._registeredDITPath;
      
      let responseXML = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
      responseXML += "<PDSResponse>\n";
      responseXML += "  <InterfaceId>PAV-SB-2K9-01</InterfaceId>\n";
      responseXML += "  <PAVCTN>" + pavctn + "</PAVCTN>\n";
      responseXML += "  <Response>DownloadAvailable</Response>\n";
      responseXML += "  <DocType>HF4</DocType>\n";
      responseXML += "  <Version>1.0</Version>\n";
      responseXML += "  <DownloadURL>" + responseURL + "</DownloadURL>\n";
      responseXML += "  <DocDesc>Device Information Table</DocDesc>\n";
      responseXML += "  <FileExtn>xml</FileExtn>\n";
      responseXML += "</PDSResponse>\n";
      
      aResponseData.setStatusLine(null, 200, "OK");
      aResponseData.bodyOutputStream.write(responseXML, responseXML.length);
      this._logResponse(responseXML);
    }
    else {   
      // Invalid Request!
      this._invalidRequest(aResponseData);
    }
  },
  
  _invalidRequest: function(aResponseData) {
    const invalidRequestResponse = "Invalid Request Method";
    aResponseData.setStatusLine(null, 405, "Method Not Allowed");
    aResponseData.write(invalidRequestResponse);
    this._logResponse(invalidRequestResponse);
  },
  
  _getPAVCTN: function(aRequestXML) {
    var documentElement = aRequestXML.documentElement;
    var deviceVersion = documentElement.getElementsByTagName("DeviceVersion")
                                       .item(0).firstChild.nodeValue;
    var strokeNumber = documentElement.getElementsByTagName("StrokeNumber")
                                      .item(0).firstChild.nodeValue;
    var pavctn = deviceVersion + "/" + strokeNumber;
    
    return pavctn;
  },
  
  _getRequestVersion: function(aRequestXML) {
    var documentElement = aRequestXML.documentElement;
    var currentVersion = documentElement.getElementsByTagName("CurrentVersion")
                                        .item(0).firstChild.nodeValue;
    return currentVersion;
  },
  
  _compareVersions: function(aVersionA, aVersionB) {
    var versionComparator = Cc["@mozilla.org/xpcom/version-comparator;1"]
                              .createInstance(Ci.nsIVersionComparator);
    return versionComparator.compare(aVersionA, aVersionB);
  },
  
  _logRequest: function(aRequestXML) {
    if(this._logRequestEnabled) {
      const console = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication)
                        .console;
      console.log("Fake PDS Request:\n" + aRequestXML);
    }
  },
  
  _logResponse: function(aResponseXML) {
    if(this._logResponseEnabled) {
      const console = Cc["@mozilla.org/fuel/application;1"]
                        .getService(Ci.fuelIApplication)
                        .console;
      console.log("Fake PDS Response:\n" + aResponseXML);
    }
  },
};
