if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");

function LFFPD(m) {
  dump('last.fm faceplate debug: '+m+'\n');
}

var sbLastFmFaceplate = {
  'stationPref': null,
};

function sbLastFmFaceplate_stationPref_change() {
  //LFFPD('pref change!');
  sbLastFmFaceplate.stationChanged();
}
function sbLastFmFaceplate_requestPref_change() {
  //LFFPD('requesting more tracks pref change!');
  sbLastFmFaceplate.requestChanged();
}

sbLastFmFaceplate.init = 
function sbLastFmFaceplate_init() {
  var stationIcon = document.getElementById("lastfm-station-icon");
  stationIcon.style.visibility = "collapse";
  this.stationPref = Application.prefs.get('songbird.lastfm.radio.station');
  this.stationPref.events.addListener('change', 
      sbLastFmFaceplate_stationPref_change);
  this.requestPref = Application.prefs.get('songbird.lastfm.radio.requesting');
  this.requestPref.events.addListener('change', 
      sbLastFmFaceplate_requestPref_change);

  this._service = Components.classes['@songbirdnest.com/lastfm;1']
    .getService().wrappedJSObject
  // catch the case of a Feather change
  if (this._service.station_name && this._service.station_name != '') {
    Application.prefs.setValue('songbird.lastfm.radio.station',
        this._service.station_name);
  }

  this._remotePreviousDisabled =
      SB_NewDataRemote( "playlist.previous.disabled", null );
  this._remoteNextDisabled =
      SB_NewDataRemote( "playlist.next.disabled", null );
  this._remoteShuffleDisabled =
      SB_NewDataRemote( "playlist.shuffle.disabled", null );
  this._remoteRepeatDisabled =
      SB_NewDataRemote( "playlist.repeat.disabled", null );

}

sbLastFmFaceplate.fini = 
function sbLastFmFaceplate_fini() {
  this.stationPref.events.removeListener('change', 
      sbLastFmFaceplate_stationPref_change);
  this.stationPref = null;
  this.requestPref.events.removeListener('change', 
      sbLastFmFaceplate_requestPref_change);
  this.requestPref = null;
  Application.prefs.setValue('songbird.lastfm.radio.station', '');
  Application.prefs.setValue('songbird.lastfm.radio.requesting', "0");

  if(this._remotePreviousDisabled) {
    this._remotePreviousDisabled.unbind();
    this._remotePreviousDisabled = null;
  }
  if(this._remoteNextDisabled) {
    this._remoteNextDisabled.unbind();
    this._remoteNextDisabled = null;
  }
  if(this._remoteShuffleDisabled) {
    this._remoteShuffleDisabled.unbind();
    this._remoteShuffleDisabled = null;
  }
  if(this._remoteRepeatDisabled) {
    this._remoteRepeatDisabled.unbind();
    this._remoteRepeatDisabled = null;
  }
}

sbLastFmFaceplate.stationChanged =
function sbLastFmFaceplate_stationChanged() {
  LFFPD('stationChanged');
  var stationIcon = document.getElementById("lastfm-station-icon");
  if (this.stationPref.value == '') {
    // hide the radio icon from the faceplate
    stationIcon.style.visibility = "collapse";
  } else {
    // disable back, shuffle and repeat controls
    this._remotePreviousDisabled.boolValue = true;
    this._remoteShuffleDisabled.boolValue = true;
    this._remoteRepeatDisabled.boolValue = true;

    // show the radio icon from the faceplate
    stationIcon.style.visibility = "visible";
  }
}


sbLastFmFaceplate.requestChanged =
function sbLastFmFaceplate_requestChanged() {
  LFFPD('requestChanged');
  if (this.requestPref.value == "1") {
	  // requesting more tracks, disable the next track button
    this._remoteNextDisabled.boolValue = true;
  } else {
	// enable the next track button
    this._remoteNextDisabled.boolValue = false;
  }
}

window.addEventListener('load', function () { sbLastFmFaceplate.init() ;}, false);
window.addEventListener('unload', function () { sbLastFmFaceplate.fini() ;}, false);
