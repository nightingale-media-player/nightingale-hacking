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

#include "sbGStreamerMediacoreFactory.h"

#include <nsAutoPtr.h>
#include <nsMemory.h>

#include <sbMediacoreCapabilities.h>
#include <sbTArrayStringEnumerator.h>

#include <sbIGStreamerService.h>

#include "sbGStreamerMediacore.h"
#include "sbGStreamerMediacoreCID.h"

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerMediacoreFactory:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerMediacoreFactory =
  PR_NewLogModule("sbGStreamerMediacoreFactory");

#define LOG(args)                                          \
  if (gGStreamerMediacoreFactory)                             \
    PR_LOG(gGStreamerMediacoreFactory, PR_LOG_WARNING, args)

#define TRACE(args)                                        \
  if (gGStreamerMediacoreFactory)                             \
    PR_LOG(gGStreamerMediacoreFactory, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

NS_IMPL_ISUPPORTS_INHERITED1(sbGStreamerMediacoreFactory,
                             sbBaseMediacoreFactory,
                             sbIMediacoreFactory)

SB_MEDIACORE_FACTORY_REGISTERSELF_IMPL(sbGStreamerMediacoreFactory, 
                                       SB_GSTREAMERMEDIACOREFACTORY_DESCRIPTION)

sbGStreamerMediacoreFactory::sbGStreamerMediacoreFactory()
{
}

sbGStreamerMediacoreFactory::~sbGStreamerMediacoreFactory()
{

}

nsresult 
sbGStreamerMediacoreFactory::Init()
{
  nsresult rv = sbBaseMediacoreFactory::InitBaseMediacoreFactory();
  NS_ENSURE_SUCCESS(rv, rv);

  /* Ensure GStreamer has been loaded by getting the gstreamer service
   * component (which loads and initialises gstreamer for us).
   */
  nsCOMPtr<sbIGStreamerService> service =
    do_GetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacoreFactory::OnInitBaseMediacoreFactory()
{
  nsresult rv = 
    SetName(NS_LITERAL_STRING(SB_GSTREAMERMEDIACOREFACTORY_DESCRIPTION));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = 
    SetContractID(NS_LITERAL_STRING(SB_GSTREAMERMEDIACOREFACTORY_CONTRACTID));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacoreFactory::OnGetCapabilities(
                             sbIMediacoreCapabilities **aCapabilities)
{
  nsRefPtr<sbMediacoreCapabilities> caps;
  NS_NEWXPCOM(caps, sbMediacoreCapabilities);
  NS_ENSURE_TRUE(caps, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = caps->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  // Build a big list of extensions based on everything gstreamer knows about,
  // plus some known ones, minus a few known non-media-file extensions that
  // gstreamer has typefinders for.
  nsTArray<nsString> extensions;
  const char *blacklist[] = {
      "txt", "htm", "html", "xml", "pdf", "cpl", "msstyles", "scr", "sys",
      "ocx", "bz2", "gz", "zip", "Z", "rar", "tar", "dll", "exe", "a",
      "bmp", "png", "gif", "jpeg", "jpg", "jpe", "tif", "tiff", "xpm",
      "dat", "swf", "swfl", "stm"};

  const char *extraExtensions[] = {"m4r", "m4p", "mp4", "vob"};
  GList *walker, *list;

  list = gst_type_find_factory_get_list ();
  walker = list;
  while (walker) {
    GstTypeFindFactory *factory = GST_TYPE_FIND_FACTORY (walker->data);
    gboolean blacklisted = FALSE;

    gchar **factoryexts = gst_type_find_factory_get_extensions (factory);
    if (factoryexts) {
      while (*factoryexts) {
        unsigned int i;
        for (i = 0; i < sizeof(blacklist)/sizeof(*blacklist); i++) {
          if (!g_ascii_strcasecmp(*factoryexts, blacklist[i])) {
            blacklisted = TRUE;
            LOG(("Ignoring extension '%s'", *factoryexts));
            break;
          }
        }

        if (!blacklisted) {
          nsString ext = NS_ConvertUTF8toUTF16(*factoryexts);
          if (!extensions.Contains(ext))
            extensions.AppendElement(ext);
        }
        factoryexts++;
      }
    }
    walker = g_list_next (walker);
  }
  g_list_free (list);

  for (unsigned int i = 0; 
          i < sizeof(extraExtensions)/sizeof(*extraExtensions); i++) 
  {
    nsString ext = NS_ConvertUTF8toUTF16(extraExtensions[i]);
    if(!extensions.Contains(ext))
      extensions.AppendElement(ext);
  }

  // For the moment, we pretend these are all audio file extensions. 
  // Later, we'll whitelist a few as audio, and throw the rest in as video.

  rv = caps->SetAudioExtensions(extensions);
  NS_ENSURE_SUCCESS(rv, rv);

  // Only audio playback for today.
  rv = caps->SetSupportsAudioPlayback(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(caps.get(), aCapabilities);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacoreFactory::OnCreate(const nsAString &aInstanceName, 
                                    sbIMediacore **_retval)
{
  nsRefPtr<sbGStreamerMediacore> mediacore;
  NS_NEWXPCOM(mediacore, sbGStreamerMediacore);
  NS_ENSURE_TRUE(mediacore, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mediacore->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mediacore->SetInstanceName(aInstanceName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(mediacore.get(), _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
