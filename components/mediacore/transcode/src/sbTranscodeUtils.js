/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

const IOFLAGS_DEFAULT = -1;
const PERMISSIONS_DEFAULT = -1;
const FLAGS_DEFAULT = 0;

const PR_RDONLY = 0x01;
const PR_IRUSR = 0400;
const PR_IWUSR = 0200;

const BUFFERSIZE = 1024;

function TranscodeBatchJob() {
}

TranscodeBatchJob.prototype = {
  classDescription: "Songbird batch transcoding job",
  classID:          Components.ID("{53cca1c6-03b1-475f-960e-adb99d9b742f}"),
  contractID:       "@songbirdnest.com/Songbird/Transcode/BatchJob;1",
  QueryInterface:   XPCOMUtils.generateQI(
          [Ci.sbITranscodeBatchJob,
           Ci.sbIJobProgress,
           Ci.sbIJobCancelable,
           Ci.sbIJobProgressTime,
           Ci.sbIMediacoreEventTarget,
           Ci.sbIMediacoreEventListener]),

  _profile         : null,
  _imageProfile    : null,
  _numSimultaneous : 1,
  _items           : [],

  _nextIndex       : 0,
  _itemsCompleted  : 0,
  _jobs            : [],

  _errors          : [],
  _joblisteners    : [],
  _jobStatus       : Ci.sbIJobProgress.STATUS_RUNNING,

  _statusText      : SBString("transcode.batch.running"),
  _titleText       : SBString("transcode.batch.title"),

  _startTime       : -1,

  _eventlisteners  : [],

  // sbITranscodeBatch implementation

  set profile(aProfile) {
    this._profile = aProfile;
  },

  get profile() {
    return this._profile;
  },

  set imageProfile(aImageProfile) {
    this._imageProfile = aImageProfile;
  },

  get imageProfile() {
    return this._imageProfile;
  },

  set items(aItems) {
    this._items = ArrayConverter.JSArray(aItems);
  },

  set items(aItems) {
    this._items = ArrayConverter.JSArray(aItems);
  },

  get items() {
    return ArrayConverter.nsIArray(this._items);
  },

  set destinationFiles(aFiles) {
    this._files = ArrayConverter.JSArray(aFiles);
  },

  get destinationFiles() {
    return ArrayConverter.nsIArray(this._files);
  },

  set concurrentTranscodes(aConcurrentTranscodes) {
    this._numSimultaneous = aConcurrentTranscodes;
  },

  get concurrentTranscodes() {
    return this._numSimultaneous;
  },

  transcodeBatch: function TranscodeBatchJob_transcodeBatch() {
    if (this._items.length == 0)
      throw new Error("No items to transcode");
    if (this._profile == null)
      throw new Error("No profile set");

    this._nextIndex = 0;
    this._startTime = Date.now();

    for (var i = 0; i < this._numSimultaneous; i++) {
      this._nextTranscode();
    }
  },

  // sbIJobProgress implementation 

  get status() {
    return this._jobStatus;
  },

  get statusText() {
    return this._statusText;
  },

  get titleText() {
    return this._titleText;
  },

  get total() {
    // We scale to 1000 because the job progress dialog doesn't work well
    // with large or small numbers.
    return 1000;
  },

  get progress() {
    // Calculate fraction of all current jobs completed. Note that this will
    // give inaccurate representation of progress if some items and much
    // longer/shorter than others. In practice, it should be ok though.
    var currentCompleted = 0;
    for (var i = 0; i < this._jobs.length; i++) {
      try {
        currentCompleted += this._jobs[i].progress / this._jobs[i].total;
      } catch (e) {
        // Fine for this to fail; just means the job didn't implement the
        // interface or couldn't answer.
      }
    }
    return 1000 * 
        (this._itemsCompleted + currentCompleted) / this._items.length;
  },

  get errorCount() {
    return this._errors.length;
  },

  getErrorMessages : function TranscodeBatchJob_getErrorMessages() {
    return ArrayConverter.stringEnumerator(this._errors);
  },

  addJobProgressListener :
      function TranscodeBatchJob_addJobProgressListener(aListener) 
  {
    this._joblisteners.push(aListener);
  },

  removeJobProgressListener :
      function TranscodeBatchJob_removeJobProgressListener(aListener) 
  {
    var index = this._joblisteners.indexOf(aListener);

    if (index >= 0) {
      this._joblisteners.splice(index, 1);
    }
  },

  // sbIJobProgressTime implementation

  get elapsedTime() {
    if (this._startTime < 0)
      return -1;

    return Date.now() - this._startTime;
  },

  get remainingTime() {
    var fractionDone = this.progress / this.total;

    return this.elapsedTime / fractionDone;
  },

  // sbIJobCancelable implementation 

  get canCancel() {
    var can = true;

    for (var i = 0; i < this._jobs.length; i++) {
      can = can && this._jobs[i].canCancel;
    }

    return can;
  },

  cancel : function() {
    if (!this.canCancel)
      throw Components.results.NS_ERROR_FAILURE;

    for (var i = 0; i < this._jobs.length; i++) {
      this._jobs[i].cancel();
    }
  },

  // sbIMediacoreEventTarget implementation
  dispatchEvent : function TranscodeBatchJob_dispatchEvent(aEvent, aAsync)
  {
    // Iterate over a copy of the listeners array to avoid problems with
    // removal while we're iterating.
    var listeners = [].concat(this._eventlisteners);
    for (var i = 0; i < listeners.length; i++) {
      try {
        listeners[i].onMediacoreEvent(aEvent);
      }
      catch (e) {
        Cu.reportError(e);
      }
    }
  },

  addListener : function TranscodeBatchJob_addListener(aListener)
  {
    this._eventlisteners.push(aListener);
  },

  removeListener :
      function TranscodeBatchJob_removeListener(aListener) 
  {
    var index = this._eventlisteners.indexOf(aListener);

    if (index >= 0) {
      this._eventlisteners.splice(index, 1);
    }
  },

  // sbIMediacoreEventListener implementation
  onMediacoreEvent : function TranscodeBatchJob_onMediacoreEvent(aEvent)
  {
    // Just forward to our listeners.
    this.dispatchEvent(aEvent, false);
  },

  // internals

  _updateListeners : function TranscodeBatchJob__updateListeners() {
    // Iterate over a copy of the listeners array to avoid problems with
    // removal while we're iterating.
    var listeners = [].concat(this._joblisteners);
    for (var i = 0; i < listeners.length; i++) {
      try {
        listeners[i].onJobProgress(this);
      }
      catch (e) {
        Cu.reportError(e);
      }
    }
  },

  // Start the next transcode job, if any
  _nextTranscode : function TranscodeBatchJob__nextTranscode() {
    if (this._nextIndex < this._items.length) {
      var index = this._nextIndex++;

      try {
        this._startTranscode(
                this._items[index].QueryInterface(Ci.sbITranscodeBatchJobItem));
      } catch (e) {
        // Go onto the next media item after reporting the error
        this._errors.push(SBString("transcode.file.notsupported"));
        this._itemDone();
      }
    }
  },

  // Called after a job completes. 
  _itemDone : function TranscodeBatchJob__itemDone(aJob) {
    this._itemsCompleted++;

    // Remove job if we have it here
    if (aJob) {
      var index = this._jobs.indexOf(aJob);
      if (index >= 0)
        this._jobs.splice(index, 1);

      // Get errors from the job; append to our error array.
      if (aJob.errorCount > 0) {
        var errors = ArrayConverter.JSArray(aJob.getErrorMessages());
        this._errors = this._errors.concat(errors);
      }
    }

    if (this._itemsCompleted == this._items.length) {
      // We're done. Update job status, etc.
      if (this._errors.length > 0)
        this._jobStatus = Ci.sbIJobProgress.STATUS_FAILED;
      else
        this._jobStatus = Ci.sbIJobProgress.STATUS_SUCCEEDED;

      this._statusText = SBString("transcode.batch.complete");
    }
    else {
      this._nextTranscode();
    }

    // Send status updates to listeners
    this._updateListeners();
  },

  _startTranscode : function TranscodeBatchJob__startTranscode(aItem)
  {
    var manager = Cc["@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1"]
                  .getService(Ci.sbITranscodeManager);
    var ioServ = Components.classes["@mozilla.org/network/io-service;1"]
                 .getService(Components.interfaces.nsIIOService);
    var fph = ioServ.getProtocolHandler("file")
              .QueryInterface(Components.interfaces.nsIFileProtocolHandler);

    var mediaItem = aItem.mediaItem;
    var transcoder = manager.getTranscoderForMediaItem(
            mediaItem, this._profile);

    if (transcoder == null) {
      this._errors.push(SBString("transcode.file.notsupported"));
      this._itemDone(null);
    }

    var uri = mediaItem.contentSrc;
    var destUri = fph.newFileURI(aItem.destinationFile);

    transcoder.profile = this._profile;
    transcoder.sourceURI = uri.spec;
    transcoder.destURI = destUri.spec;
    transcoder.metadata = mediaItem.getProperties();
    transcoder.metadataImage = null;

    var imageURLspec = mediaItem.getProperty(SBProperties.primaryImageURL);
    if (imageURLspec) {
      var ioservice = Cc["@mozilla.org/network/io-service;1"].
          getService(Ci.nsIIOService);
      var uri = ioservice.newURI(imageURLspec, null, null);
      if (uri.schemeIs("resource")) {
        var resourceProtocolHandler = ioservice.getProtocolHandler("resource");
        imageURLspec = resourceProtocolHandler.resolveURI(uri);
      }

      var fileProtocolHandler = ioservice.getProtocolHandler("file");
      var file = fileProtocolHandler.getFileFromURLSpec(imageURLspec);

      if (this._imageProfile != null) {
        var imageTranscoder =
            Cc["@songbirdnest.com/Songbird/Transcode/Image;1"].
            createInstance(Ci.sbITranscodeImage);
        var mimeService = Cc["@mozilla.org/mime;1"].
            getService(Ci.nsIMIMEService);

        var srcMimeType = mimeService.getTypeFromFile(file);
        var destMimeType = this._imageProfile.mimetype;
        var destWidth = this._imageProfile.width;
        var destHeight = this._imageProfile.height;

        transcoder.metadataImage = imageTranscode.transcodeImage(
                file, srcMimeType, destMimeType, destWidth, destHeight);
      }
      else {
        // No transcoding profile; just provide the file as-is.
        var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                          .createInstance(Ci.nsIFileInputStream);
        inputStream.init(file,
                IOFLAGS_DEFAULT, PERMISSIONS_DEFAULT, FLAGS_DEFAULT);
        transcoder.metadataImage = inputStream;
      }
    }

    var self = this;

    var listener = {
      onJobProgress : function(job) {
        if (job.status != Components.interfaces.sbIJobProgress.STATUS_RUNNING)
        {
          // Then it's done!
          job.removeJobProgressListener(this);
          job.removeListener(self);

          self._itemDone(job);
        }
        self._updateListeners();
      }
    };

    transcoder.addJobProgressListener(listener);
    transcoder.addListener(this);

    this._jobs.push(transcoder);

    transcoder.transcode();
  }
}

