/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef _SB_GSTREAMER_RTPSTREAMER_H_
#define _SB_GSTREAMER_RTPSTREAMER_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>

#include <nsIClassInfo.h>

#include <gst/gst.h>

#include "sbIGStreamerRTPStreamer.h"
#include "sbGStreamerPipeline.h"

class sbGStreamerRTPStreamer : public sbGStreamerPipeline,
                               public sbIGStreamerRTPStreamer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIGSTREAMERRTPSTREAMER

  sbGStreamerRTPStreamer();

  NS_IMETHODIMP BuildPipeline();

private:
  virtual ~sbGStreamerRTPStreamer();

  nsString                               mSourceURI;
  nsString                               mDestHost;
  PRInt32                                mDestPort;

protected:
  void OnCapsSet(GstCaps *caps);

  // Static helpers
private:
  static void capsNotifyHelper(GObject *obj, GParamSpec *pspec,
          sbGStreamerRTPStreamer *core);

};

#endif // _SB_GSTREAMER_RTPSTREAMER_H_
