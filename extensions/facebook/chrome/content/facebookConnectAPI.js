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
  
if (typeof Facebook == 'undefined') {
  var Facebook = {};
}

Facebook.api = {
  _appTokenKey: "access.token",
  _appTokenRoot: "facebook.",
  _appTokenRemote: null,
  
  _appAuthDialogUri: "chrome://facebook/content/facebookAuthorize.xul",
  
  _graphApiUri: "https://graph.facebook.com/",
  
  _callQueue: [],
  _busy: false,
  
  _currentCall: null,
  
  _xmlHttpRequest: null,
  
  /*
   * Public entry points for our Facebook API implementation.
   */
  call: function Facebook_api_call(aFunction, aParams, aCallback) {
    this._initHttpRequest();
    this._callQueue.push({ f: aFunction, p: aParams, cb: aCallback });
    this._process();
  },
  
  /*
   * 
   */
  ensureAppAuthentication: function Facebook_api_ensureAppAuthentication(aEvent) {
    const defaultWidth = 480;
    const defaultHeight = 320;
    
    var appToken = this._getAppTokenRemote();
    
    if(!appToken.stringValue || appToken.stringValue == "") {
      var features = "modal,dependent" +
                     ",left=" + (aEvent.screenX - defaultWidth) + 
                     ",top=" + aEvent.screenY;
      window.openDialog(this._appAuthDialogUri, "facebook_authorize", features);
    }
    
    return;
  },
  
  /*
   * nsIDOMEventListener implementation.
   */
  handleEvent: function Facebook_api_handleEvent(aEvent) {
    const COMPLETED = 4;
    if(this._xmlHttpRequest.readyState != COMPLETED) {
      return;
    }
    
    // Parse the result.
    var result = JSON.parse(this._xmlHttpRequest.responseText);
    
    // Callbacks may throw errors so catch all of them and report them.
    try {
      this._currentCall.cb(result);
    }
    catch(e) {
      Cu.reportError(e);
    }

    // We are done with this request, so we aren't busy anymore.
    this._busy = false;
    
    // Check to see if there are more calls to process.
    this._process();
  },
  
  /*
   * Internal Methods
   */
  _getAppTokenRemote: function Facebook_api__getAppTokenRemote() {
    if(!this._appTokenRemote) {
      this._appTokenRemote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                               .createInstance(Ci.sbIDataRemote);
      this._appTokenRemote.init(this._appTokenKey, this._appTokenRoot);
    }
    
    return this._appTokenRemote;
  },
  
  _initHttpRequest: function Facebook_api__initHttpRequest() {
    if(this._xmlHttpRequest) {
      return;
    }
    
    this._xmlHttpRequest = new XMLHttpRequest();
    
    var self = this;
    this._xmlHttpRequest.onreadystatechange = function(aEvent) {
      self.handleEvent(aEvent); 
    };
  },
  
  _process: function Facebook_api__process() {
    // Busy, process will get called again when the current function
    // being run is done.
    if(this._busy) {
      return;
    }
    
    // If there are no calls in the queue we have nothing to do.
    if(!this._callQueue.length) {
      return;
    }
    
    // Ok, we're busy.
    this._busy = true;
    // Get the function from the call queue.
    this._currentCall = this._callQueue.shift();
    
    // Parse the function information to generate the URL we'll
    // use to execute that function.
    var callInfo = null;
    
    try {
      callInfo = this._parseFunc(this._currentCall);
      this._xmlHttpRequest.open(callInfo.method, callInfo.url, true);
      
      // Send form data associated with this call, if available.
      if(callInfo.data) {
        this._xmlHttpRequest.setRequestHeader("Content-type", 
                                              "application/x-www-form-urlencoded");
        this._xmlHttpRequest.send(callInfo.data);
      }
    }
    catch(e) {
      Cu.reportError(e);
      this._busy = false;
      this._currentCall.cb({ isError: true, error: e });
    }
  },
  
  _addAccessToken: function Facebook_api__addAccessToken(aFormData) {
    const ACCESS_TOKEN = "access_token=";
    if(aFormData.length) {
      aFormData += "&";
    }
    
    aFormData += ACCESS_TOKEN + 
                 encodeURIComponent(this._appTokenRemote.stringValue);
                 
    return aFormData;
  },
  
  _addFormData: function Facebook_api__addFormData(aFormData, aKey, aValue) {
    if(aFormData.length) {
      aFormData += "&";
    }
    
    aFormData += aKey + "=";
    aFormData += encodeURIComponent(aValue);
    
    return aFormData;
  },
  
  _parseFunc: function Facebook_api__parseFunc(aObject) {
    switch(aObject.f) {
      case "post.feed": {
        return this._handlePostFeed(aObject);
      }
      break;
      case "post.likes": {
        return this._handlePostLike(aObject);
      }
      break;
      case "post.unlikes": {
        return this._handlePostUnlike(aObject);
      }
      break;
      case "post.delete": {
        return this._handlePostDelete(aObject);
      }
      break;
    }
    
    return { isError: true, error: "Function not supported." };
  },
  
  _handlePostFeed: function Facebook_api__handlePostFeed(aObject) {
    var callUrl = this._graphApiUri;
    
    // Check for a profile id. By default we'll post to the users
    // feed if a profile id is not specified.
    if(aObject.p.profileId) {
      callUrl += aObject.p.profileId;
    }
    else {
      callUrl += "me/";
    }
    
    // We're posting to a feed.
    callUrl += "feed";
    
    // Always add the access token.
    var formData = this._addAccessToken("");

    // Add the message if there is one.
    if(aObject.p.message) {
      formData = this._addFormData(formData, "message", aObject.p.message);
    }
    
    // Add the link if there is one.
    if(aObject.p.link) {
      formData = this._addFormData(formData, "link", aObject.p.link);
    }
    
    // Add the link caption if there is one.
    if(aObject.p.caption) {
      formData = this._addFormData(formData, "caption", aObject.p.caption);
    }
    
    // Add the link description if there is one.
    if(aObject.p.description) {
      formData = this._addFormData(formData, "description", aObject.p.description);
    }
    
    return { method: "POST", url: callUrl, data: formData };
  },
  
  _handlePostLike: function Facebook_api__handlePostLike(aObject) {
    const LIKES_TOKEN = "/likes";
    
    var callUrl = this._graphApiUri;
    if(!aObject.p.postId) {
      throw "post.likes requires 'postId' containing the postId to like!";
    }
    
    callUrl += aObject.p.postId + LIKES_TOKEN;

    // Always add the access token.
    var formData = this._addAccessToken("");
    
    return { method: "POST", url: callUrl, data: formData };
  },
  
  _handlePostUnlike: function Facebook_api__handlePostUnlike(aObject) {
    const LIKES_TOKEN = "/likes";
    
    var callUrl = this._graphApiUri;
    if(!aObject.p.postId) {
      throw "post.unlikes requires 'postId' containing the postId to unlike!";
    }
    
    callUrl += aObject.p.postId + LIKES_TOKEN;
    
    // Always add the access token.
    var formData = this._addAccessToken("");
    
    // XML HTTP Request doesn't support 'DELETE' so we need to pass that
    // method as a query paremeter.
    formData = this._addFormData(formData, "method", "delete");
    
    return { method: "POST", url: callUrl, data: formData };
  },
  
  _handlePostDelete: function Facebook_api__handlePostDelete(aObject) {
    var callUrl = this._graphApiUri;
    if(!aObject.p.objectId) {
      throw "post.delete requires 'objectId' containing the ID of the object to delete!";
    }
    
    callUrl += aObject.p.objectId;
    
    // Always add the access token.
    var formData = this._addAccessToken("");
    
    // XML HTTP Request doesn't support 'DELETE' so we need to pass that
    // method as a query paremeter.
    formData = this._addFormData(formData, "method", "delete");

    return { method: "POST", url: callUrl, data: formData };
  }
};