function TranscodeProfileLoader() {
}

TranscodeProfileLoader.prototype = {
  classDescription: "Songbird transcoding profile loader service",
  classID:          Components.ID("{ebd4a7f5-5bec-42b7-9d39-8de8c9112373}"),
  contractID:       "@songbirdnest.com/Songbird/Transcode/ProfileLoader;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbITranscodeProfileLoader]),
  
  /**
   * \brief Load a profile from the file
   */
  loadProfile: function TranscodeProfileLoader_loadProfile(aFile) {
    var domParser = Cc["@mozilla.org/xmlextras/domparser;1"]
                      .createInstance(Ci.nsIDOMParser);
    var fileStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);

    fileStream.init(aFile, IOFLAGS_DEFAULT, PERMISSIONS_DEFAULT, FLAGS_DEFAULT);

    var doc = domParser.parseFromStream(fileStream,
                                        null,
                                        fileStream.available(),
                                        "text/xml");
    fileStream.close();

    var profile = {
      QueryInterface: XPCOMUtils.generateQI(
          [Components.interfaces.sbITranscodeProfile]),

      id              : null,
      description     : null,
      type            : 0,
      containerFormat : null,
      audioCodec      : null,
      videoCodec      : null,

      containerProperties : null,
      audioProperties     : null,
      videoProperties     : null
    }

    var element = doc.documentElement;


    // The document element should be 'profile'. Check this.
    if (element.localName != "profile")
      throw new Error("Not a profile");

     var children = element.childNodes;
     for (var i = 0; i < children.length; i++) 
     {
       if (children[i].localName == "type") {
         profile.type = this.getType(children[i]);
       }
       else if (children[i].localName == "description") {
         profile.description = children[i].textContent;
       }
       else if (children[i].localName == "id") {
         profile.id = children[i].textContent;
       }
       else if (children[i].localName == "container") {
         this.processContainer(profile, children[i]);
       }
       else if (children[i].localName == "audio") {
         this.processAudioCodec(profile, children[i]);
       }
       else if (children[i].localName == "video") {
         this.processVideoCodec(profile, children[i]);
       }
     }

    return profile;
  },

  getType : function TranscodeProfileLoader_getType(aTypeNode) {
    var type = aTypeNode.textContent;
    if (type == "audio")
      return Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO;
    else if (type == "video")
      return Ci.sbITranscodeProfile.TRANSCODE_TYPE_VIDEO;
    else if (type == "audio+video")
      return Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO_VIDEO;
    else
      throw new Error("Bad type: "+type);
  },

  processProperty : 
      function TranscodeProfileLoader_processProperty(aPropertyNode) 
  {
    var property = {
      QueryInterface: XPCOMUtils.generateQI(
        [Components.interfaces.sbITranscodeProfileProperty]),

      propertyName : aPropertyNode.getAttribute("name"),
    }

    var type = aPropertyNode.getAttribute("type");
      
    if (type == "int") {
      property.valueMin = parseInt(aPropertyNode.getAttribute("min"));
      property.valueMax = parseInt(aPropertyNode.getAttribute("max"));
      property.value = parseInt(aPropertyNode.getAttribute("default"));
    }
    else
      throw new Error("Type " + type + " not supported");

    return property;
  },

  processContainer :
      function TranscodeProfileLoader_processContainer(aProfile, aContainer)
  {
    var properties = [];
    var children = aContainer.childNodes;
    for (var i = 0; i < children.length; i++) 
    {
      if (children[i].localName == "type")
         aProfile.containerFormat = children[i].textContent; 
      else if (children[i].localName == "property")
        properties.push(this.processProperty(children[i]));
    }

    aProfile.containerProperties = ArrayConverter.nsIArray(properties);
  },

  processAudioCodec :
      function TranscodeProfileLoader_processAudioCodec(aProfile, aAudioCodec)
  {
    var properties = [];
    var children = aAudioCodec.childNodes;
    for (var i = 0; i < children.length; i++) 
    {
      if (children[i].localName == "type")
         aProfile.audioCodec = children[i].textContent; 
      else if (children[i].localName == "property")
        properties.push(this.processProperty(children[i]));
    }

    aProfile.audioProperties = ArrayConverter.nsIArray(properties);
  },

  processVideoCodec :
      function TranscodeProfileLoader_processVideoCodec(aProfile, aVideoCodec)
  {
    var properties = [];
    var children = aVideoCodec.childNodes;
    for (var i = 0; i < children.length; i++) 
    {
      if (children[i].localName == "type")
         aProfile.videoCodec = children[i].textContent; 
      else if (children[i].localName == "property")
        properties.push(this.processProperty(children[i]));
    }

    aProfile.videoProperties = ArrayConverter.nsIArray(properties);
  }
} // TranscodeProfileLoader.prototype

