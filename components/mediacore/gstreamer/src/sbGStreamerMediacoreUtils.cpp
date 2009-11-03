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

#include "sbGStreamerMediacoreUtils.h"

#include <nsIRunnable.h>
#include <nsINetUtil.h>

#include <nsAutoPtr.h>
#include <nsThreadUtils.h>
#include <nsTArray.h>
#include <nsMemory.h>

#include <sbStandardProperties.h>
#include <sbStringBundle.h>

#include <prlog.h>
#include <gst/gst.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerMediacoreUtils:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerMediacoreUtils =
  PR_NewLogModule("sbGStreamerMediacoreUtils");

#define LOG(args)                                          \
  if (gGStreamerMediacoreUtils)                            \
    PR_LOG(gGStreamerMediacoreUtils, PR_LOG_WARNING, args)

#define TRACE(args)                                        \
  if (gGStreamerMediacoreUtils)                            \
    PR_LOG(gGStreamerMediacore, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

// TODO: The following are known GStreamer tags we don't handle:
// GST_TAG_TITLE_SORTNAME
// GST_TAG_ARTIST_SORTNAME
// GST_TAG_ALBUM_SORTNAME
// GST_TAG_EXTENDED_COMMENT (probably not convertable?)
// GST_TAG_DESCRIPTION
// GST_TAG_VERSION
// GST_TAG_ISRC
// GST_TAG_ORGANIZATION
// GST_TAG_CONTACT
// GST_TAG_LICENSE
// GST_TAG_LICENSE_URI
// GST_TAG_PERFORMER
// GST_TAG_SERIAL
// GST_TAG_TRACK_GAIN
// GST_TAG_TRACK_PEAK
// GST_TAG_ALBUM_GAIN
// GST_TAG_ALBUM_PEAK
// GST_TAG_REFERENCE_LEVEL
// GST_TAG_LANGUAGE_CODE
// GST_TAG_IMAGE
// GST_TAG_PREVIEW_IMAGE
// GST_TAG_ATTACHMENT
// GST_TAG_BEATS_PER_MINUTE
// GST_TAG_CODEC 
// GST_TAG_VIDEO_CODEC
// GST_TAG_AUDIO_CODEC
// GST_TAG_BITRATE
// GST_TAG_NOMINAL_BITRATE
// GST_TAG_MINIMUM_BITRATE
// GST_TAG_MAXIMUM_BITRATE
// GST_TAG_ENCODER
// GST_TAG_ENCODER_VERSION

// Property names for Gracenote properties
// These must match the definitions in 
// extras/extensions/gracenote/src/sbGracenoteDefines.h
#define SB_GN_PROP_EXTENDEDDATA "http://gracenote.com/pos/1.0#extendedData"
#define SB_GN_PROP_TAGID        "http://gracenote.com/pos/1.0#tagId"

PRBool
ConvertSinglePropertyToTag(sbIProperty *property,
        GstTagList *taglist)
{
  nsresult rv;
  nsString id, value;

  rv = property->GetId(id);
  NS_ENSURE_SUCCESS(rv,PR_FALSE);

  rv = property->GetValue(value);
  NS_ENSURE_SUCCESS(rv,PR_FALSE);

/* Some nasty macros. Less nasty than writing all this out any other way... */
#define TAG_CONVERT_STRING(sbname,gstname) \
  if (id == NS_LITERAL_STRING (sbname)) { \
    NS_ConvertUTF16toUTF8 utf8value(value); \
    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, (gchar *)gstname, \
            utf8value.BeginReading(), NULL); \
    return PR_TRUE; \
  }

#define TAG_CONVERT_UINT(sbname,gstname) \
  if (id == NS_LITERAL_STRING (sbname)) { \
    unsigned int gstvalue = value.ToInteger(&rv); \
    NS_ENSURE_SUCCESS (rv, PR_FALSE); \
    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, (gchar *)gstname, \
            gstvalue, NULL); \
    return PR_TRUE; \
  }

  TAG_CONVERT_STRING (SB_PROPERTY_ALBUMNAME, GST_TAG_ALBUM);
  TAG_CONVERT_STRING (SB_PROPERTY_ARTISTNAME, GST_TAG_ARTIST);
  TAG_CONVERT_STRING (SB_PROPERTY_TRACKNAME, GST_TAG_TITLE);
  TAG_CONVERT_STRING (SB_PROPERTY_COMPOSERNAME, GST_TAG_COMPOSER);
  TAG_CONVERT_STRING (SB_PROPERTY_GENRE, GST_TAG_GENRE);
  TAG_CONVERT_STRING (SB_PROPERTY_COMMENT, GST_TAG_COMMENT);
  TAG_CONVERT_STRING (SB_PROPERTY_ORIGINURL, GST_TAG_LOCATION);
  TAG_CONVERT_STRING (SB_PROPERTY_COPYRIGHT, GST_TAG_COPYRIGHT);
  TAG_CONVERT_STRING (SB_PROPERTY_COPYRIGHTURL, GST_TAG_COPYRIGHT_URI);

  TAG_CONVERT_UINT (SB_PROPERTY_TRACKNUMBER, GST_TAG_TRACK_NUMBER);
  TAG_CONVERT_UINT (SB_PROPERTY_TOTALTRACKS, GST_TAG_TRACK_COUNT);
  TAG_CONVERT_UINT (SB_PROPERTY_DISCNUMBER, GST_TAG_ALBUM_VOLUME_NUMBER);
  TAG_CONVERT_UINT (SB_PROPERTY_TOTALDISCS, GST_TAG_ALBUM_VOLUME_COUNT);

  // Some special cases, without the macros...
  if (id == NS_LITERAL_STRING (SB_PROPERTY_YEAR)) {
    GDate *date;
    int year = value.ToInteger(&rv);
    NS_ENSURE_SUCCESS (rv, PR_FALSE);

    date = g_date_new();
    g_date_set_year (date, year);
    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, (gchar *)GST_TAG_DATE,
            date, NULL);
    g_date_free (date);
    return PR_TRUE;
  }

  if (id == NS_LITERAL_STRING (SB_PROPERTY_DURATION)) {
    int durationMillis = value.ToInteger(&rv);
    guint64 durationNanos;
    NS_ENSURE_SUCCESS (rv, PR_FALSE);
    durationNanos = durationMillis * GST_MSECOND;
    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, (gchar *)GST_TAG_DURATION,
            durationNanos, NULL);
    return PR_TRUE;
  }

  /* Custom stuff we're contractually obligated to include. There's no clean
     way to do this. */
  TAG_CONVERT_STRING (SB_GN_PROP_TAGID,
                      SB_GST_TAG_GRACENOTE_TAGID);
  TAG_CONVERT_STRING (SB_GN_PROP_EXTENDEDDATA,
                      SB_GST_TAG_GRACENOTE_EXTENDED_DATA);

  // If we get here, we failed to convert it.
  return PR_FALSE;
}