Facebook.db = {
  _dbGuid: "facebook@db.songbirdnest.com",
  _dbQuery: null,
  
  /*
   * Public Methods
   */
  isItemLiked: function Facebook_db_isItemLiked(aMediaItem) {
    this._initDB();
    
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("SELECT like_post_id FROM user_likes " + 
                           "WHERE library_guid = ? AND item_guid = ?");
    this._dbQuery.bindStringParameter(0, aMediaItem.library.guid);
    this._dbQuery.bindStringParameter(1, aMediaItem.guid);
    this._dbQuery.execute();
    
    var result = this._dbQuery.getResultObject();
    if(result.getRowCount()) {
      return true;
    }
    
    return false;
  },
  
  likeItem: function Facebook_db_likeItem(aMediaItem, aPostId) {
    this._initDB();
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("INSERT INTO user_likes " +
                           "(library_guid, item_guid, like_post_id) " +
                           "VALUES (?, ?, ?)");
    this._dbQuery.bindStringParameter(0, aMediaItem.library.guid);
    this._dbQuery.bindStringParameter(1, aMediaItem.guid);
    this._dbQuery.bindStringParameter(2, aPostId);

    // XXXAus: Handle Error.
    var error = this._dbQuery.execute();
  },
  
  unlikeItem: function Facebook_db_unlikeItem(aMediaItem) {
    this._initDB();
    
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("DELETE FROM user_likes " +
                           "WHERE library_guid = ? AND item_guid = ?");
    this._dbQuery.bindStringParameter(0, aMediaItem.library.guid);
    this._dbQuery.bindStringParameter(1, aMediaItem.guid);
    
    // XXXAus: Handle Error.
    var error = this._dbQuery.execute();
  },
  
  getLikePostId: function Facebook_db_getLikePostId(aMediaItem) {
    this._initDB();
    
    this._dbQuery.resetQuery();
    this._dbQuery.addQuery("SELECT like_post_id FROM user_likes " + 
                           "WHERE library_guid = ? AND item_guid = ?");
    this._dbQuery.bindStringParameter(0, aMediaItem.library.guid);
    this._dbQuery.bindStringParameter(1, aMediaItem.guid);
    
    // XXXAus: Handle Error.
    var error = this._dbQuery.execute();
    
    var result = this._dbQuery.getResultObject();
    if(result.getRowCount()) {
      return result.getRowCell(0, 0);
    }
    
    return "";
  },
  
  /*
   * Internal Methods
   */
  _initDB: function Facebook_db__initDB() {
    if(!this._dbQuery) {
      this._dbQuery = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
                        .createInstance(Ci.sbIDatabaseQuery);
      this._dbQuery.setDatabaseGUID(this._dbGuid);
      this._dbQuery.addQuery("CREATE TABLE IF NOT EXISTS user_likes " +
                             "(library_guid TEXT NOT NULL," +
                             " item_guid TEXT UNIQUE NOT NULL, " +
                             " like_post_id TEXT UNIQUE NOT NULL)");
      this._dbQuery.addQuery("CREATE INDEX IF NOT EXISTS user_likes_index " +
                             "ON user_likes (library_guid, item_guid)");
      var error = this._dbQuery.execute();
      if(error > 0) {
        throw "Facebook DB failed to initialize!";
      }
    }
  }
};
