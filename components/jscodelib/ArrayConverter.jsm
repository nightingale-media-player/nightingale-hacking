/*
 * NOTE: This was snagged from https://bugzilla.mozilla.org/show_bug.cgi?id=380839
 * This will go away when this lands
 */

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Code modules: JavaScript array converter.
 *
 * The Initial Developer of the Original Code is
 * Alexander J. Vincent <ajvincent@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/**
 * Utilities for JavaScript components loaded by the JS component
 * loader.
 *
 * Import into a JS component using
 * 'Components.utils.import("rel:ArrayConverter.jsm");'
 *
 */

EXPORTED_SYMBOLS = [ "ArrayConverter" ];

const Ci = Components.interfaces;
const Cr = Components.results;

/**
 * Get a simple enumerator.
 *
 * @param aItemsList JavaScript array to enumerate.
 */
function Enumerator(aItemsList) {
  this._itemsList = aItemsList;
  this._iteratorPosition = 0;
}
Enumerator.prototype = {
  // nsISimpleEnumerator
  hasMoreElements: function hasMoreElements() {
    return (this._iteratorPosition < this._itemsList.length);
  },

  // nsISimpleEnumerator
  getNext: function getNext() {
    if (!(this.hasMoreElements())) {
      throw Components.results.NS_ERROR_FAILURE;
    }
    var type = this._itemsList[this._iteratorPosition];
    this._iteratorPosition++;
    return type;
  },

  // nsISupports
  QueryInterface: function QueryInterface(aIID)
  {
    if (aIID.equals(Ci.nsISimpleEnumerator) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

/**
 * Get a string enumerator.
 *
 * @param aItemsList JavaScript array to enumerate.
 */
function StringEnumerator(aItemsList) {
  this._itemsList = aItemsList;
  this._iteratorPosition = 0;
}
StringEnumerator.prototype = {
  // nsIStringEnumerator
  hasMore: function hasMore() {
    return (this._iteratorPosition < this._itemsList.length);
  },

  // nsIStringEnumerator
  getNext: function getNext() {
    if (!(this.hasMore())) {
      throw Components.results.NS_ERROR_FAILURE;
    }
    var type = this._itemsList[this._iteratorPosition];
    this._iteratorPosition++;
    return type;
  },

  // nsISupports
  QueryInterface: function QueryInterface(aIID)
  {
    if (aIID.equals(Ci.nsIStringEnumerator) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

function NSArray(aItemsList) {
  this._itemsList = aItemsList;
}
NSArray.prototype = {
  // nsIArray
  get length() {
    return this._itemsList.length;
  },

  // nsIArray
  queryElementAt: function queryElementAt(aIndex, aIID) {
    if (aIndex > this.length - 1) {
      throw Components.results.NS_ERROR_ILLEGAL_VALUE;
    }

    return this._itemsList[aIndex].QueryInterface(aIID);
  },

  // nsIArray
  indexOf: function indexOf(aIndex, aElement) {
    for (var i = aIndex; i < this._itemsList.length; i++) {
      if (this._itemsList[i] == aElement) {
        return i;
      }
    }
    throw Components.results.NS_ERROR_NOT_FOUND;
  },

  // nsIArray
  enumerate: function enumerate() {
    return new Enumerator(this._itemsList);
  },

  // nsISupports
  QueryInterface: function QueryInterface(aIID)
  {
    if (aIID.equals(Ci.nsIArray) ||
        aIID.equals(Ci.nsISupports))
      return this;

    throw Components.results.NS_ERROR_NO_INTERFACE;
  }
}

var ArrayConverter = {
  /**
   * Get a JavaScript array for a nsIArray or nsISimpleEnumerator.
   *
   * @param aObject nsIArray or nsISimpleEnumerator to convert.
   *
   * @throws NS_ERROR_INVALID_ARG.
   */
  JSArray: function getJSArray(aObject) {
    if (aObject instanceof Ci.nsIArray) {
      aObject = aObject.enumerate();
    }
    
    var hasMore;
    
    if (aObject instanceof Ci.nsISimpleEnumerator) {
        hasMore = "hasMoreElements";
    } else if (aObject instanceof Ci.nsIStringEnumerator ||
               aObject instanceof Ci.nsIUTF8StringEnumerator)
    {
        hasMore = "hasMore";
    } else {
        throw new Components.Exception("invalid object",
                                       Cr.NS_ERROR_INVALID_ARG);
    }

    var array = [];
    while (aObject[hasMore]()) {
        array.push(aObject.getNext());
    }
    return array;
  },

  /**
   * Get a JavaScript iterator for a nsIArray, nsISimpleEnumerator,
   * or nsIStringEnumerator.
   *
   * @param aObject nsIArray or nsISimpleEnumerator to convert.
   *
   * @throws NS_ERROR_INVALID_ARG.
   */
  JSEnum: function JSEnum(enumIn) {
    if (enumIn instanceof Ci.nsIArray) {
      enumIn = enumIn.enumerate();
    }
    
    if (enumIn instanceof Ci.nsISimpleEnumerator) {
      return {
        __iterator__: function() {
          while(enumIn.hasMoreElements())
            yield(enumIn.getNext());
        }
      }
    }
    else if (enumIn instanceof Ci.nsIStringEnumerator) {
      return {
        __iterator__: function() {
          while(enumIn.hasMore())
            yield(enumIn.getNext());
        }
      }
    }
    return null;
  },

  /**
   * Return a nsIArray for a JavaScript array.
   *
   * @param aArray JavaScript array to convert.
   */
  nsIArray: function get_nsIArray(aArray) {
    return new NSArray(aArray);
  },

  /**
   * Return a nsISimpleEnumerator for a JavaScript array or nsIArray.
   *
   * @param aArray JavaScript array to convert.
   */
  enumerator: function get_enumerator(/* in JSArray */ aArray) {
    if (aArray instanceof Ci.nsIArray) {
      return aArray.nsIArray.enumerate();
    }
    return new Enumerator(aArray);
  },

  /**
   * Return a nsIStringEnumerator for a JavaScript array.
   *
   * @param aArray JavaScript array to convert.
   */
  stringEnumerator: function get_stringEnumerator(/* in JSArray */ aArray) {
    return new StringEnumerator(aArray);
  },

  /**
   * Return a method to convert a JavaScript array into a XPCOM array.
   *
   * @param aArrayName Name of JavaScript array property in "this" to convert.
   */
  xpcomArrayMethod: function getXPCOMMethod(aArrayName) {
    /**
     * Return a XPCOM array and count.
     *
     * @param (out) aCount The length of the array returned.
     *
     * @return The XPCOM array.
     */
    return function xpcomArray(aCount) {
      var array = this[aArrayName];
      aCount.value = array.length;
      var rv = [];
      for (var i = 0; i < array.length; i++) {
        rv[i] = array[i];
      }
      return rv;
    }
  }  
};