GstTagList *
ConvertPropertyArrayToTagList(sbIPropertyArray *properties)
{
  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> propertyEnum;
  PRBool more;
  GstTagList *tags;
  PRBool converted;

  if (properties == nsnull)
    return NULL;

  tags = gst_tag_list_new();
  rv = properties->Enumerate(getter_AddRefs(propertyEnum));
  NS_ENSURE_SUCCESS(rv, NULL);

  while (NS_SUCCEEDED (propertyEnum->HasMoreElements(&more)) && more) {
    nsCOMPtr<nsISupports> next;
    if (NS_SUCCEEDED(propertyEnum->GetNext(getter_AddRefs(next))) && next) {
      nsCOMPtr<sbIProperty> property(do_QueryInterface(next));

      converted = ConvertSinglePropertyToTag (property, tags);
      if (!converted)
        LOG(("Failed to convert property to tag"));
    }
  }

  return tags;
}

static void
ConvertSingleTag(const GstTagList *taglist, 
        const gchar *tag, gpointer user_data)
{
  sbIMutablePropertyArray *propArray = 
      reinterpret_cast<sbIMutablePropertyArray *>(user_data);

#define PROPERTY_CONVERT_STRING(propname, tagname)            \
  if (!strcmp(tag, tagname)) {                                \
    gchar *tagvalue;                                          \
    if (gst_tag_list_get_string (taglist, tag, &tagvalue)) {  \
      propArray->AppendProperty(NS_LITERAL_STRING (propname), \
              NS_ConvertUTF8toUTF16(tagvalue));               \
      g_free (tagvalue);                                      \
      return;                                                 \
    }                                                         \
  }

#define PROPERTY_CONVERT_UINT(propname, tagname, scale)       \
  if (!strcmp(tag, tagname)) {                                \
    guint tagvalue;                                           \
    if (gst_tag_list_get_uint (taglist, tag, &tagvalue)) {    \
      nsString stringVal;                                     \
      stringVal.AppendInt(tagvalue / scale);                  \
      propArray->AppendProperty(NS_LITERAL_STRING (propname), \
              stringVal);                                     \
      return;                                                 \
    }                                                         \
  }

  PROPERTY_CONVERT_STRING (SB_PROPERTY_ALBUMNAME, GST_TAG_ALBUM);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_ARTISTNAME, GST_TAG_ARTIST);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_TRACKNAME, GST_TAG_TITLE);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COMPOSERNAME, GST_TAG_COMPOSER);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_GENRE, GST_TAG_GENRE);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COMMENT, GST_TAG_COMMENT);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_ORIGINURL, GST_TAG_LOCATION);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COPYRIGHT, GST_TAG_COPYRIGHT);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COPYRIGHTURL, GST_TAG_COPYRIGHT_URI);

  PROPERTY_CONVERT_UINT (SB_PROPERTY_BITRATE, GST_TAG_BITRATE, 1000);
}

