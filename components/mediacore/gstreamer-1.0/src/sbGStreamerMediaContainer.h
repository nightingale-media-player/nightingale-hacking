/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2012 POTI, Inc.
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

#pragma once



#include "nsStringAPI.h"
#include "sbIMediaContainer.h"
#include "gst/gst.h"



class sbGStreamerMediaContainer : public sbIMediaContainer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACONTAINER

  ~sbGStreamerMediaContainer();

  sbGStreamerMediaContainer();

  nsresult Init();

private:
  nsresult
  AcquireMimeType_Priv();

  static
  void
  OnHaveType_Priv (
    GstElement *  typefind,
    guint         probability,
    GstCaps *     caps,
    gpointer      user_data);

  static
  gboolean
  OnBusMessage_Priv (
    GstBus *		bus,
    GstMessage *	message,
    gpointer		data);

  static
  gboolean
  OnMainLoopDone_Priv (
    gpointer		user_data);

  static
  void
  OnPrerollDone_Priv (
    gpointer		user_data);

private:
  GMainLoop *		  mLoop;
                    ///< The GLib loop in which the GStreamer pipeline
                    ///  runs.

  //
  // The typefind pipeline topology is:
  //
  //  ...[ GstPipeline ]....................................
  //  .                                                    .
  //  .   +----------+     +----------+     +----------+   .
  //  .   | filesrc  | --> | typefind | --> | fakesink |   .
  //  .   +----------+     +----------+     +----------+   .
  //  .                                                    .
  //  ......................................................
  //
  // The following variables hold the elements:
  //
  GstPipeline *   mPipeline;
  GstElement *    mFilesrc;
  GstElement *    mTypefind;
  GstElement *    mSink;

  nsCString       mPath;
                    ///< The target file.  When SetPath() sets this
                    ///  value, it also resets mCaps and mMimeType.
                    ///
                    ///  @todo This is redundant, as mFilesrc already
                    ///        keeps this path

  GstCaps *       mCaps;
                    ///< The GStreamer caps of the file as determined
                    ///  by the typefind element.  If this is not NULL
                    ///  then mMimeType is already cached.

  nsCString       mMimeType;
                    ///< The cached MIME type of the file, if mCaps is
                    ///  not NULL.  If mCaps is NULL, then
                    ///  AcquireMimeType_Priv() must be called to set
                    ///  this variable.
};

#define SB_GSTREAMER_MEDIACONTAINER_CLASSNAME \
  "sbGStreamerMediaContainer"
#define SB_GSTREAMER_MEDIACONTAINER_CID \
  {0x4da451c0, 0xa11a, 0x42bc, {0x85, 0xed, 0xa2, 0x4c, 0x4, 0xbd, 0xdd, 0xa5}}
  // {4DA451C0-A11A-42bc-85ED-A24C04BDDDA5}
// SB_MEDIACONTAINER_CONTRACTID is defined in sbIMediaInspector.idl/.h
#define SB_GSTREAMER_MEDIACONTAINER_CONTRACTID \
  SB_MEDIACONTAINER_CONTRACTID
#define SB_GSTREAMER_MEDIACONTAINER_DESCRIPTION \
  "Songbird GStreamer Media Container"
