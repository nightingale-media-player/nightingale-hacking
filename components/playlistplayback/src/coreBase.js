/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
 */

/**
 * \file coreBase.js
 * \brief Base class for the sbICoreWrapper implementations
 * \sa sbICoreWrapper.idl coreFlash.js coreWMP.js coreQT.js coreVLC.js
 */

/**
 * \class CoreBase
 * \brief Base class for sbICoreWrapper implementations
 * \sa CoreFlash CoreWMP CoreQT CoreVLC
 */
function CoreBase()
{
};

// Define the class-level data and methods
CoreBase.prototype = 
{
  _object  : null,
  _url     : "",
  _id      : "",
  _playing : false,
  _paused  : false,

  _verifyObject: function () {
    if ( this._object == null ) {
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
      this._object = aObject;
    }
  },

  onSwapCore: function() {
    this._verifyObject();
    try {
      this._object.Stop();
    } catch(err) {
      LOG(err);
    }
  },

  /**
   * \brief Clean up an url.
   * prepends file:// to urls if they do not have '(letters)://'
   * replaces '\\' with '/'
   *
   * \param aURL
   *        An url to sanitize
   */
  sanitizeURL: function (aURL) {
    if (!aURL)
      throw Components.results.NS_ERROR_INVALID_ARG;

    if( aURL.search(/[A-Za-z].*\:\/\//g) < 0 ) {
      aURL = "file://" + aURL;
    }

    if( aURL.search(/\\/g)) {
      aURL = aURL.replace(/\\/, '/');
    }
    return aURL;
  },

  // Debugging helper functions

  /**
   * \brief Helper function to provide output.
   *
   * \param aString
   *        A string to output to the command line.
   * \param aImplName
   *        The name of the derived class calling the method
   */
  LOG: function ( aString, aImplName ) {
    if (!aImplName)
      aImplName = "CoreBase";
    dump("***" + aImplName + " *** " + aString + "\n");
  },

  /**
   * \brief Dump an object to the console.
   * Dumps an object's properties to the console
   *
   * \param   aObj
   *          The object to dump
   * \param   aObjName
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

/**
 * \class ExtensionSchemeMatcher
 * \brief Utility class to easily test a list of extensions and schemes
 *        against a string
 */
function ExtensionSchemeMatcher(aExtensions, aSchemes) {
  this._extensions = aExtensions;
  this._schemes    = aSchemes;
}

ExtensionSchemeMatcher.prototype.match = function(aStr) {

  // Short circuit for the most common case
  if(aStr.lastIndexOf("." + this._extensions[0]) ==
     aStr.length - this._extensions[0].length + 1) {
    return true;
  }

  var extensionSep = aStr.lastIndexOf(".");
  if(extensionSep >= 0) {
    var extension = aStr.substring(extensionSep + 1);
    var rv = this._extensions.some(function(ext) {
      return extension == ext;
    });
    if(rv) {
      return true;
    }
  }

  var schemeSep = aStr.indexOf("://");
  if(schemeSep >= 0) {
    var scheme = aStr.substring(0, schemeSep);
    var rv = this._schemes.some(function(sch) {
      return scheme == sch;
    });
    if(rv) {
      return true;
    }
  }

  return false;
}

function StringArrayEnumerator(aArray) {
  this._array = aArray;
  this._current = 0;
}

StringArrayEnumerator.prototype.hasMore = function() {
  return this._current < this._array.length;
}

StringArrayEnumerator.prototype.getNext = function() {
  return this._array[this._current++];
}
  
StringArrayEnumerator.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.nsIStringEnumerator) &&
      !iid.equals(Components.interfaces.nsISupports))
    throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};

