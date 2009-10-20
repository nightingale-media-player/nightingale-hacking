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

#ifndef _SB_GSTREAMER_SERVICE_H_
#define _SB_GSTREAMER_SERVICE_H_

#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include "sbIGStreamerService.h"

#include <gst/gst.h>

class sbGStreamerService : public sbIGStreamerService,
                           public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIGSTREAMERSERVICE
  NS_DECL_NSIOBSERVER

  nsresult Init();

  sbGStreamerService();

private:

  nsresult InspectFactory(GstElementFactory* aFactory,
                          sbIGStreamerInspectHandler* aHandler);

  nsresult InspectFactoryPads(GstElement* aElement,
                              GstElementFactory* aFactory,
                              sbIGStreamerInspectHandler* aHandler);

  nsresult GetGStreamerRegistryFile(nsIFile **aOutRegistryFile);

  virtual ~sbGStreamerService();
};

#endif // _SB_GSTREAMER_SERVICE_H_
