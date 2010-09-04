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
  
if(typeof Facebook == 'undefined') {
  var Facebook = {};
}

if(typeof(gBrowser) == "undefined") {
  var gBrowser = Cc['@mozilla.org/appshell/window-mediator;1']
                   .getService(Ci.nsIWindowMediator)
                   .getMostRecentWindow('Songbird:Main')
                   .window.gBrowser;
}

if(typeof(SBProperties) == "undefined") {
  Cu.import("resource://app/jsmodules/sbProperties.jsm");
}

Facebook.onLoad = function() {
  this._createFaceplateElements();

  Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager)
    .addListener(this);
    
  this._sequencer = Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
                      .getService(Ci.sbIMediacoreManager).sequencer;
}

Facebook.onUnload = function() {
  Cc['@songbirdnest.com/Songbird/Mediacore/Manager;1']
    .getService(Ci.sbIMediacoreManager)
    .removeListener(this);
  
  this._sequencer = null;
}

Facebook.onLikeClicked = function(aEvent) {
  var curItem = this._sequencer.currentItem;
  var isItemLiked = Facebook.db.isItemLiked(curItem);
  
  if(!isItemLiked) {
    this._postLikeMessage(aEvent, curItem);
  }
  else {
    this._postUnlikeMessage(aEvent, curItem);
  }
}

Facebook.onMediacoreEvent = function(aEvent) {
  switch(aEvent.type) {
    case Ci.sbIMediacoreEvent.TRACK_CHANGE: {
      var mediaItem = aEvent.data;
      if (mediaItem.getProperty(SBProperties.contentType) != "audio") {
        document.getElementById("facebook-faceplate").hidden = true;
      } else {
        document.getElementById("facebook-faceplate").hidden = false;
        Facebook._updateLikeState(mediaItem);
      }
    }
    break;
  }
}

Facebook._createFaceplateElements = function() {
  var faceplateParent = document.getElementById('faceplate-tool-bar');
  
  if(!faceplateParent) {
    return;
  }

  this._faceplate = document.createElement('hbox');
  this._faceplate.setAttribute('id', 'facebook-faceplate');

  faceplateParent.insertBefore(this._faceplate, faceplateParent.firstChild);

  this._faceplateLike = document.createElement('image');
  this._faceplateLike.setAttribute('id', 'facebook-faceplate-like');
  this._faceplateLike.setAttribute('mousethrough', 'never');
  this._faceplateLike.setAttribute('tooltiptext', 'I like this song!');

  var self = this;
  this._faceplateLike.addEventListener('click', function(e) { self.onLikeClicked(e); }, false);

  this._faceplate.appendChild(this._faceplateLike);
}

Facebook._updateLikeState = function(aMediaItem) {
  if(Facebook.db.isItemLiked(aMediaItem)) {
    this._faceplateLike.setAttribute('like', 'like');
  }
  else {
    this._faceplateLike.removeAttribute('like');
  }
}

Facebook._postLikeMessage = function(aEvent, aMediaItem) {
  const defaultLikeMsg = "likes '%s' by '%a'.";
  const defaultCaption = "Listen!";
  const defaultDescription = "Tell me if you like this track too!";
  
  // Grab the current item first. We do this in case
  // the user takes a while to authorize the application
  // which could make the song they are trying to 'like' end
  // before the process is complete.
  var curItem = aMediaItem ? aMediaItem : this._sequencer.currentItem;
  var artistName = curItem.getProperty(SBProperties.artistName);
  var songName = curItem.getProperty(SBProperties.trackName);
  
  // Make sure Songbird is authorized to access the users account.
  Facebook.api.ensureAppAuthentication(aEvent);

  // Replace magic tokens with actual song name and artist name.
  var msg = defaultLikeMsg;
  msg = msg.replace("%s", songName);
  msg = msg.replace("%a", artistName);
  
  // Function we'll use to 'like' the post.
  function likePost(aResult) {
    if(aResult.id) {
      Facebook.api.call("post.likes", { postId: aResult.id });
      Facebook._faceplateLike.setAttribute('like', 'like');
      Facebook.db.likeItem(curItem, aResult.id);
    }
    else {
      // XXXAus: Handle error.
    }
  }
  
  // Lookup the track if we can.
  if(Facebook._canLookupTrack()) {
    function continuePost(aLink) {
      if(aLink) {
        Facebook.api.call("post.feed", 
                          { message: msg, 
                            link: aLink, 
                            caption: defaultCaption, 
                            description: defaultDescription }, 
                          likePost);
      }
      else {
        Facebook.api.call("post.feed", { message: msg }, likePost);
      }
    }
    
    Facebook._lookupTrack(songName, artistName, continuePost);
    
    return;
  }

  // If we can't lookup the track, just do a simple post.
  Facebook.api.call("post.feed", { message: msg }, likePost);
}

Facebook._postUnlikeMessage = function(aEvent, aMediaItem) {
  // Grab the current item first. We do this in case
  // the user takes a while to authorize the application
  // which could make the song they are trying to 'like' end
  // before the process is complete.
  var curItem = aMediaItem ? aMediaItem : this._sequencer.currentItem;
  
  // Make sure Songbird is authorized to access the users account.
  Facebook.api.ensureAppAuthentication(aEvent);

  // Get Post ID in DB
  var likePostId = Facebook.db.getLikePostId(curItem);
  // Unlike track in DB. 
  Facebook.db.unlikeItem(curItem);

  // Unlike the song. This will also delete the message.
  Facebook.api.call("post.delete", { objectId: likePostId }, 
    function() { 
      Facebook._updateLikeState(curItem) 
    });
}

Facebook._canLookupTrack = function() {
  if(Ci.sbILastFmWebServices) {
    return true;
  }
  return false;
}

Facebook._lookupTrack = function(aSongName, aArtistName, aCallback) {
  var lastFm = Cc['@songbirdnest.com/Songbird/webservices/last-fm;1']
                 .getService(Ci.sbILastFmWebServices);

  var args = Cc['@mozilla.org/hash-property-bag;1']
               .createInstance(Ci.nsIWritablePropertyBag);
  args.setProperty('artist', aArtistName);
  args.setProperty('track', aSongName);
  args.setProperty('limit', '1');

  var lastFmCallback = {
    responseReceived: function(aSucceeded, aResponse) {
      var trackMatchUrl = null;
      
      try {
        trackMatchUrl = aResponse.getElementsByTagName('url')
                                 .item(0).firstChild.nodeValue;
      }
      catch(e) {
        Cu.reportError(e);
        trackMatchUrl = null;
      }
      
      aCallback(trackMatchUrl);
    }
  };
  
  lastFm.apiCall('track.search', args, lastFmCallback);
}

window.addEventListener("load", function(e) { Facebook.onLoad(e); }, false);
window.addEventListener("unload", function(e) { Facebook.onUnload(e); }, false);
