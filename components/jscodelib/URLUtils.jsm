/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file  URLUtils.jsm
 * \brief Javascript source for the URL utility services.
 */

//------------------------------------------------------------------------------
//
// URL utility JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = [ "URLUtils" ];


//------------------------------------------------------------------------------
//
// URL utility defs.
//
//------------------------------------------------------------------------------

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results
const Cu = Components.utils


//------------------------------------------------------------------------------
//
// URL utility services.
//
//------------------------------------------------------------------------------

var URLUtils = {
  //----------------------------------------------------------------------------
  //
  // URL utility services.
  //
  //----------------------------------------------------------------------------

  /**
   * Return an nsIURI object for the URI spec and base URI specified by aSpec
   * and aBaseURI using the charset specified by aCharset.
   *
   * \param aSpec               URI spec.
   * \param aCharset            URI character set.
   * \param aBaseURI            Base URI.
   *
   * \return                    nsIURI object.
   */

  newURI: function URLUtils_newURI(aSpec, aCharset, aBaseURI) {
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var uri = null;
    try {
      uri = ioService.newURI(aSpec, aCharset, aBaseURI);
    } catch (ex) {
      Cu.reportError(ex);
    }
    return uri;
  },


  /**
   * Add the query parameters specified by aQueryParams to the URL specified by
   * aURL and return the resulting URL.
   *
   * \param aURL                URL to which to add query parameters.
   * \param aQueryParams        Query parameters.
   *
   * \return                    URL with query parameters added.
   */

  addQuery: function URLUtils_addQuery(aURL, aQueryParams) {
    // Get the URL query parameters.
    var urlQueryParams = {};
    this.extractQuery(aURL, urlQueryParams);

    // Add the query parameters.
    for (paramName in aQueryParams) {
      urlQueryParams[paramName] = aQueryParams[paramName];
    }

    // Produce the new URL query string.
    var urlQuery = this.produceQuery(urlQueryParams);

    // Copy the specified URL and set the new query string.
    var url = Cc["@mozilla.org/network/standard-url;1"]
                .createInstance(Ci.nsIStandardURL);
    url.init(Ci.nsIStandardURL.URLTYPE_STANDARD, 0, aURL, null, null);
    url.QueryInterface(Ci.nsIURL);
    url.query = urlQuery;

    return url.spec;
  },


  /**
   * Produce a URL query string from the query parameters specified by the
   * aParams object.  Produce a URL query parameter for each field in the
   * aParams object, applying proper URI encoding.
   *
   * \param aParams             URL query params.
   *
   * \return                    A URL query string.
   */

  produceQuery: function URLUtils_produceQuery(aParams) {
    // Add each field in the query params to the URL query.
    var urlQuery = "";
    for (paramName in aParams) {
      // Add a separator before all but the first field.
      if (urlQuery.length > 0)
        urlQuery += "&";

      // Add the parameter to the URL query.
      var paramValue = aParams[paramName];
      urlQuery += encodeURIComponent(paramName) +
                  "=" +
                  encodeURIComponent(paramValue);
    }

    return urlQuery;
  },


  /**
   * Extract the query parameters from the URL specified by aURLSpec and return
   * them in the object specified by aQueryParams.  If the URL contains a query,
   * return true; otherwise, return false.
   *
   * \param aURLSpec            URL from which to extract query.
   * \param aQueryParams        Returned query parameters.
   *
   * \return                    True if URL contains a query.
   */

  extractQuery: function URLUtils_extractQuery(aURLSpec, aQueryParams) {
    // Get a URL object.
    var url = this.newURI(aURLSpec).QueryInterface(Ci.nsIURL);

    // Get the URL query string.  Just return false if URL does not contain a
    // query string.
    var queryStr = url.query;
    if (!queryStr)
      return false;

    // Extract the query parameters.
    var queryParamList = queryStr.split("&");
    for (var i = 0; i < queryParamList.length; i++) {
      var queryParam = queryParamList[i].split("=");
      if (queryParam.length >= 2) {
        aQueryParams[queryParam[0]] = decodeURIComponent(queryParam[1]);
      }
      else if (queryParam.length == 1) {
        aQueryParams[queryParam[0]] = true;
      }
    }

    return true;
  },


  convertURLToDisplayName: function URLUtils_convertURLToDisplayName(aURL) {
    var urlDisplay = "";

    try {
      urlDisplay = decodeURI( aURL );
    } catch(err) {
      dump("convertURLToDisplayName, oops! URI decode weirdness: " + err + "\n");
    }

    // Set the title display
    if ( urlDisplay.lastIndexOf('/') != -1 )
    {
      urlDisplay = urlDisplay.substring( urlDisplay.lastIndexOf('/') + 1, urlDisplay.length );
    }
    else if ( aURL.lastIndexOf('\\') != -1 )
    {
      urlDisplay = aURL.substring( aURL.lastIndexOf('\\') + 1, aURL.length );
    }

    if ( ! urlDisplay.length )
    {
      urlDisplay = aURL;
    }

    return urlDisplay;
  }
};

