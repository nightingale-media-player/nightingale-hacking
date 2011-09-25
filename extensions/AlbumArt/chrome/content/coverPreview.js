
const DROP_TARGET_IMAGE = "chrome://nightingale/skin/album-art/drop-target.png";
const XLINK = "http://www.w3.org/1999/xlink";

var metadataImage = null;
  
/**
 * \brief onLoad - called when the cover preview window loads.
 */
function onLoad () {
  if (window.arguments.length > 0 &&
      window.arguments[0]) {
    var coverPreviewImage = document.getElementById("coverPreviewImage");
    coverPreviewImage.setAttributeNS(XLINK, "href", window.arguments[0]);
  } else {
    var imageObserver = {
      observe: function(aSubject, aTopic, aData) {
        var coverPreviewImage = document.getElementById("coverPreviewImage");
        if (!aData) { aData = DROP_TARGET_IMAGE; }
        coverPreviewImage.setAttributeNS(XLINK, "href", aData);
      }
    }
    // Setup the dataremote for the now playing image.
    var createDataRemote =  new Components.Constructor(
                                  "@getnightingale.com/Nightingale/DataRemote;1",
                                  Components.interfaces.sbIDataRemote,
                                  "init");
    metadataImage = createDataRemote("metadata.imageURL", null);
    metadataImage.bindObserver(imageObserver, false);
  }
}

/**
 * \brief onUnload - called when the cover preview window unloads.
 */
function onUnload () {
  if (metadataImage) {
    metadataImage.unbind();
  }
}
