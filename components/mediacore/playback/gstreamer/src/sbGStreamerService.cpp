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
#include "sbGStreamerService.h"
#include <gst/pbutils/descriptions.h>
#include <glib/gutils.h>

#include <nsIEnvironment.h>
#include <nsIProperties.h>
#include <nsIFile.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <nsServiceManagerUtils.h>
#include <nsDirectoryServiceDefs.h>
#include <nsAppDirectoryServiceDefs.h>
#include <nsXULAppAPI.h>
#include <nsISimpleEnumerator.h>

#if defined( PR_LOGGING )
extern PRLogModuleInfo* gGStreamerLog;
#define LOG(args) PR_LOG(gGStreamerLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

static char *
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

NS_IMPL_ISUPPORTS2(sbGStreamerService, sbIGStreamerService, nsIObserver)

sbGStreamerService::sbGStreamerService()
{
  LOG(("sbGStreamerService[0x%.8x] - ctor", this));
}

sbGStreamerService::~sbGStreamerService()
{
  LOG(("sbGStreamerService[0x%.8x] - dtor", this));
}

// sbIGStreamerService
nsresult
sbGStreamerService::Init()
{
  nsresult rv;
  NS_NAMED_LITERAL_STRING(kGstPluginSystemPath, "GST_PLUGIN_SYSTEM_PATH");
  NS_NAMED_LITERAL_STRING(kGstRegistry, "GST_REGISTRY");
  NS_NAMED_LITERAL_STRING(kGstPluginPath, "GST_PLUGIN_PATH");
  PRBool bundledGst;
  PRBool pluginPathExists;
  PRBool hasMore;
  PRBool first = PR_TRUE;
  nsString pluginPaths;

  nsCOMPtr<nsISimpleEnumerator> dirList;

  nsCOMPtr<nsIEnvironment> envSvc =
    do_GetService("@mozilla.org/process/environment;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperties> directorySvc =
    do_GetService("@mozilla.org/file/directory_service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(XP_MACOSX) || defined(XP_WIN)
  bundledGst = PR_TRUE;
#else
  // On unix, default to using the system gstreamer; only use the bundled one
  // if this env var is set.
  rv = envSvc->Exists(NS_LITERAL_STRING("SB_GST_BUNDLED"), &bundledGst);
  NS_ENSURE_SUCCESS(rv, rv);
#endif

  if (bundledGst) {
    // Set the plugin path.  This is gst-plugins in the dist directory
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

    LOG(("sbGStreamerService[0x%.8x] - Setting GST_PLUGIN_SYSTEM_PATH=%s", this,
         NS_LossyConvertUTF16toASCII(pluginDirStr).get()));

    rv = envSvc->Set(kGstPluginSystemPath, pluginDirStr);
    NS_ENSURE_SUCCESS(rv, rv);

    // Clear GST_PLUGIN_PATH: we're using the bundled gstreamer, and don't
    // want external plugins to get used.
    rv = envSvc->Set(kGstPluginPath, EmptyString());
    NS_ENSURE_SUCCESS(rv, rv);

    // Set registry path
    nsCOMPtr<nsIFile> registryPath;
    rv = directorySvc->Get(NS_APP_USER_PROFILE_50_DIR,
                           NS_GET_IID(nsIFile),
                           getter_AddRefs(registryPath));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = registryPath->Append(NS_LITERAL_STRING("gstreamer-0.10"));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = registryPath->Append(NS_LITERAL_STRING("registry.bin"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString registryPathStr;
    rv = registryPath->GetPath(registryPathStr);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("sbGStreamerService[0x%.8x] - Setting GST_REGISTRY=%s", this,
         NS_LossyConvertUTF16toASCII(registryPathStr).get()));

    rv = envSvc->Set(kGstRegistry, registryPathStr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Now, we append plugin directories from extensions to GST_PLUGIN_PATH
  rv = directorySvc->Get(XRE_EXTENSIONS_DIR_LIST,
                         NS_GET_IID(nsISimpleEnumerator),
                         getter_AddRefs(dirList));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = envSvc->Exists(kGstPluginPath, &pluginPathExists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (pluginPathExists) {
    rv = envSvc->Get(kGstPluginPath, pluginPaths);
    NS_ENSURE_SUCCESS(rv, rv);
    first = PR_FALSE;
  }
  else
    pluginPaths = EmptyString();

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
        pluginPaths.Append(NS_LITERAL_STRING(G_SEARCHPATH_SEPARATOR_S));
      pluginPaths.Append(dirPath);
      first = PR_FALSE;
    }
  }

  rv = envSvc->Set(kGstPluginPath, pluginPaths);
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef XP_MACOSX
  // XXX This is very bad according to edward!  But we need it until
  // http://bugzilla.gnome.org/show_bug.cgi?id=521978
  // is fixed.

  gst_registry_fork_set_enabled(FALSE);
#endif

  gst_init(NULL, NULL);

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

  orig_plugins = plugins = gst_default_registry_get_plugin_list();
  while (plugins) {
    GstPlugin *plugin;
    plugin = (GstPlugin *) (plugins->data);
    plugins = g_list_next (plugins);

    nsCString filename;
    if (plugin->filename) {
      filename = plugin->filename;
    }
    else {
      filename.SetIsVoid(PR_TRUE);
    }

    rv = aHandler->BeginPluginInfo(nsDependentCString(plugin->desc.name),
                                   nsDependentCString(plugin->desc.description),
                                   filename,
                                   nsDependentCString(plugin->desc.version),
                                   nsDependentCString(plugin->desc.license),
                                   nsDependentCString(plugin->desc.source),
                                   nsDependentCString(plugin->desc.package),
                                   nsDependentCString(plugin->desc.origin));
    NS_ENSURE_SUCCESS(rv, rv);

    GList *features, *orig_features;
    orig_features = features =
      gst_registry_get_feature_list_by_plugin(gst_registry_get_default(),
                                              plugin->desc.name);
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

  gint rank = GST_PLUGIN_FEATURE(factory)->rank;

  rv = aHandler->BeginFactoryInfo(nsDependentCString(factory->details.longname),
                                  nsDependentCString(factory->details.klass),
                                  nsDependentCString(factory->details.description),
                                  nsDependentCString(factory->details.author),
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
  pads = aFactory->staticpadtemplates;
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

// nsIObserver
NS_IMETHODIMP
sbGStreamerService::Observe(nsISupports *aSubject,
                            const char *aTopic,
                            const PRUnichar *aData)
{
  return NS_OK;
}
