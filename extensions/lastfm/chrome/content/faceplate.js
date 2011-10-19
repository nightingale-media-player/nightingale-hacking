function LFFPD(m) {
  dump('last.fm faceplate debug: '+m+'\n');
}

var sbLastFmFaceplate = {
  'stationPref': null,
  'disableTags': ['sb-player-back-button', 'sb-player-shuffle-button', 
      'sb-player-repeat-button'],
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
  this.stationPref = Application.prefs.get('nightingale.lastfm.radio.station');
  this.stationPref.events.addListener('change', 
      sbLastFmFaceplate_stationPref_change);
  this.requestPref = Application.prefs.get('nightingale.lastfm.radio.requesting');
  this.requestPref.events.addListener('change', 
      sbLastFmFaceplate_requestPref_change);

  this.listenerBound = false;

  this._service = Components.classes['@getnightingale.com/lastfm;1']
    .getService().wrappedJSObject
  // catch the case of a Feather change
  if (this._service.station_name && this._service.station_name != '') {
    Application.prefs.setValue('nightingale.lastfm.radio.station',
        this._service.station_name);
  }
}

sbLastFmFaceplate.fini = 
function sbLastFmFaceplate_fini() {
  this.stationPref.events.removeListener('change', 
      sbLastFmFaceplate_stationPref_change);
  this.stationPref = null;
  this.requestPref.events.removeListener('change', 
      sbLastFmFaceplate_requestPref_change);
  this.requestPref = null;
  Application.prefs.setValue('nightingale.lastfm.radio.station', '');
  Application.prefs.setValue('nightingale.lastfm.radio.requesting', "0");

  // Remove mediacore listener
  if (this.listenerBound) {
    Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
      .getService(Ci.sbIMediacoreManager)
      .removeListener(sbLastFmFaceplate);
  }
}

sbLastFmFaceplate.stationChanged =
function sbLastFmFaceplate_stationChanged() {
  LFFPD('stationChanged');
  var stationIcon = document.getElementById("lastfm-station-icon");
  if (this.stationPref.value == '') {
    for (var i in this.disableTags) {
      var elements = document.getElementsByTagName(this.disableTags[i]);
      for (var j=0; j<elements.length; j++) {
        elements[j].removeAttribute('disabled');
      }
    }
    // hide the radio icon from the faceplate
    stationIcon.style.visibility = "collapse";
	
    // Remove mediacore listener
    Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
			.getService(Ci.sbIMediacoreManager)
			.removeListener(sbLastFmFaceplate);
    sbLastFmFaceplate.listenerBound = false;
  } else {
    for (var i in this.disableTags) {
      var elements = document.getElementsByTagName(this.disableTags[i]);
      for (var j=0; j<elements.length; j++) {
        elements[j].setAttribute('disabled', 'true');
      }
    }
    // If this has the disabled attribute (e.g. coming from SHOUTcast), then
    // remove it
    document.getElementsByTagName("sb-player-forward-button")[0]
      .removeAttribute('disabled');

    // show the radio icon from the faceplate
    stationIcon.style.visibility = "visible";

    // Add mediacore listener
    Cc['@getnightingale.com/Nightingale/Mediacore/Manager;1']
        .getService(Ci.sbIMediacoreManager)
        .addListener(sbLastFmFaceplate);

    sbLastFmFaceplate.listenerBound = true;
  }
}


sbLastFmFaceplate.requestChanged =
function sbLastFmFaceplate_requestChanged() {
  LFFPD('requestChanged');
  var b = document.getElementsByTagName('sb-player-forward-button')[0];
  if (this.requestPref.value == "1") {
	  // requesting more tracks, disable the next track button
      b.setAttribute('disabled', 'true');
  } else {
	// enable the next track button
      b.removeAttribute('disabled');
  }
}

sbLastFmFaceplate.onMediacoreEvent =
function sbLastFmFaceplate_onMediacoreEvent(aEvent) {
	switch(aEvent.type) {
		case Ci.sbIMediacoreEvent.TRACK_CHANGE:
			document.getElementsByTagName('sb-player-back-button')[0]
				.setAttribute("disabled", "true");
			break;
		default:
			break;
	}
}

window.addEventListener('load', function () { sbLastFmFaceplate.init() ;}, false);
window.addEventListener('unload', function () { sbLastFmFaceplate.fini() ;}, false);
