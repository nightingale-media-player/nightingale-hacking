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

#ifndef _SB_GSTREAMER_AUDIO_FILTER_INTERFACE_H_
#define _SB_GSTREAMER_AUDIO_FILTER_INTERFACE_H_

#include <gst/gst.h>

/* Internal (for now) interface for adding filters to the playback pipeline.
 */
class sbIGstAudioFilter
{
public:
  virtual ~sbIGstAudioFilter() {};

  /* Add an audio filter to playback pipelines created by this mediacore. */
  virtual nsresult AddAudioFilter(GstElement *element) = 0;

  /* Remove an audio filter that has previously been added */
  virtual nsresult RemoveAudioFilter(GstElement *element) = 0;
};

#endif // _SB_GSTREAMER_AUDIO_FILTER_INTERFACE_H_
