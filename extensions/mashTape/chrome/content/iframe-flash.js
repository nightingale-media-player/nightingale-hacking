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
  },

  resizeHandler: function(e) {
    e.preventDefault();
    e.stopPropagation();

    var frameWidth = document.getElementsByTagName("html")[0].clientWidth;
    var frameHeight = document.getElementsByTagName("html")[0].clientHeight;
    //dump("New frame size: " + frameWidth + "x" + frameHeight + "\n");

    var swf = document.getElementById("mTFlashObject");
    if (!swf)
      return;

    var newHeight = frameHeight - 25; // 25px is more|less our padding
    var ratio = document.getUserData("mashTapeRatio");
    if (!ratio)
      return;
    var newWidth = newHeight * ratio;
    if (newWidth > frameWidth - 25) {
      newWidth = frameWidth - 25;
      newHeight = newWidth / ratio;
    }

    if (frameWidth - newWidth > 120) {
      document.getElementById("content").style.clear = "none";
      document.getElementById("author").style.display = "block";
    } else {
      document.getElementById("content").style.clear = "both";
      document.getElementById("author").style.display = "inline";
      var capHeight = document.getElementById("content").clientHeight;
      if (frameHeight - newHeight < capHeight) {
        newHeight = newHeight - capHeight;
        newWidth = newHeight * ratio;
        //dump("Adjustment: " + newWidth + "x" + newHeight + "\n");
      }
    }

    if (newWidth != swf.width && newWidth > 150 && newHeight > 50) {
      //dump("Setting SWF size: "+newWidth + "x" + newHeight + "\n");
      swf.width = newWidth;
      swf.height = newHeight;
    }
  }
}

window.addEventListener("songbirdPlaybackResumed", mashTapeVideo.pauseVideo, false);
window.addEventListener("unload", mashTapeVideo.resumeSongbird, false);
window.addEventListener("resize", mashTapeVideo.resizeHandler, false);

// YouTube is hard-coded to look for a window-level function named
// 'onYouTubePlayerReady' which is invoked with the id of the object
function onYouTubePlayerReady(id) {
  var p = document.getElementById("mTFlashObject");
  p.addEventListener("onStateChange", "mashTapeVideo.youTubeListener");
}
