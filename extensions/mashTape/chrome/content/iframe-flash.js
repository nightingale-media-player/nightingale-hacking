var mashTapeVideo = {
  // Only resume playback if mT triggered the pause in the first place
  paused: false,
  pauseSongbird: function() {
    if (songbird.playing && !songbird.paused)
    {
      mashTapeVideo.paused = true;
      songbird.pause();
    }
  },

  resumeSongbird: function() {
    if (songbird.paused && mashTapeVideo.paused)
    {
      mashTapeVideo.paused = false;
      songbird.play();
    }
  },
  
  pauseVideo: function() {
    var obj = document.getElementById("mTFlashObject");
    if (!obj)
      return;
    switch(obj.getAttribute("mashTape-provider")) {
      case "YouTube":
        obj.pauseVideo();
        break;
      case "Yahoo Music":
        obj.vidPause();
        break;
      case "MTV Music Video":
        obj.pause();
        break;
      default:
        return;
    }
  },

  // Provider specific listeners
  youTubeListener: function(state) {
    switch (state) {
      case 1: // playing
      case 3: // buffering
        mashTapeVideo.pauseSongbird();
        break;
      case 0: // done playing
      case 2: // paused
        mashTapeVideo.resumeSongbird();
        break;
      default:
        break;
    }
  },
  yahooListener: function(eventType, eventInfo) {
    if (eventType != "done")
      return;
    mashTapeVideo.resumeSongbird();
  },

  mtvListener: function(state) {
    // if no state parameter passed, then this is the MEDIA_ENDED
    // event, and we're done with the video, so resume.
    if (state == null || state == "stopped") {
      mashTapeVideo.resumeSongbird();
    }
  }
}

window.addEventListener("songbirdPlaybackResumed", mashTapeVideo.pauseVideo, false);
window.addEventListener("unload", mashTapeVideo.resumeSongbird, false);

// YouTube is hard-coded to look for a window-level function named
// 'onYouTubePlayerReady' which is invoked with the id of the object
function onYouTubePlayerReady(id) {
  var p = document.getElementById("mTFlashObject");
  p.addEventListener("onStateChange", "mashTapeVideo.youTubeListener");
}