function TranscodeImage() {
}

TranscodeImage.prototype = {
  classDescription: "Songbird still image transcoder",
  classID:          Components.ID("{a7ee65ee-7fbf-4a40-ad5e-609e8a10646e}"),
  contractID:       "@songbirdnest.com/Songbird/Transcode/Image;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbITranscodeImage]),
  
  transcodeImage : function TranscodeImage_transcodeImage(
          aInputFile, aInputMimeType, aOutputMimeType, aWidth, aHeight)
  {
    var imgtools = Cc["@mozilla.org/image/tools;1"].getService(Ci.imgITools);

    var fileStream = Cc["@mozilla.org/network/file-input-stream;1"].
        createInstance(Ci.nsIFileInputStream);
    fileStream.init(aInputFile, PR_RDONLY, PR_IRUSR | PR_IWUSR, 0600, 0);

    var inputStream = Cc["@mozilla.org/network/buffered-input-stream;1"].
        createInstance(Ci.nsIBufferedInputStream);
    bis.init(fileStream, BUFFERSIZE);

    var outParam = { value: null };
    imgtools.decodeImageData(inputStream, aInputMimeType, outParam);
    var container = outParam.value;

    return imgtools.encodeScaledImage(container, aOutputMimeType,
            aWidth, aHeight);
  }
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule(
          [TranscodeProfileLoader,
           TranscodeBatchJob,
           TranscodeImage]);
}

