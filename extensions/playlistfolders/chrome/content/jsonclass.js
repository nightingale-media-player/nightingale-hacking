/* jsonclass.js
 * A implementation of the JSON object to get JSON support on NG 1.8
 */
// Make a namespace.
if (typeof playlistfolders == 'undefined') {
  var playlistfolders = {};
};

// The following code is a loose adaption of Douglas Crockford's code
// from http://www.json.org/json.js (public domain'd)

// Notable differences:
// * Unserializable values such as |undefined| or functions aren't
//   silently dropped but always lead to a TypeError.
// * An optional key blacklist has been added to JSON.toString

playlistfolders.JSON = {
  /**
   * Converts a JavaScript object into a JSON string.
   *
   * @param aJSObject is the object to be converted
   * @param aKeysToDrop is an optional array of keys which will be
   *                    ignored in all objects during the serialization
   * @return the object's JSON representation
   *
   * Note: aJSObject MUST not contain cyclic references.
   */
  stringify: function JSON_toString(aJSObject, aKeysToDrop) {
    // we use a single string builder for efficiency reasons
    var pieces = [];
    
    // this recursive function walks through all objects and appends their
    // JSON representation (in one or several pieces) to the string builder
    function append_piece(aObj) {
      if (typeof aObj == "string") {
        aObj = aObj.replace(/[\\"\x00-\x1F\u0080-\uFFFF]/g, function($0) {
          // use the special escape notation if one exists, otherwise
          // produce a general unicode escape sequence
          switch ($0) {
          case "\b": return "\\b";
          case "\t": return "\\t";
          case "\n": return "\\n";
          case "\f": return "\\f";
          case "\r": return "\\r";
          case '"':  return '\\"';
          case "\\": return "\\\\";
          }
          return "\\u" + ("0000" + $0.charCodeAt(0).toString(16)).slice(-4);
        });
        pieces.push('"' + aObj + '"')
      }
      else if (typeof aObj == "boolean") {
        pieces.push(aObj ? "true" : "false");
      }
      else if (typeof aObj == "number" && isFinite(aObj)) {
        // there is no representation for infinite numbers or for NaN!
        pieces.push(aObj.toString());
      }
      else if (aObj === null) {
        pieces.push("null");
      }
      // if it looks like an array, treat it as such - this is required
      // for all arrays from either outside this module or a sandbox
      else if (aObj instanceof Array ||
               typeof aObj == "object" && "length" in aObj &&
               (aObj.length === 0 || aObj[aObj.length - 1] !== undefined)) {
        pieces.push("[");
        for (var i = 0; i < aObj.length; i++) {
          arguments.callee(aObj[i]);
          pieces.push(",");
        }
        if (aObj.length > 0)
          pieces.pop(); // drop the trailing colon
        pieces.push("]");
      }
      else if (typeof aObj == "object") {
        pieces.push("{");
        for (var key in aObj) {
          // allow callers to pass objects containing private data which
          // they don't want the JSON string to contain (so they don't
          // have to manually pre-process the object)
          if (aKeysToDrop && aKeysToDrop.indexOf(key) != -1)
            continue;
          
          arguments.callee(key.toString());
          pieces.push(":");
          arguments.callee(aObj[key]);
          pieces.push(",");
        }
        if (pieces[pieces.length - 1] == ",")
          pieces.pop(); // drop the trailing colon
        pieces.push("}");
      }
      else {
        throw new TypeError("No JSON representation for this object!");
      }
    }
    append_piece(aJSObject);
    
    return pieces.join("");
  },

  /**
   * Converts a JSON string into a JavaScript object.
   *
   * @param aJSONString is the string to be converted
   * @return a JavaScript object for the given JSON representation
   */
  parse: function JSON_fromString(aJSONString) {
    if (!this.isMostlyHarmless(aJSONString))
      throw new SyntaxError("No valid JSON string!");
    
    var s = new Components.utils.Sandbox("about:blank");
    return Components.utils.evalInSandbox("(" + aJSONString + ")", s);
  },

  /**
   * Checks whether the given string contains potentially harmful
   * content which might be executed during its evaluation
   * (no parser, thus not 100% safe! Best to use a Sandbox for evaluation)
   *
   * @param aString is the string to be tested
   * @return a boolean
   */
  isMostlyHarmless: function JSON_isMostlyHarmless(aString) {
    const maybeHarmful = /[^,:{}\[\]0-9.\-+Eaeflnr-u \n\r\t]/;
    const jsonStrings = /"(\\.|[^"\\\n\r])*"/g;
    
    return !maybeHarmful.test(aString.replace(jsonStrings, ""));
  }
};
