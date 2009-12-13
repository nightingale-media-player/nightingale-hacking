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

#ifndef _SB_GSTREAMER_MEDIAINSPECTOR_H_
#define _SB_GSTREAMER_MEDIAINSPECTOR_H_

#include <nsCOMPtr.h>
#include <nsIClassInfo.h>

#include "sbIMediaInspector.h"

class sbGStreamerMediaInspector : public sbIMediaInspector, nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIMEDIAINSPECTOR

  sbGStreamerMediaInspector();

private:
  virtual ~sbGStreamerMediaInspector();

  nsCOMPtr<sbIMediaFormat>               mMediaFormat;
};

#define SB_GSTREAMER_MEDIAINSPECTOR_CLASSNAME \
  "sbGStreamerMediaInspector"
#define SB_GSTREAMER_MEDIAINSPECTOR_DESCRIPTION \
  "Songbird GStreamer Media Inspector"
#define SB_GSTREAMER_MEDIAINSPECTOR_CONTRACTID \
  "@songbirdnest.com/Songbird/Mediacore/gstreamermediainspector;1"
#define SB_GSTREAMER_MEDIAINSPECTOR_CID \
  {0x200782a4, 0x07c6, 0x4ec7, {0xa3, 0x31, 0xb8, 0x88, 0x87, 0x36, 0x80, 0xa6}}

#endif // _SB_GSTREAMER_MEDIAINSPECTOR_H_
