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
			dump("triggering play\n");
			mashTapeVideo.paused = false;
			gMcMgr.playbackControl.play();
		}
	},

	pauseVideo: function() {
		var obj = document.getElementById("mTFlashObject");
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

	// expand or collapse.  When expanded, we hide the index and the metadata
	// giving the whole pane to the flash video itself
	// we could optionally make the display pane height go full too
	toggleExpand: function(expand) {
		if (expand != null)
			this.expanded = !expand;
		var splitter = window.parent.document
			.getElementById("mashTape-flash-splitter");
		var metadata = document.getElementById("content");
		var img = document.getElementById("toggle-expand-img");
		var dp = window.parent.mashTape.displayPane;

		if (this.expanded) {
			//metadata.style.visibility = "visible";
			splitter.setAttribute("state", "open");
			img.src = "chrome://mashtape/skin/video_expand.png";
			this.expanded = false;
			
			if (this.height != null)
				dp.height = this.height;
		} else {
			//metadata.style.visibility = "collapse";
			splitter.setAttribute("state", "collapsed");
			img.src = "chrome://mashtape/skin/video_collapse.png";
			this.expanded = true;
			
			var mainDoc = Cc['@mozilla.org/appshell/window-mediator;1']
				.getService(Ci.nsIWindowMediator)
				.getMostRecentWindow('Songbird:Main').window.document;
			var tabbrowser = mainDoc.getElementById("content");
			var tabstyle = mainDoc.defaultView.getComputedStyle(tabbrowser, "");

			this.height = dp.height;
			dp.height = parseInt(dp.height) + parseInt(tabstyle.height);
		}
	},
}
