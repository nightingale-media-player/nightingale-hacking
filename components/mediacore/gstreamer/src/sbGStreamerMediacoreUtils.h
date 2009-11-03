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

#ifndef _SB_GSTREAMERMEDIACOREUTILS_H_
#define _SB_GSTREAMERMEDIACOREUTILS_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsStringGlue.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMWindow.h>
#include <nsITimer.h>
#include <nsComponentManagerUtils.h>
#include <nsIStringBundle.h>

#include <sbIPropertyArray.h>
#include <sbMediacoreError.h>

#include <gst/gst.h>

/* Custom GStreamer tag names. Must match those used in the various gstreamer
   muxers/taggers we have.
 */
#define SB_GST_TAG_GRACENOTE_TAGID         "gracenote-tagid"
#define SB_GST_TAG_GRACENOTE_EXTENDED_DATA "gracenote-extdata"

GstTagList *ConvertPropertyArrayToTagList(sbIPropertyArray *properties);

nsresult ConvertTagListToPropertyArray(GstTagList *taglist, 
        sbIPropertyArray **aPropertyArray);

class sbGStreamerMessageHandler : public nsISupports {
public:
  virtual ~sbGStreamerMessageHandler() {};
  virtual void HandleMessage(GstMessage *message) = 0;
  virtual PRBool HandleSynchronousMessage(GstMessage *message) = 0;
};
GstBusSyncReply SyncToAsyncDispatcher(GstBus* bus, GstMessage* message,
                                      gpointer data);

/* Get a mediacore error from the gstreamer error. The error string may include
   a reference to a filename or URI; if so the string aResource will be used

   \param aResource  the URI string points to the file that has error.
                     Will be unescaped in the processing.
 */
nsresult GetMediacoreErrorFromGstError(GError *gerror, nsString aResource,
        sbIMediacoreError **_retval);

/* Find an element name for an element that can produce caps compatible with
   'srcCapsString' on its source pad, and has a klass name include 'typeName'.
   Returns NULL if none is found.

   e.g. Call FindMatchingElementName("application/ogg", "Muxer") to get an ogg
        muxer element name ("oggmux" will be returned).
 */
const char *
FindMatchingElementName(const char *srcCapsString, const char *typeName);

/* Register any the custom tags we need to use */
void
RegisterCustomTags();

#endif // _SB_GSTREAMERMEDIACOREUTILS_H_