nsresult
ConvertTagListToPropertyArray(GstTagList *taglist, 
        sbIPropertyArray **aPropertyArray)
{
  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> proparray = do_CreateInstance(
          "@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  gst_tag_list_foreach (taglist, ConvertSingleTag, proparray);

  nsCOMPtr<sbIPropertyArray> props = do_QueryInterface(proparray, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  NS_ADDREF(*aPropertyArray = props);
  return NS_OK;
}

class sbGstMessageEvent : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  explicit sbGstMessageEvent(GstMessage *msg, sbGStreamerMessageHandler *handler) :
      mHandler(handler)
  {
    gst_message_ref(msg);
    mMessage = msg;
  }

  ~sbGstMessageEvent() {
    gst_message_unref(mMessage);
  }

  NS_IMETHOD Run()
  {
    mHandler->HandleMessage(mMessage);
    return NS_OK;
  }

private:
  GstMessage *mMessage;
  nsRefPtr<sbGStreamerMessageHandler> mHandler;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbGstMessageEvent,
                              nsIRunnable)

GstBusSyncReply
SyncToAsyncDispatcher(GstBus* bus, GstMessage* message, gpointer data)
{
  sbGStreamerMessageHandler *handler =
    reinterpret_cast<sbGStreamerMessageHandler*>(data);

  // Allow a sync handler to look at this first.
  // If it returns false (the default), we dispatch it asynchronously.
  PRBool handled = handler->HandleSynchronousMessage(message);

  if (!handled) {
    nsCOMPtr<nsIRunnable> event = new sbGstMessageEvent(message, handler);
    NS_DispatchToMainThread(event);
  }

  gst_message_unref (message);

  return GST_BUS_DROP;
}

struct errMap {
  int gstErrorCode;
  long sbErrorCode;
  const char *sbErrorMessageName;
};

static const struct errMap ResourceErrorMap[] = {
  { GST_RESOURCE_ERROR_NOT_FOUND, sbIMediacoreError::SB_RESOURCE_NOT_FOUND,
      "mediacore.error.resource_not_found"},
  { GST_RESOURCE_ERROR_READ, sbIMediacoreError::SB_RESOURCE_READ,
      "mediacore.error.read_failed"},
  { GST_RESOURCE_ERROR_WRITE, sbIMediacoreError::SB_RESOURCE_WRITE,
      "mediacore.error.write_failed"},
  { GST_RESOURCE_ERROR_OPEN_READ, sbIMediacoreError::SB_RESOURCE_OPEN_READ,
      "mediacore.error.read_open_failed"},
  { GST_RESOURCE_ERROR_OPEN_WRITE, sbIMediacoreError::SB_RESOURCE_OPEN_WRITE,
      "mediacore.error.write_open_failed"},
  { GST_RESOURCE_ERROR_OPEN_READ_WRITE,
      sbIMediacoreError::SB_RESOURCE_OPEN_READ_WRITE,
      "mediacore.error.rw_open_failed"},
  { GST_RESOURCE_ERROR_CLOSE, sbIMediacoreError::SB_RESOURCE_CLOSE,
      "mediacore.error.close_failed"},
  { GST_RESOURCE_ERROR_SEEK, sbIMediacoreError::SB_RESOURCE_SEEK,
      "mediacore.error.seek_failed"},
  { GST_RESOURCE_ERROR_NO_SPACE_LEFT,
      sbIMediacoreError::SB_RESOURCE_NO_SPACE_LEFT,
      "mediacore.error.out_of_space"},
};

// No error codes for these yet
#if 0
static const struct errMap CoreErrorMap[] = {
};

static const struct errMap LibraryErrorMap[] = {
};
#endif

static const struct errMap StreamErrorMap[] = {
  { GST_STREAM_ERROR_TYPE_NOT_FOUND,
      sbIMediacoreError::SB_STREAM_TYPE_NOT_FOUND,
      "mediacore.error.type_not_found"},
  { GST_STREAM_ERROR_WRONG_TYPE, sbIMediacoreError::SB_STREAM_WRONG_TYPE,
      "mediacore.error.wrong_type"},
  { GST_STREAM_ERROR_CODEC_NOT_FOUND,
      sbIMediacoreError::SB_STREAM_CODEC_NOT_FOUND,
      "mediacore.error.codec_not_found"},
  { GST_STREAM_ERROR_DECODE, sbIMediacoreError::SB_STREAM_DECODE,
      "mediacore.error.decode_failed"},
  { GST_STREAM_ERROR_ENCODE, sbIMediacoreError::SB_STREAM_ENCODE,
      "mediacore.error.encode_failed"},
  { GST_STREAM_ERROR_FAILED, sbIMediacoreError::SB_STREAM_FAILURE,
      "mediacore.error.failure"},
};

nsresult
GetMediacoreErrorFromGstError(GError *gerror, nsString aResource,
        sbIMediacoreError **_retval)
{
  nsString errorMessage;
  nsRefPtr<sbMediacoreError> error;
  int errorCode = 0;
  const char *stringName = NULL;
  nsresult rv;

  NS_NEWXPCOM(error, sbMediacoreError);
  NS_ENSURE_TRUE (error, NS_ERROR_OUT_OF_MEMORY);

  const struct errMap *map = NULL;
  int mapLength;
#if 0
  if (gerror->domain == GST_CORE_ERROR) {
    map = CoreErrorMap;
    mapLength = NS_ARRAY_LENGTH(CoreErrorMap);
  }
  else if (gerror->domain == GST_LIBRARY_ERROR) {
    map = LibraryErrorMap;
    mapLength = NS_ARRAY_LENGTH(LibraryErrorMap);
  }
  else 
#endif
  if (gerror->domain == GST_RESOURCE_ERROR) {
    map = ResourceErrorMap;
    mapLength = NS_ARRAY_LENGTH(ResourceErrorMap);
  }
  else if (gerror->domain == GST_STREAM_ERROR) {
    map = StreamErrorMap;
    mapLength = NS_ARRAY_LENGTH(StreamErrorMap);
  }
  
  if (map) {
    for (int i = 0; i < mapLength; i++) {
      if (map[i].gstErrorCode == gerror->code) {
        errorCode = map[i].sbErrorCode;
        stringName = map[i].sbErrorMessageName;
        break;
      }
    }
  }

  if (stringName) {
    sbStringBundle bundle;
    nsTArray<nsString> params;
    if (aResource.IsEmpty()) {
      params.AppendElement(bundle.Get("mediacore.error.unknown_resource"));
    }
    else {
      // Unescape the resource.
      nsCOMPtr<nsINetUtil> netUtil =
        do_CreateInstance("@mozilla.org/network/util;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCString unescapedResource;
      rv = netUtil->UnescapeString(NS_ConvertUTF16toUTF8(aResource),
                                   nsINetUtil::ESCAPE_ALL, unescapedResource);
      NS_ENSURE_SUCCESS(rv, rv);

      params.AppendElement(NS_ConvertUTF8toUTF16(unescapedResource));
    }

    errorMessage = bundle.Format(stringName, params);
  }

  if (errorMessage.IsEmpty()) {
    // If we get here just fall back to the gerror's internal message,
    // untranslated.
    CopyUTF8toUTF16(nsDependentCString(gerror->message), errorMessage);
  }

  rv = error->Init(errorCode, errorMessage);
  NS_ENSURE_SUCCESS (rv, rv);

  NS_ADDREF (*_retval = error);

  return NS_OK;
}

typedef struct {
  GstCaps *srccaps;
  const char *type;
} TypeMatchingInfo;

static gboolean
match_element_filter (GstPluginFeature * feature, TypeMatchingInfo * data)
{
  const gchar *klass;
  const GList *templates;
  GList *walk;
  GstElementFactory * factory;
  const char *name;

  /* we only care about element factories */
  if (!GST_IS_ELEMENT_FACTORY (feature))
    return FALSE;

  factory = GST_ELEMENT_FACTORY (feature);

  klass = gst_element_factory_get_klass (factory);

  if (strstr (klass, data->type) == NULL) {
    /* Wrong type, don't check further */
    return FALSE;
  }

  /* Blacklist ffmux and ffenc. We don't want to accidently use these on
     linux systems where they might be loaded from the system. */
  name = gst_plugin_feature_get_name (feature);
  if (strstr (name, "ffmux") != NULL ||
      strstr (name, "ffenc") != NULL)
    return FALSE;

  templates = gst_element_factory_get_static_pad_templates (factory);
  for (walk = (GList *) templates; walk; walk = g_list_next (walk)) {
    GstStaticPadTemplate *templ = (GstStaticPadTemplate *)(walk->data);

    /* Only want source pad templates */
    if (templ->direction == GST_PAD_SRC) {
      GstCaps *template_caps = gst_static_caps_get (&templ->static_caps);
      GstCaps *intersect;

      intersect = gst_caps_intersect (template_caps, data->srccaps);
      gst_caps_unref (template_caps);

      if (!gst_caps_is_empty (intersect)) {
        // Non-empty intersection: caps are usable
        gst_caps_unref (intersect);
        return TRUE;
      }
      gst_caps_unref (intersect);
    }
  }

  /* Didn't find a compatible pad */
  return FALSE;
}

const char *
FindMatchingElementName(const char *srcCapsString, const char *typeName)
{
  GstCaps *caps;
  GList *list, *walk;
  TypeMatchingInfo data;
  guint bestrank = 0;
  GstElementFactory *bestfactory = NULL;

  caps = gst_caps_from_string (srcCapsString);
  if (!caps)
    return NULL;

  data.srccaps = caps;
  data.type = typeName;

  list = gst_default_registry_feature_filter (
          (GstPluginFeatureFilter)match_element_filter, FALSE, &data);

  for (walk = list; walk; walk = g_list_next (walk)) {
    GstElementFactory *factory = GST_ELEMENT_FACTORY (walk->data);
    guint rank = gst_plugin_feature_get_rank (GST_PLUGIN_FEATURE (factory));

    // Find the highest-ranked element that we considered acceptable.
    if (!bestfactory || rank > bestrank) {
      bestfactory = factory;
      bestrank = rank;
    }
  }

  if (!bestfactory)
    return NULL;

  return gst_plugin_feature_get_name (GST_PLUGIN_FEATURE (bestfactory));
}

void
RegisterCustomTags()
{
  gst_tag_register (SB_GST_TAG_GRACENOTE_TAGID, GST_TAG_FLAG_META,
                  G_TYPE_STRING, "GN TagID", "Gracenote Tag ID", NULL);
  gst_tag_register (SB_GST_TAG_GRACENOTE_EXTENDED_DATA, GST_TAG_FLAG_META,
                  G_TYPE_STRING, "GN ExtData", "Gracenote Extended Data", NULL);
}

