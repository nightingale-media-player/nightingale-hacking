/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

