/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

/**
 * \file  sbMetadataUtils.jsm
 * \brief Javascript source for the metadata utilities.
 */

//------------------------------------------------------------------------------
//
// Metadata utilities JSM configuration.
//
//------------------------------------------------------------------------------

EXPORTED_SYMBOLS = ["sbMetadataUtils"];


//------------------------------------------------------------------------------
//
// Metadata utilities imported services.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

// Songbird imports.
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");


//------------------------------------------------------------------------------
//
// Metadata utilities.
//
//------------------------------------------------------------------------------

var sbMetadataUtils = {
  /**
   *   Write the metadata specified by aPropertyList for the set of media items
   * specified by aMediaItemList.  The metadata is read from each media item's
   * properties; aPropertyList simply specifies which properties to write, not
   * their values.  As with sbIFileMetadataService.write, all media items must
   * be from the same library.
   *   If aSuppressProgressDialog is false, display a metadata write job
   * progress dialog using the parent window specified by aParentWindow;
   * aParentWindow may be null.
   *   Check conditions for writing metadata such as the dontWriteMetadata
   * property of the items' library.  If aMediaList is specified, also check
   * its dontWriteMetadata property.  Do nothing if the dontWriteMetadata
   * property is true.
   *
   * \param aMediaItemList      List of media items for which to write metadata.
   * \param aPropertyList       List of properties to write.
   * \param aParentWindow       Optional parent window for job progress dialog.
   * \param aMediaList          Optional media list providing context for
   *                            writing.
   * \param aSuppressProgressDialog
   *                            If true, don't open job progress dialog.
   */
  writeMetadata: function sbMetadataUtils_writeMetadata
                            (aMediaItemList,
                             aPropertyList,
                             aParentWindow,
                             aMediaList,
                             aSuppressProgressDialog) {
    // Get the item library.
    var library = aMediaItemList[0].library;

    // Do nothing if the item library or the provided media list don't allow
    // writing metadata.
    if ((library.getProperty(SBProperties.dontWriteMetadata) == "1") ||
        (aMediaList &&
         (aMediaList.getProperty(SBProperties.dontWriteMetadata) == "1"))) {
      return;
    }

    // Create an array for the media item list.
    var mediaItemArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                           .createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < aMediaItemList.length; i++) {
      mediaItemArray.appendElement(aMediaItemList[i], false);
    }

    // Create an array for the property list.
    var propertyArray = ArrayConverter.stringEnumerator([aPropertyList]);

    try {
      // Initiate writing of the metadata.
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
      var job = metadataService.write(mediaItemArray, propertyArray);

      // Show a progress dialog.
      if (!aSuppressProgressDialog) {
        SBJobUtils.showProgressDialog(job, aParentWindow);
      }
    } catch (ex) {
      // Job will fail if writing is disabled by the pref.
      Cu.reportError(ex);
    }
  }
};


