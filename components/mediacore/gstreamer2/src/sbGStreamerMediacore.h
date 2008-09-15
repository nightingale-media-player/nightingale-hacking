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

#ifndef __SB_GSTREAMERMEDIACORE_H__
#define __SB_GSTREAMERMEDIACORE_H__

#include <sbIGStreamerMediacore.h>

#include <nsCOMPtr.h>

#include <sbIMediacore.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreVolumeControl.h>
#include <sbIMediacoreVotingParticipant.h>

#include <sbBaseMediacore.h>
#include <sbBaseMediacorePlaybackControl.h>
#include <sbBaseMediacoreVolumeControl.h>

#include <gst/gst.h>
#include "sbIGstPlatformInterface.h"

class nsIURI;

class sbGStreamerMediacore : public sbBaseMediacore,
                             public sbBaseMediacorePlaybackControl,
                             public sbBaseMediacoreVolumeControl,
                             public sbIMediacoreVotingParticipant,
                             public sbIGStreamerMediacore
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICLASSINFO
  
  NS_FORWARD_SBIMEDIACORE(sbBaseMediacore::)
  NS_FORWARD_SBIMEDIACOREPLAYBACKCONTROL(sbBaseMediacorePlaybackControl::)
  NS_FORWARD_SBIMEDIACOREVOLUMECONTROL(sbBaseMediacoreVolumeControl::)

  NS_DECL_SBIMEDIACOREVOTINGPARTICIPANT
  NS_DECL_SBIGSTREAMERMEDIACORE

  sbGStreamerMediacore();

  nsresult Init();

  // sbBaseMediacore overrides
  virtual nsresult OnInitBaseMediacore();
  virtual nsresult OnGetCapabilities();
  virtual nsresult OnShutdown();

  // sbBaseMediacorePlaybackControl overrides
  virtual nsresult OnInitBaseMediacorePlaybackControl();
  virtual nsresult OnSetUri(nsIURI *aURI);
  virtual nsresult OnSetPosition(PRUint64 aPosition);
  virtual nsresult OnPlay();
  virtual nsresult OnPause();
  virtual nsresult OnStop();

  // sbBaseMediacoreVolumeControl overrides
  virtual nsresult OnInitBaseMediacoreVolumeControl();
  virtual nsresult OnSetMute(PRBool aMute);
  virtual nsresult OnSetVolume(PRFloat64 aVolume);

  // GStreamer message handling
  virtual void HandleMessage(GstMessage *message);
  virtual PRBool HandleSynchronousMessage(GstMessage *message);

protected:
  virtual ~sbGStreamerMediacore();

  virtual nsresult DestroyPipeline();
  virtual nsresult CreatePlaybackPipeline();

private:
  // Static helper for C callback
  static void syncHandler(GstBus *bus, GstMessage *message, gpointer data);

protected:
  PRBool mVideoEnabled;

  GstElement *mPipeline;

  sbIGstPlatformInterface *mPlatformInterface;
};

#endif /* __SB_GSTREAMERMEDIACORE_H__ */
