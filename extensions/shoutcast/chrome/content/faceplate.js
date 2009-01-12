if (typeof Cc == 'undefined')
	var Cc = Components.classes;
if (typeof Ci == 'undefined')
	var Ci = Components.interfaces;
if (typeof Cu == 'undefined')
	var Cu = Components.utils;

if (typeof(gMM) == "undefined")
	var gMM = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
		.getService(Ci.sbIMediacoreManager);

if (typeof(SBProperties) == "undefined") {
	Cu.import("resource://app/jsmodules/sbProperties.jsm");
	if (!SBProperties)
		throw new Error("Import of sbProperties module failed");
}

var sbShoutcastFaceplate = {
  'stationPref': null,
  'disableTags': ['sb-player-back-button', 'sb-player-shuffle-button', 
      'sb-player-repeat-button', 'sb-player-forward-button'],
};

sbShoutcastFaceplate.onMediacoreEvent = function(e) {
	var item = e.data;
	switch (e.type) {
		case Ci.sbIMediacoreEvent.VIEW_CHANGE:
			if (gMM.sequencer.view == null)
				return;
			var list = gMM.sequencer.view.mediaList;
			var stationIcon = document.getElementById("shoutcast-station-icon");
			var stopButton = document.getElementById("play_stop_button");
			var playButton = document.getElementById("play_pause_button");
			if (list.getProperty(SBProperties.customType) ==
					"radio_tempStreamList")
			{
				// we're playing a SHOUTcast station
				stationIcon.style.visibility = "visible";
				for (var i in sbShoutcastFaceplate.disableTags) {
					var elements = document.getElementsByTagName(
										sbShoutcastFaceplate.disableTags[i]);
					for (var j=0; j<elements.length; j++) {
						elements[j].setAttribute('disabled', 'true');
					}
				}
				playButton.setAttribute("hidden", "true");
				stopButton.removeAttribute("hidden");
			} else {
				sbShoutcastFaceplate.resetButtons();
			}
			break;
		case Ci.sbIMediacoreEvent.STREAM_END:
		case Ci.sbIMediacoreEvent.STREAM_STOP:
			sbShoutcastFaceplate.resetButtons();
			break;
	}
}
	
sbShoutcastFaceplate.resetButtons = function(e) {
	var stationIcon = document.getElementById("shoutcast-station-icon");
	var stopButton = document.getElementById("play_stop_button");
	var playButton = document.getElementById("play_pause_button");

	// we're playing something else
	stationIcon.style.visibility = "collapse";
	for (var i in sbShoutcastFaceplate.disableTags) {
		var elements = document.getElementsByTagName(
							sbShoutcastFaceplate.disableTags[i]);
		for (var j=0; j<elements.length; j++) {
			elements[j].removeAttribute('disabled');
		}
	}
	stopButton.setAttribute("hidden", "true");
	playButton.removeAttribute("hidden");
}

window.addEventListener('load', function () {
	gMM.addListener(sbShoutcastFaceplate);
}, false);

window.addEventListener('unload', function () {
	gMM.removeListener(sbShoutcastFaceplate);
}, false);
