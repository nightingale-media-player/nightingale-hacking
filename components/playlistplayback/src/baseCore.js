
function BaseCore()
{
};

BaseCore.prototype = 
{
  _object  : null,
  _url     : "",
  _id      : "",
  _playing : false,
  _paused  : false,

  _verifyObject: function () {
    if ((this._object == undefined) || (!this._object)) {
      LOG("VERIFY OBJECT FAILED. OBJECT DOES NOT EXIST");
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    }
  },

  getId : function () {
    return this._id;
  },

  setId : function (aId) {
    if (this._id != aId)
      this._id = aId;
  },

  getObject : function () {
    return this._object;
  },

  setObject : function (aObject) {
    if (this._object != aObject) {
      //
      // swapCore not impl yet
      //if (this._object)
      //  this.swapCore();
      //
      this.LOG("XXXredfive setting _object\n");
      this._object = aObject;
    }
  },

  /**
   * When a swap happens, stop ourself
   */
  onSwappedOut: function () {
    this._verifyObject();
    try {
      this._object.Stop();
    } catch(err) {
      LOG(err);
    }
  },

  /** 
   * prepends file:// to urls if they have a :// in them
   * replaces \\ with /
   */
  sanitizeURL: function (aURL) {
    if (!aURL)
      throw Components.results.NS_ERROR_INVALID_ARG;

    if( aURL.search(/[A-Za-z].*\:\/\//g) < 0 ) {
      aURL = "file://" + aURL;
    }

    if( aURL.search(/\\/g)) {
      aURL.replace(/\\/, '/');
    }
    return aURL;
  },

  /**
   * Converts seconds to a time string
   * @param   seconds
   *          The number of seconds to convert
   * @return  A string containing the converted time (HH:MM:SS)
   */
  emitSecondsToTimeString: function (aSeconds) {
    if ( aSeconds < 0 )
      return "00:00";
    aSeconds = parseFloat( aSeconds );
    var minutes = parseInt( aSeconds / 60 );
    aSeconds = parseInt( aSeconds ) % 60;
    var hours = parseInt( minutes / 60 );
    if ( hours > 50 ) // lame
      return "Error";
    minutes = parseInt( minutes ) % 60;
    var text = ""
    if ( hours > 0 ) {
      text += hours + ":";
    }
    if ( minutes < 10 ) {
      text += "0";
    }
    text += minutes + ":";
    if ( aSeconds < 10 ) {
      text += "0";
    }
    text += aSeconds;
    return text;
  },

  // Debugging helper functions

  /**
   * Helper function to provide output.
   */
  LOG: function (aImplName, aString) {
    dump("***" + aImplName + "*** " + aString + "\n");
  },

  /**
   * Dumps an object's properties to the console
   * @param   obj
   *          The object to dump
   * @param   objName
   *          A string containing the name of obj
   */
  listProperties: function (aObj, aObjName) {
    var columns = 1;
    var count = 0;
    var result = "";
    for (var i in aObj) {
      result += objName + "." + i + " = " + obj[i] + "\t\t\t";
      count = ++count % columns;
      if ( count == columns - 1 )
        result += "\n";
    }
    LOG("listProperties");
    dump(result + "\n");
  }
};

