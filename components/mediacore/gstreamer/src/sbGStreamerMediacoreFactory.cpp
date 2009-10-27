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

#include <nsMemory.h>

#include <nsXPCOMCID.h>
#include <nsIObserverService.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>

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

#define BLACKLIST_EXTENSIONS_PREF "songbird.mediacore.gstreamer.blacklistExtensions"
#define VIDEO_EXTENSIONS_PREF "songbird.mediacore.gstreamer.videoExtensions"
#define VIDEO_DISABLED_PREF "songbird.mediacore.gstreamer.disablevideo"

NS_IMPL_ISUPPORTS_INHERITED2(sbGStreamerMediacoreFactory,
                             sbBaseMediacoreFactory,
                             sbIMediacoreFactory,
                             nsIObserver)

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

  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this, "quit-application", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch2> rootPrefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rootPrefBranch->AddObserver(BLACKLIST_EXTENSIONS_PREF, this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rootPrefBranch->AddObserver(VIDEO_EXTENSIONS_PREF, this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbGStreamerMediacoreFactory::Shutdown()
{
  nsresult rv;

  nsCOMPtr<nsIObserverService> obs =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->RemoveObserver(this, "quit-application");
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIPrefBranch2> rootPrefBranch =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rootPrefBranch->RemoveObserver(BLACKLIST_EXTENSIONS_PREF, this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = rootPrefBranch->RemoveObserver(VIDEO_EXTENSIONS_PREF, this);
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
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsAutoMonitor mon(mMonitor);

  nsresult rv;
  if (!mCapabilities) {
    nsRefPtr<sbMediacoreCapabilities> caps;
    NS_NEWXPCOM(caps, sbMediacoreCapabilities);
    NS_ENSURE_TRUE(caps, NS_ERROR_OUT_OF_MEMORY);

    rv = caps->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    // Build a big list of extensions based on everything gstreamer knows about,
    // plus some known ones, minus a few known non-media-file extensions that
    // gstreamer has typefinders for.

    nsCOMPtr<nsIPrefBranch> rootPrefBranch =
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsTArray<nsString> audioExtensions;
    nsTArray<nsString> videoExtensions;
    
    // XXX Mook: we have a silly list of blacklisted extensions because we don't
    // support them and we're being stupid and guessing things based on them.
    // This crap should really look for a plugin that may possibly actually decode
    // these things, or something better.  Whatever the real solution is, this
    // isn't it :(
    nsCString blacklistExtensions;
    { // for scope
      const char defaultBlacklistExtensions[] =
        "txt,htm,html,xml,pdf,cpl,msstyles,scr,sys,ocx,bz2,gz,zip,Z,rar,tar,dll,"
        "exe,a,bmp,png,gif,jpeg,jpg,jpe,tif,tiff,xpm,dat,swf,swfl,stm,cgi,sf,xcf,"
        "far,wvc,mpc,mpp,mp+";
      char* blacklistExtensionsPtr = nsnull;
      rv = rootPrefBranch->GetCharPref(BLACKLIST_EXTENSIONS_PREF,
                                       &blacklistExtensionsPtr);
      if (NS_SUCCEEDED(rv)) {
        blacklistExtensions.Adopt(blacklistExtensionsPtr);
      } else {
        blacklistExtensions.Assign(defaultBlacklistExtensions);
      }
      blacklistExtensions.Insert(',', 0);
      blacklistExtensions.Append(',');
      LOG(("sbGStreamerMediacoreFactory: blacklisted extensions: %s\n",
           blacklistExtensions.BeginReading()));
    }

    const char *extraAudioExtensions[] = {"m4r", "m4p", "mp4", "oga"};
    const char *extraVideoExtensions[] = {"vob"};
    
    // XXX Mook: we currently assume anything not known to be video is audio :|
    nsCString knownVideoExtensions;
    
    { // for scope
      const char defaultKnownVideoExtensions[] =
        "264,avi,dif,dv,flc,fli,flv,h264,jng,m4v,mkv,mng,mov,mpe,mpeg,mpg,mpv,mve,"
        "nuv,ogm,qif,qti,qtif,ras,rm,rmvb,smil,ts,viv,wmv,x264";
      char* knownVideoExtensionsPtr = nsnull;
      rv = rootPrefBranch->GetCharPref(VIDEO_EXTENSIONS_PREF,
                                       &knownVideoExtensionsPtr);
      if (NS_SUCCEEDED(rv)) {
        knownVideoExtensions.Adopt(knownVideoExtensionsPtr);
      } else {
        knownVideoExtensions.Assign(defaultKnownVideoExtensions);
      }
      knownVideoExtensions.Insert(',', 0);
      knownVideoExtensions.Append(',');
      LOG(("sbGStreamerMediacoreFactory: known video extensions: %s\n",
           knownVideoExtensions.BeginReading()));
    }

    GList *walker, *list;

    list = gst_type_find_factory_get_list ();
    walker = list;
    while (walker) {
      GstTypeFindFactory *factory = GST_TYPE_FIND_FACTORY (walker->data);
      gboolean blacklisted = FALSE;
      const gchar* factoryName = gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (factory));
      gboolean isAudioFactory = g_str_has_prefix(factoryName, "audio/");

      gchar **factoryexts = gst_type_find_factory_get_extensions (factory);
      if (factoryexts) {
        while (*factoryexts) {
          gboolean isAudioExtension = isAudioFactory;
          nsCString delimitedExtension(*factoryexts);
          delimitedExtension.Insert(',', 0);
          delimitedExtension.Append(',');
          
          blacklisted = (blacklistExtensions.Find(delimitedExtension) != -1);
          #if PR_LOGGING
            if (blacklisted) {
                LOG(("sbGStreamerMediacoreFactory: Ignoring extension '%s'", *factoryexts));
            }
          #endif /* PR_LOGGING */

          if (!blacklisted) {
            if (!isAudioExtension) {
              if (knownVideoExtensions.Find(delimitedExtension) == -1) {
                isAudioExtension = TRUE;
              }
            }
            
            nsString ext = NS_ConvertUTF8toUTF16(*factoryexts);
            if (isAudioExtension) {
              if (!audioExtensions.Contains(ext)) {
                audioExtensions.AppendElement(ext);
                LOG(("sbGStreamerMediacoreFactory: registering audio extension %s\n",
                     *factoryexts));
              }
            } else {
              if (!videoExtensions.Contains(ext)) {
                videoExtensions.AppendElement(ext);
                LOG(("sbGStreamerMediacoreFactory: registering video extension %s\n",
                     *factoryexts));
              }
            }
          }
          factoryexts++;
        }
      }
      walker = g_list_next (walker);
    }
    g_list_free (list);

    for (unsigned int i = 0; i < NS_ARRAY_LENGTH(extraAudioExtensions); i++) 
    {
      nsString ext = NS_ConvertUTF8toUTF16(extraAudioExtensions[i]);
      if(!audioExtensions.Contains(ext))
        audioExtensions.AppendElement(ext);
    }

    for (unsigned int i = 0; i < NS_ARRAY_LENGTH(extraVideoExtensions); i++) 
    {
      nsString ext = NS_ConvertUTF8toUTF16(extraVideoExtensions[i]);
      if(!videoExtensions.Contains(ext))
        videoExtensions.AppendElement(ext);
    }

    rv = caps->SetAudioExtensions(audioExtensions);
    NS_ENSURE_SUCCESS(rv, rv);

    // Audio playback is always allowed.
    rv = caps->SetSupportsAudioPlayback(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool videoDisabled = PR_FALSE;
    rv = rootPrefBranch->GetBoolPref(
                                    "songbird.mediacore.gstreamer.disablevideo",
                                    &videoDisabled);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!videoDisabled) {
      rv = caps->SetVideoExtensions(videoExtensions);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = caps->SetSupportsVideoPlayback(PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mCapabilities = caps;
  }

  rv = CallQueryInterface(mCapabilities.get(), aCapabilities);
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

nsresult 
sbGStreamerMediacoreFactory::Observe(nsISupports *subject,
                                     const char* topic,
                                     const PRUnichar *aData)
{
  if (!strcmp(topic, "quit-application")) {
    return Shutdown();
  }
  else if (!strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID)) {
    NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
    nsAutoMonitor mon(mMonitor);

    mCapabilities = nsnull;
    return NS_OK;
  }

  return NS_OK;
}
