// vim: set sw=2 :
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

#ifndef __SB_GSTREAMER_METADATA_HANDLER_H__
#define __SB_GSTREAMER_METADATA_HANDLER_H__

#include "sbIMetadataHandler.h"
#include "sbGStreamerMediacoreUtils.h"

#include <sbIMediacoreFactory.h>

#include <nsIChannel.h>
#include <nsITimer.h>

#include <prlock.h>
#include <mozilla/Mutex.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <gst/gst.h>

class sbGStreamerMetadataHandler : public sbGStreamerMessageHandler,
                                   public sbIMetadataHandler,
                                   public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETADATAHANDLER
  NS_DECL_NSITIMERCALLBACK

  /* sbGstreamerMessageHandler */
  virtual void HandleMessage(GstMessage *message);
  virtual PRBool HandleSynchronousMessage(GstMessage *message);

  sbGStreamerMetadataHandler();
  
  nsresult Init();

private:
  ~sbGStreamerMetadataHandler();

protected:
  
  /* protects access to all member variables; use nsAutoLock */
  mozilla::Mutex mLock;
  GstElement *mPipeline; // owning

  // Metadata, both in original GstTagList form, and transformed into an
  // sbIPropertyArray. Both may be NULL.  mProperties will explicitly be null
  // if we have failed to read the data.
  GstTagList *mTags;
  nsCOMPtr<sbIMutablePropertyArray> mProperties;

  PRBool mHasAudio;
  PRBool mHasVideo;
  
  static void on_pad_added(GstElement *decodeBin,
                           GstPad *newPad,
                           sbGStreamerMetadataHandler *self);
  static void on_pad_caps_changed(GstPad *pad,
                                  GParamSpec *pspec,
                                  sbGStreamerMetadataHandler *data);
  
  void HandleTagMessage(GstMessage *message);
  
  nsCOMPtr<nsIChannel> mChannel;
  nsCOMPtr<nsITimer> mTimer; // for timeout
  nsCOMPtr<sbIMediacoreFactory> mFactory; // for voting
  nsCString mSpec;
  PRBool mCompleted;
  
  // the lock should be held when calling these methods
  
  /**
   * Prepare the tags for reporting to the caller
   * \param aSucceeded true if metadata scanning had succeded
   */
  nsresult FinalizeTags(PRBool aSucceeded);
};

#define SB_GSTREAMER_METADATA_HANDLER_CLASSNAME \
  "sbGStreamerMetadataHandler"
#define SB_GSTREAMER_METADATA_HANDLER_DESCRIPTION \
  "Songbird GStreamer Metadata Handler"
#define SB_GSTREAMER_METADATA_HANDLER_CONTRACTID \
  "@songbirdnest.com/Songbird/MetadataHandler/GStreamer;1"
#define SB_GSTREAMER_METADATA_HANDLER_CID \
  {0x58cc7fbd, 0xdfa3, 0x4e36, {0x95, 0xd6, 0xb3, 0x6d, 0x67, 0x7a, 0x6e, 0xab}}

#endif /* __SB_GSTREAMER_METADATA_HANDLER_H__ */
