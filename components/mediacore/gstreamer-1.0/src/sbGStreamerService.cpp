/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#include "sbGStreamerService.h"
#include "sbGStreamerMediacoreUtils.h"
#include <gst/pbutils/descriptions.h>
#include <glib.h>

#include <sbLibraryLoaderUtils.h>

#include <nsIEnvironment.h>
#include <nsIProperties.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <prenv.h>
#include <nsServiceManagerUtils.h>
#include <nsDirectoryServiceUtils.h>
#include <nsAppDirectoryServiceDefs.h>
#include <nsComponentManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsXULAppAPI.h>
#include <nsISimpleEnumerator.h>
#include <nsIPrefBranch.h>

#include <sbStringUtils.h>

#if XP_WIN
#include <windows.h>
#endif

#define GSTREAMER_COMPREG_LAST_MODIFIED_TIME_PREF \
          "songbird.mediacore.gstreamer.compreg_last_modified_time"

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerService:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerService =
  PR_NewLogModule("sbGStreamerService");

#define LOG(args)                                          \
  if (gGStreamerService)                             \
    PR_LOG(gGStreamerService, PR_LOG_WARNING, args)

#define TRACE(args)                                        \
  if (gGStreamerService)                             \
    PR_LOG(gGStreamerService, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

static const char *
get_rank_name (gint rank)
{
  switch (rank) {
    case GST_RANK_NONE:
      return "none";
    case GST_RANK_MARGINAL:
      return "marginal";
    case GST_RANK_SECONDARY:
      return "secondary";
    case GST_RANK_PRIMARY:
      return "primary";
    default:
      return "unknown";
  }
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbGStreamerService, sbIGStreamerService)

sbGStreamerService::sbGStreamerService()
{
  LOG(("sbGStreamerService[0x%.8x] - ctor", this));
}

sbGStreamerService::~sbGStreamerService()
{
  LOG(("sbGStreamerService[0x%.8x] - dtor", this));
}

/**
 * Wrapper to set environment variables in a way that doesn't show up in the
 * leak log.  This leaks just as much but shuts up the buildbots.
 * Yes, calling PR_SetEnv / nsIEnvironment::set() leaks.  On purpose.
 */
nsresult SetEnvVar(const nsAString& aName, const nsAString& aValue)
{
  #if XP_WIN
    // on Windows, we need to go through the OS APIs to be able to set Unicode
    // environment variables.  See bmo bug 476739.
    BOOL result;
    if (aValue.IsVoid()) {
      result = ::SetEnvironmentVariableW(PromiseFlatString(aName).get(), NULL);
    } else {
      result = ::SetEnvironmentVariableW(PromiseFlatString(aName).get(),
                                         PromiseFlatString(aValue).get());
    }
    return (result != 0) ? NS_OK : NS_ERROR_FAILURE;
  #else
    nsCString env;
    CopyUTF16toUTF8(aName, env);
    env.AppendLiteral("=");
    env.Append(NS_ConvertUTF16toUTF8(aValue));
    PRInt32 length = env.Length();
    char* buf = (char*)NS_Alloc(length + 1);
    if (!buf) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(buf, env.get(), length);
    buf[length] = '\0';
    PRStatus status = PR_SetEnv(buf);
    // intentionally leak buf
    return (PR_SUCCESS == status) ? NS_OK : NS_ERROR_FAILURE;
  #endif
}

// sbIGStreamerService
nsresult
sbGStreamerService::Init()
{
  NS_ASSERTION(NS_IsMainThread(), "Not on main thread");

  nsresult rv;
  NS_NAMED_LITERAL_STRING(kGstPluginSystemPath, "GST_PLUGIN_SYSTEM_PATH");
  NS_NAMED_LITERAL_STRING(kGstRegistry, "GST_REGISTRY");
  NS_NAMED_LITERAL_STRING(kGstPluginPath, "GST_PLUGIN_PATH");
  PRBool noSystemPlugins;
  PRBool systemGst;
  PRBool hasMore;
  PRBool first = PR_TRUE;
  nsString pluginPaths;
  nsString systemPluginPaths;

  nsCOMPtr<nsISimpleEnumerator> dirList;

  nsCOMPtr<nsIEnvironment> envSvc =
    do_GetService("@mozilla.org/process/environment;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperties> directorySvc =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_MACOSX) || defined(XP_WIN)
  systemGst = PR_FALSE;
  noSystemPlugins = PR_TRUE;
#else
  // On unix, default to using the bundled gstreamer (plus the system plugins
  // as a fallback).
  // If this env var is set, use ONLY the system gstreamer.
  rv = envSvc->Exists(NS_LITERAL_STRING("SB_GST_SYSTEM"), &systemGst);
  NS_ENSURE_SUCCESS(rv, rv);
  // And if this one is set, don't use system plugins
  rv = envSvc->Exists(NS_LITERAL_STRING("SB_GST_NO_SYSTEM"), &noSystemPlugins);
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  if (!systemGst) {
    // Build the plugin path. This is from highest-to-lowest priority, so 
    // we prefer our plugins to the system ones (unless overridden by 
    // GST_PLUGIN_PATH).
    //
    // We use the following paths:
    //   1. Plugin directories set by the user using GST_PLUGIN_PATH (if any),
    //      on unix systems (not windows/osx) only.
    //   2. Extension-provided plugin directories (in no particular order)
    //   3. Our bundled gst-plugins directory
    //
    // Plus the system plugin path on linux:
    //   4. $HOME/.gstreamer-0.10/plugins or $HOME/.gstreamer-1.0/plugins
    //   5. /usr/lib/gstreamer-1.0, /usr/lib64/gstreamer-1.0,
    //      or /usr/lib/x86_64-linux-gnu/gstreamer-1.0

#if defined(XP_MACOSX) || defined(XP_WIN)
    pluginPaths = EmptyString();
#else
    // 1. Read the existing GST_PLUGIN_PATH (if any)
    PRBool pluginPathExists;
    rv = envSvc->Exists(kGstPluginPath, &pluginPathExists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (pluginPathExists) {
      rv = envSvc->Get(kGstPluginPath, pluginPaths);
      NS_ENSURE_SUCCESS(rv, rv);
      first = PR_FALSE;
    }
    else
      pluginPaths = EmptyString();
#endif

    // 2. Add extension-provided plugin directories (if any)
    rv = directorySvc->Get(XRE_EXTENSIONS_DIR_LIST,
                           NS_GET_IID(nsISimpleEnumerator),
                           getter_AddRefs(dirList));
    NS_ENSURE_SUCCESS(rv, rv);

    while (NS_SUCCEEDED(dirList->HasMoreElements(&hasMore)) && hasMore) {
      PRBool dirExists;
      nsCOMPtr<nsISupports> supports;
      rv = dirList->GetNext(getter_AddRefs(supports));
      if (NS_FAILED(rv))
        continue;
      nsCOMPtr<nsIFile> extensionDir(do_QueryInterface(supports, &rv));
      if (NS_FAILED(rv))
          continue;

      rv = extensionDir->Append(NS_LITERAL_STRING("gst-plugins"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = extensionDir->Exists(&dirExists);
      NS_ENSURE_SUCCESS(rv, rv);

      if (dirExists) {
        nsString dirPath;
        rv = extensionDir->GetPath(dirPath);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!first)
          pluginPaths.AppendLiteral(G_SEARCHPATH_SEPARATOR_S);
        pluginPaths.Append(dirPath);
        first = PR_FALSE;

        // The extension might also provide dependent libraries that it needs
        // for the plugin(s) to work. So, load those if there's a special
        // text file listing what we need to load.
        PRBool fileExists;
        nsCOMPtr<nsIFile> dependencyListFile;
        rv = extensionDir->Clone(getter_AddRefs(dependencyListFile));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = dependencyListFile->Append(
                NS_LITERAL_STRING("dependent-libraries.txt"));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = dependencyListFile->Exists(&fileExists);
        NS_ENSURE_SUCCESS(rv, rv);

        if (fileExists) {
          rv = SB_LoadLibraries(dependencyListFile);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }

    // 3. Add our bundled gst-plugins directory
    nsCOMPtr<nsIFile> pluginDir;
    rv = directorySvc->Get("resource:app",
                           NS_GET_IID(nsIFile),
                           getter_AddRefs(pluginDir));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = pluginDir->Append(NS_LITERAL_STRING("gst-plugins"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString pluginDirStr;
    rv = pluginDir->GetPath(pluginDirStr);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!first)
      pluginPaths.AppendLiteral(G_SEARCHPATH_SEPARATOR_S);
    pluginPaths.Append(pluginDirStr);

    // Remaining steps on unix only
#if !defined(XP_MACOSX) && !defined(XP_WIN)

    if (!noSystemPlugins) {


      // 4. Add $HOME/.gstreamer-1.0/plugins to system plugin path
      // Use the same code as gstreamer for this to ensure it's the
      // same path...
      char *homeDirPlugins = g_build_filename (g_get_home_dir (), 
              ".gstreamer-1.0", "plugins", NULL);
      systemPluginPaths = NS_ConvertUTF8toUTF16(homeDirPlugins);

      // 5. Add /usr/lib/gstreamer-1.0 to system plugin path

      // There's a bug in GStreamer which can cause registry problems with
      // renamed plugins. Older versions of decodebin2 were in 
      // 'libgsturidecodebin.so' rather than the current 'libgstdecodebin2.so'.
      // To avoid this, do not use system plugins if this old plugin file
      // exists.
      nsCOMPtr<nsILocalFile> badFile = do_CreateInstance(
              "@mozilla.org/file/local;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString sysLibDir;

#ifdef HAVE_64BIT_OS
  #if 0 // Ubuntu lib paths...
    sysLibDir = NS_LITERAL_STRING("/usr/lib/x86_64-linux-gnu/gstreamer-1.0");
  #else
    sysLibDir = NS_LITERAL_STRING("/usr/lib64/gstreamer-1.0");
  #endif
#else
  sysLibDir = NS_LITERAL_STRING("/usr/lib/gstreamer-1.0");
#endif // HAVE_64BIT_OS

      nsString badFilePath = sysLibDir;
      badFilePath.AppendLiteral("/libgsturidecodebin.so");

      rv = badFile->InitWithPath(badFilePath);
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool badFileExists;
      rv = badFile->Exists(&badFileExists);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!badFileExists) {
        systemPluginPaths.AppendLiteral(G_SEARCHPATH_SEPARATOR_S);
        systemPluginPaths.Append(sysLibDir);
      }
    }
#else
    systemPluginPaths = NS_LITERAL_STRING("");
#endif

    LOG(("sbGStreamerService[0x%.8x] - Setting GST_PLUGIN_PATH=%s", this,
         NS_LossyConvertUTF16toASCII(pluginPaths).get()));
    rv = SetEnvVar(kGstPluginPath, pluginPaths);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("sbGStreamerService[0x%.8x] - Setting GST_PLUGIN_SYSTEM_PATH=%s", this,
         NS_LossyConvertUTF16toASCII(systemPluginPaths).get()));
    rv = SetEnvVar(kGstPluginSystemPath, systemPluginPaths);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set registry path
    nsCOMPtr<nsIFile> registryPath;
    rv = GetGStreamerRegistryFile(getter_AddRefs(registryPath));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString registryPathStr;
    rv = registryPath->GetPath(registryPathStr);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("sbGStreamerService[0x%.8x] - Setting GST_REGISTRY=%s", this,
         NS_LossyConvertUTF16toASCII(registryPathStr).get()));

    rv = SetEnvVar(kGstRegistry, registryPathStr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

#ifdef XP_MACOSX
  // XXX This is very bad according to edward!  But we need it until
  // http://bugzilla.gnome.org/show_bug.cgi?id=521978
  // is fixed.

  gst_registry_fork_set_enabled(FALSE);
#endif

  // Update the gstreamer registry file if needed.
  UpdateGStreamerRegistryFile();

  gst_init(NULL, NULL);

  // Register our custom tags.
  RegisterCustomTags();

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerService::Inspect(sbIGStreamerInspectHandler* aHandler)
{
  NS_ENSURE_ARG_POINTER(aHandler);
  nsresult rv;

  GList *plugins, *orig_plugins;

  rv = aHandler->BeginInspect();
  NS_ENSURE_SUCCESS(rv, rv);

  orig_plugins = plugins = gst_registry_get_plugin_list(gst_registry_get());
  while (plugins) {
    GstPlugin *plugin;
    plugin = (GstPlugin *) (plugins->data);
    plugins = g_list_next (plugins);

    nsCString filename;
    if (gst_plugin_get_filename(plugin)) {
      filename = gst_plugin_get_filename(plugin);
    }
    else {
      filename.SetIsVoid(PR_TRUE);
    }

    rv = aHandler->BeginPluginInfo(nsDependentCString(gst_plugin_get_name(plugin)),
                                   nsDependentCString(gst_plugin_get_description(plugin)),
                                   filename,
                                   nsDependentCString(gst_plugin_get_version(plugin)),
                                   nsDependentCString(gst_plugin_get_license(plugin)),
                                   nsDependentCString(gst_plugin_get_source(plugin)),
                                   nsDependentCString(gst_plugin_get_package(plugin)),
                                   nsDependentCString(gst_plugin_get_origin(plugin)));
    NS_ENSURE_SUCCESS(rv, rv);

    GList *features, *orig_features;
    orig_features = features =
      gst_registry_get_feature_list_by_plugin(gst_registry_get(),
                                              gst_plugin_get_name(plugin));
    while (features) {
      GstPluginFeature *feature;
      feature = GST_PLUGIN_FEATURE(features->data);

      if (GST_IS_ELEMENT_FACTORY(feature)) {
        GstElementFactory *factory;
        factory = GST_ELEMENT_FACTORY(feature);

        rv = InspectFactory(factory, aHandler);
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "InspectFactory failed");
      }

      features = g_list_next(features);
    }

    gst_plugin_feature_list_free(orig_features);

    rv = aHandler->EndPluginInfo();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  gst_plugin_list_free(orig_plugins);

  rv = aHandler->EndInspect();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerService::InspectFactory(GstElementFactory* aFactory,
                                   sbIGStreamerInspectHandler* aHandler)
{
  nsresult rv;

  GstElementFactory* factory;
  factory =
      GST_ELEMENT_FACTORY(gst_plugin_feature_load(GST_PLUGIN_FEATURE
                                                 (aFactory)));
  NS_ENSURE_TRUE(factory, NS_ERROR_UNEXPECTED);

  GstElement *element;
  element = gst_element_factory_create(aFactory, NULL);
  NS_ENSURE_TRUE(element, NS_ERROR_UNEXPECTED);

  gint rank = gst_plugin_feature_get_rank(GST_PLUGIN_FEATURE(factory));

  rv = aHandler->BeginFactoryInfo(nsDependentCString(gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(factory))),
                                  nsDependentCString(gst_element_factory_get_longname(factory)),
                                  nsDependentCString(gst_element_factory_get_klass(factory)),
                                  nsDependentCString(gst_element_factory_get_description(factory)),
                                  nsDependentCString(gst_element_factory_get_author(factory)),
                                  nsDependentCString(get_rank_name(rank)),
                                  rank);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InspectFactoryPads(element, factory, aHandler);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aHandler->EndFactoryInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerService::InspectFactoryPads(GstElement* aElement,
                                       GstElementFactory* aFactory,
                                       sbIGStreamerInspectHandler* aHandler)
{
  GstElementClass *gstelement_class;
  gstelement_class = GST_ELEMENT_CLASS(G_OBJECT_GET_CLASS(aElement));
  nsresult rv;

  const GList *pads;
  pads = gst_element_factory_get_static_pad_templates(aFactory);
  while (pads) {
    GstStaticPadTemplate *padtemplate;
    padtemplate = (GstStaticPadTemplate *) (pads->data);
    pads = g_list_next(pads);

    PRUint32 direction;
    switch (padtemplate->direction) {
      case GST_PAD_SRC:
        direction = sbIGStreamerService::PAD_DIRECTION_SRC;
        break;
      case GST_PAD_SINK:
        direction = sbIGStreamerService::PAD_DIRECTION_SINK;
        break;
      default:
        direction = sbIGStreamerService::PAD_DIRECTION_UNKNOWN;
    }

    PRUint32 presence;
    switch (padtemplate->presence) {
      case GST_PAD_ALWAYS:
        presence = sbIGStreamerService::PAD_PRESENCE_ALWAYS;
        break;
      case GST_PAD_SOMETIMES:
        presence = sbIGStreamerService::PAD_PRESENCE_SOMETIMES;
        break;
      default:
        presence = sbIGStreamerService::PAD_PRESENCE_REQUEST;
    }

    nsCString codecDescription;

    GstCaps* caps = gst_static_caps_get(&padtemplate->static_caps);
    if (caps) {
      if (gst_caps_is_fixed(caps)) {
        gchar* codec = gst_pb_utils_get_codec_description(caps);
        if (codec) {
          codecDescription = codec;
          g_free(codec);
        }
        gst_caps_unref(caps);
      }
    }

    if (codecDescription.IsEmpty()) {
      codecDescription.SetIsVoid(PR_TRUE);
    }
    rv = aHandler->BeginPadTemplateInfo(nsDependentCString(padtemplate->name_template),
                                        direction,
                                        presence,
                                        codecDescription);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aHandler->EndPadTemplateInfo();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbGStreamerService::UpdateGStreamerRegistryFile()
{
  nsresult rv;

  // Get the Mozilla component registry file.
  nsCOMPtr<nsIFile> mozRegFile;
  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR,
                              getter_AddRefs(mozRegFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mozRegFile->Append(NS_LITERAL_STRING("compreg.dat"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if the component registry file exists.
  PRBool exists;
  rv = mozRegFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the last modified time of the component registry file as a string.
  nsCAutoString lastModifiedTime;
  if (exists) {
    PRInt64 lastModifiedTime64;
    rv = mozRegFile->GetLastModifiedTime(&lastModifiedTime64);
    NS_ENSURE_SUCCESS(rv, rv);
    lastModifiedTime.Assign(NS_ConvertUTF16toUTF8
                              (sbAutoString(lastModifiedTime64)));
  }

  // Get the last modified time of the component registry file stored in
  // preferences.
  nsCAutoString lastModifiedTimePref;
  nsCOMPtr<nsIPrefBranch>
    prefBranch = do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  prefBranch->GetCharPref(GSTREAMER_COMPREG_LAST_MODIFIED_TIME_PREF,
                          getter_Copies(lastModifiedTimePref));

  // If the Mozilla component registry file has been modified since it was last
  // checked, delete the gstreamer registry file to force it to be regenerated.
  // This ensures that gstreamer will pick up any new protocol handlers.  See
  // bug 18216.
  if (lastModifiedTimePref.IsEmpty() ||
      !lastModifiedTimePref.Equals(lastModifiedTime)) {
    nsCOMPtr<nsIFile> gstreamerRegFile;
    rv = GetGStreamerRegistryFile(getter_AddRefs(gstreamerRegFile));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = gstreamerRegFile->Exists(&exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (exists) {
      rv = gstreamerRegFile->Remove(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Update the Mozilla component registry file last modified time pref.
  rv = prefBranch->SetCharPref(GSTREAMER_COMPREG_LAST_MODIFIED_TIME_PREF,
                               lastModifiedTime.get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerService::GetGStreamerRegistryFile(nsIFile **aOutRegistryFile)
{
  NS_ENSURE_ARG_POINTER(aOutRegistryFile);
  *aOutRegistryFile = nsnull;

  nsresult rv;
  nsCOMPtr<nsIProperties> directorySvc =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> registryPath;
  rv = directorySvc->Get(NS_APP_USER_PROFILE_50_DIR,
                         NS_GET_IID(nsIFile),
                         getter_AddRefs(registryPath));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = registryPath->Append(NS_LITERAL_STRING("gstreamer-0.10"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = registryPath->Append(NS_LITERAL_STRING("registry.bin"));
  NS_ENSURE_SUCCESS(rv, rv);

  registryPath.forget(aOutRegistryFile);
  return NS_OK;
}

