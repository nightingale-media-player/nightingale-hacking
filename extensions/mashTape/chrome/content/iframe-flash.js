const Cc = Components.classes;
const Ci = Components.interfaces;

if (typeof(gMcMgr) == "undefined")
	var gMcMgr = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
			.getService(Ci.sbIMediacoreManager);

// So not thrilled that I have to hardcode this as YouTube-specific, but
// oh well...
function onYouTubePlayerReady(id) {
	var player = document.getElementById("mTFlashObject");
	player.addEventListener("onStateChange", "mashTapeVideo.youTubeListener");
}

var mashTapeVideo = {
	// Only resume playback if mashTape triggered the pause in the first place
	paused : false,
	
	// Our current expansion state
	expanded: false,
	height: null,

	pauseSongbird: function() {
		if (gMcMgr.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING)
		{
			mashTapeVideo.paused = true;
			gMcMgr.playbackControl.pause();
		}
	},

	resumeSongbird: function() {
		if ((gMcMgr.status.state == Ci.sbIMediacoreStatus.STATUS_PLAYING ||
				gMcMgr.status.state == Ci.sbIMediacoreStatus.STATUS_PAUSED) &&
				mashTapeVideo.paused)
		{
			mashTapeVideo.paused = false;
			gMcMgr.playbackControl.play();
		}
	},

	pauseVideo: function() {
		var obj = document.getElementById("mTFlashObject");
		if (typeof(obj) == "undefined")
			return;
		switch(obj.getAttribute("mashTape-provider")) {
			case "YouTube":
				obj.pauseVideo();
				break;
			case "Yahoo Music":
				obj.vidPause();
				break;
			default:
				return;
		}
	},

	// boooo... hardcoding a YouTube listener
	youTubeListener: function(state) {
		if (state != 0)
			return;
		mashTapeVideo.resumeSongbird();
	},

	// boooo^2, another provider-specific handler... this one for Yahoo
	yahooListener: function(eventType, eventInfo) {
		if (eventType != "done")
			return;
		mashTapeVideo.resumeSongbird();
	},
}
