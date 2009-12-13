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

#include "sbGStreamerMediaInspector.h"

#include <sbClassInfoUtils.h>

#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <prlog.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerMediaInspector:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerMediaInspector = PR_NewLogModule("sbGStreamerMediaInspector");
#define LOG(args) PR_LOG(gGStreamerMediaInspector, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerMediaInspector, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */

NS_IMPL_THREADSAFE_ISUPPORTS2(sbGStreamerMediaInspector,
                              sbIMediaInspector,
                              nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbGStreamerMediaInspector,
                             sbIMediaInspector,
                             nsIClassInfo);

NS_DECL_CLASSINFO(sbGstreamerMediaInspector);
NS_IMPL_THREADSAFE_CI(sbGStreamerMediaInspector);

sbGStreamerMediaInspector::sbGStreamerMediaInspector()
{
  mMediaFormat = do_CreateInstance(SB_MEDIAFORMAT_CONTRACTID);
}

sbGStreamerMediaInspector::~sbGStreamerMediaInspector()
{
  /* destructor code */
}

/* sbIGStreamerMediaInspector interface implementation */

NS_IMETHODIMP
sbGStreamerMediaInspector::InspectMedia(sbIMediaItem *aMediaItem,
                                        sbIMediaFormat **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

