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
#include <nsIWritablePropertyBag.h>

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
      if (!converted) {
        LOG(("Failed to convert property to tag"));
      }
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
  GStreamer::pipelineOp_t gstPipelineOp;
  long sbErrorCode;
  const char *sbErrorMessageName;
};

static const struct errMap ResourceErrorMap[] = {
  { GST_RESOURCE_ERROR_NOT_FOUND, GStreamer::OP_UNKNOWN, 
      sbIMediacoreError::SB_RESOURCE_NOT_FOUND, 
        "mediacore.error.resource_not_found"},
  { GST_RESOURCE_ERROR_READ, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_READ,
        "mediacore.error.read_failed"},
  { GST_RESOURCE_ERROR_WRITE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_WRITE,
        "mediacore.error.write_failed"},
  { GST_RESOURCE_ERROR_OPEN_READ, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_OPEN_READ,
        "mediacore.error.read_open_failed"},
  { GST_RESOURCE_ERROR_OPEN_WRITE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_OPEN_WRITE,
        "mediacore.error.write_open_failed"},
  { GST_RESOURCE_ERROR_OPEN_READ_WRITE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_OPEN_READ_WRITE,
        "mediacore.error.rw_open_failed"},
  { GST_RESOURCE_ERROR_CLOSE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_CLOSE,
        "mediacore.error.close_failed"},
  { GST_RESOURCE_ERROR_SEEK, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_RESOURCE_SEEK,
        "mediacore.error.seek_failed"},
  { GST_RESOURCE_ERROR_NO_SPACE_LEFT, GStreamer::OP_UNKNOWN,
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
  { GST_STREAM_ERROR_TYPE_NOT_FOUND, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_TYPE_NOT_FOUND,
        "mediacore.error.type_not_found"},
  { GST_STREAM_ERROR_WRONG_TYPE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_WRONG_TYPE,
        "mediacore.error.wrong_type"},
  { GST_STREAM_ERROR_CODEC_NOT_FOUND, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_CODEC_NOT_FOUND,
        "mediacore.error.codec_not_found"},
  { GST_STREAM_ERROR_DECODE, GStreamer::OP_TRANSCODING,
      sbIMediacoreError::SB_STREAM_DECODE,
        "mediacore.error.decode_for_transcode_failed" },
  { GST_STREAM_ERROR_DECODE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_DECODE,
        "mediacore.error.decode_failed"},
  { GST_STREAM_ERROR_ENCODE, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_ENCODE,
        "mediacore.error.encode_failed"},
  { GST_STREAM_ERROR_FAILED, GStreamer::OP_TRANSCODING,
      sbIMediacoreError::SB_STREAM_FAILURE,
        "mediacore.error.failure_during_transcode"},
  { GST_STREAM_ERROR_FAILED, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_FAILURE,
        "mediacore.error.failure"},
  { GST_STREAM_ERROR_DECRYPT, GStreamer::OP_UNKNOWN,
      sbIMediacoreError::SB_STREAM_FAILURE,
        "mediacore.error.decrypt"},
};

nsresult
GetMediacoreErrorFromGstError(GError *gerror, nsString aResource,
                              GStreamer::pipelineOp_t aPipelineOp,
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
    int match = -1;
    int bestMatch = -1;

    for (int i = 0; i < mapLength; i++) {
      if (map[i].gstErrorCode == gerror->code) {
        // Try and find an exact match.
        if (aPipelineOp != GStreamer::OP_UNKNOWN &&
            map[i].gstPipelineOp == aPipelineOp) {
          bestMatch = i;
          // bestMatch found, break out right away.
          break;
        }
        else {
          // We can't break out here because the bestMatch
          // may be coming later in the map.
          match = i;
        }
      }
    }
    
    if (bestMatch == -1 && match != -1) {
      bestMatch = match;
    }

    if (bestMatch != -1) {
      errorCode = map[bestMatch].sbErrorCode;
      stringName = map[bestMatch].sbErrorMessageName;
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

  caps = gst_caps_from_string (srcCapsString);
  if (!caps)
    return NULL;

  const char* result = FindMatchingElementName(caps, typeName);
  gst_caps_unref(caps);
  return result;
}

const char *
FindMatchingElementName(GstCaps *srcCaps, const char *typeName)
{
  GList *list, *walk;
  TypeMatchingInfo data;
  guint bestrank = 0;
  GstElementFactory *bestfactory = NULL;

  if (!srcCaps)
    return NULL;

  data.srccaps = srcCaps;
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

nsresult
ApplyPropertyBagToElement(GstElement *element, nsIPropertyBag *props)
{
  nsCOMPtr<nsISimpleEnumerator> enumerator;
  nsresult rv = props->GetEnumerator (getter_AddRefs (enumerator));
  NS_ENSURE_SUCCESS (rv, rv);

  PRBool hasMore = PR_FALSE;
  while (NS_SUCCEEDED (enumerator->HasMoreElements (&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> next;
    rv = enumerator->GetNext (getter_AddRefs (next));
    NS_ENSURE_SUCCESS (rv, rv);

    nsCOMPtr<nsIProperty> property = do_QueryInterface (next, &rv);
    NS_ENSURE_SUCCESS (rv, rv);

    nsString propertyName;
    rv = property->GetName (propertyName);
    NS_ENSURE_SUCCESS (rv, rv);

    nsCOMPtr<nsIVariant> propertyVariant;
    rv = property->GetValue (getter_AddRefs (propertyVariant));
    NS_ENSURE_SUCCESS (rv, rv);

    NS_ConvertUTF16toUTF8 propertyNameUTF8 (propertyName);

    GParamSpec *paramSpec = g_object_class_find_property (
            G_OBJECT_GET_CLASS (element),
            propertyNameUTF8.BeginReading());
    if (!paramSpec)
    {
      LOG(("No such property %s on element", propertyNameUTF8.BeginReading()));
      return NS_ERROR_FAILURE;
    }

    PRUint16 variantType;
    rv = propertyVariant->GetDataType(&variantType);
    NS_ENSURE_SUCCESS (rv, rv);

    GValue propertyValue = { 0 };
    if (paramSpec->value_type == G_TYPE_INT)
    {
      PRInt32 val;
      rv = propertyVariant->GetAsInt32 (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_INT);
      g_value_set_int (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_UINT)
    {
      PRUint32 val;
      rv = propertyVariant->GetAsUint32 (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_UINT);
      g_value_set_uint (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_UINT64)
    {
      PRUint64 val;
      rv = propertyVariant->GetAsUint64 (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_UINT64);
      g_value_set_uint64 (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_INT64)
    {
      PRInt64 val;
      rv = propertyVariant->GetAsInt64 (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_INT64);
      g_value_set_int64 (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_BOOLEAN)
    {
      PRBool val;
      rv = propertyVariant->GetAsBool (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_BOOLEAN);
      g_value_set_boolean (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_FLOAT)
    {
      float val;
      rv = propertyVariant->GetAsFloat (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_FLOAT);
      g_value_set_float (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_DOUBLE)
    {
      double val;
      rv = propertyVariant->GetAsDouble (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_DOUBLE);
      g_value_set_float (&propertyValue, val);
    }
    else if (paramSpec->value_type == G_TYPE_STRING)
    {
      nsCString val;
      rv = propertyVariant->GetAsACString(val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, G_TYPE_STRING);
      g_value_set_string (&propertyValue, val.BeginReading());
    }
    else if (G_TYPE_IS_ENUM (paramSpec->value_type))
    {
      PRUint32 val;
      rv = propertyVariant->GetAsUint32 (&val);
      NS_ENSURE_SUCCESS (rv, rv);
      g_value_init (&propertyValue, paramSpec->value_type);
      g_value_set_enum (&propertyValue, val);
    }
    else {
      LOG(("Unsupported property type"));
      return NS_ERROR_FAILURE;
    }

    g_object_set_property (G_OBJECT (element),
                           propertyNameUTF8.BeginReading(),
                           &propertyValue);
    g_value_unset (&propertyValue);
  }

  return NS_OK;
}

struct sb_gst_caps_map_entry {
  const char *mime_type;
  const char *gst_name;
  enum sbGstCapsMapType map_type;
};

static const struct sb_gst_caps_map_entry sb_gst_caps_map[] =
{
  // mpeg audio we always want id3 tags on for the container
  { "audio/mpeg",        "application/x-id3", SB_GST_CAPS_MAP_CONTAINER },
  // Quicktime video variants
  { "video/3gpp",        "video/quicktime", SB_GST_CAPS_MAP_CONTAINER },
  { "video/mp4",         "video/quicktime", SB_GST_CAPS_MAP_CONTAINER },

  { "audio/x-pcm-int",   "audio/x-raw-int", SB_GST_CAPS_MAP_AUDIO },
  { "audio/x-pcm-float", "audio/x-raw-float", SB_GST_CAPS_MAP_AUDIO },
  { "audio/x-ms-wma",    "audio/x-wma", SB_GST_CAPS_MAP_AUDIO },
  { "audio/mpeg",        "audio/mpeg", SB_GST_CAPS_MAP_AUDIO },
  { "audio/aac",         "audio/mpeg", SB_GST_CAPS_MAP_AUDIO },

  { "video/x-ms-wmv",    "video/x-wmv", SB_GST_CAPS_MAP_VIDEO },
  { "video/h264",        "video/x-h264", SB_GST_CAPS_MAP_VIDEO },

  // The remaining ones have NONE type; they should ONLY be used for mapping
  // a GST type to a mime type, not the other way around. It's ok for duplicate
  // mime types (the first column) here, since they're not used for mapping
  // mime types to caps.

  // MPEG4 video variants; treat them all as video/mpeg
  { "video/mpeg",        "video/x-divx", SB_GST_CAPS_MAP_NONE },
  { "video/mpeg",        "video/x-xvid", SB_GST_CAPS_MAP_NONE },

  // Quicktime file format variants, map them to the appropriate type
  { "video/mp4",         "audio/x-m4a", SB_GST_CAPS_MAP_NONE },
  { "video/3gpp",        "application/x-3gp", SB_GST_CAPS_MAP_NONE },
};

/* GStreamer caps name are generally similar to mime-types, but some of them
 * differ. We use this table to convert the ones that differ that we know about,
 * and all other names are returned unchanged.
 */
static nsCString
GetGstCapsName(const nsACString &aMimeType, enum sbGstCapsMapType aType)
{
  nsCString result(aMimeType);

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH (sb_gst_caps_map); i++) {
    if (sb_gst_caps_map[i].map_type == aType &&
        aMimeType.EqualsLiteral(sb_gst_caps_map[i].mime_type)) 
    {
      result.AssignLiteral(sb_gst_caps_map[i].gst_name);
      return result;
    }
  }

  return result;
}

GstCaps *
GetCapsForMimeType (const nsACString &aMimeType, enum sbGstCapsMapType aType )
{
  nsCString name = GetGstCapsName (aMimeType, aType);
  // set up a caps structure. Note that this ONLY provides the name part of the
  // structure, it does NOT add attributes to the caps that may be required.
  GstCaps *caps = gst_caps_from_string(name.BeginReading());
  return caps;
}


nsresult
GetMimeTypeForCaps (GstCaps *aCaps, nsACString &aMimeType)
{
  GstStructure *structure = gst_caps_get_structure (aCaps, 0);
  const gchar *capsName = gst_structure_get_name (structure);

  // Need to special-case some of these
  if (!strcmp(capsName, "video/quicktime")) {
    const gchar *variant = gst_structure_get_string (structure, "variant");
    if (variant) {
      if (!strcmp(variant, "3gpp"))
        aMimeType.AssignLiteral("video/3gpp");
      else if (!strcmp(variant, "iso"))
        aMimeType.AssignLiteral("video/mp4");
      else // variant is 'apple' or something we don't recognise; use quicktime.
        aMimeType.AssignLiteral("video/quicktime");
    }
    else {
      // No variant; use quicktime
      aMimeType.AssignLiteral("video/quicktime");
    }
    return NS_OK;
  }
  else if (!strcmp(capsName, "audio/mpeg")) {
    gint mpegversion;
    if (gst_structure_get_int (structure, "mpegversion", &mpegversion) &&
        mpegversion == 4) {
      aMimeType.AssignLiteral("audio/aac");
    }
    else {
      aMimeType.AssignLiteral("audio/mpeg");
    }
    return NS_OK;
  }

  for (unsigned int i = 0; i < NS_ARRAY_LENGTH (sb_gst_caps_map); i++)
  {
    if (!strcmp(capsName, sb_gst_caps_map[i].gst_name)) {
      aMimeType.AssignLiteral(sb_gst_caps_map[i].mime_type);
      return NS_OK;
    }
  }

  /* Use the same name if we don't have a different mapping */
  aMimeType.AssignLiteral(capsName);
  return NS_OK;

}


// A GstStructureForeachFunc.  Adds each GValue to the nsIWritablePropertyBag2
// (in aUserData) using the property name given by aFieldId
static gboolean
_OnEachGValue(GQuark aFieldId, const GValue *aValue, gpointer aUserData)
{
  const gchar * fieldname = g_quark_to_string(aFieldId);

  nsresult rv;
  rv = SetPropertyFromGValue(
        static_cast<nsIWritablePropertyBag2 *>(aUserData),
        NS_ConvertASCIItoUTF16(fieldname),
        aValue);
  NS_ENSURE_SUCCESS(rv, FALSE);
  
  return TRUE;
}


nsresult
SetPropertiesFromGstStructure(nsIWritablePropertyBag2 * aPropertyBag,
                              const GstStructure * aStructure,
                              const gchar * const aDesiredFieldList[],
                              PRUint32 aFieldCount)
{
  nsresult rv;

  // Use the list of field names if present.  Otherwise, fall through
  // and copy every field of aStructure to aPropertyBag
  if (aDesiredFieldList) {
    for (PRUint32 i = 0; i < aFieldCount; i++) {
      const gchar * fieldname = aDesiredFieldList[i];
      const GValue * value = gst_structure_get_value(aStructure, fieldname);

      // Silently skip missing fields
      if (value) {
        rv = SetPropertyFromGValue(aPropertyBag,
                                   NS_ConvertASCIItoUTF16(fieldname),
                                   value);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    return NS_OK;
  }

  // If the field list is NULL, its count must be zero
  NS_ENSURE_TRUE(aFieldCount == 0, NS_ERROR_INVALID_ARG);

  gboolean ok = gst_structure_foreach(aStructure, _OnEachGValue, aPropertyBag);
  NS_ENSURE_TRUE(ok, NS_ERROR_FAILURE);

  return NS_OK;
}


nsresult
SetPropertyFromGValue(nsIWritablePropertyBag2 * aPropertyBag,
                      const nsAString & aProperty,
                      const GValue * aValue)
{
  NS_ENSURE_ARG_POINTER(aPropertyBag);

  nsresult rv;
  GType type = G_VALUE_TYPE(aValue);
  const gchar *asGstr = NULL;
  gboolean asGboolean;
  gint asGint;
  guint asGuint;

  switch (type) {
    case G_TYPE_BOOLEAN:
      asGboolean = g_value_get_boolean (aValue);
      rv = aPropertyBag->SetPropertyAsBool(aProperty, asGboolean);
      NS_ENSURE_SUCCESS (rv, rv);
      break;

    case G_TYPE_INT:
      asGint = g_value_get_int (aValue);
      rv = aPropertyBag->SetPropertyAsInt32(aProperty, asGint);
      NS_ENSURE_SUCCESS (rv, rv);
      break;

    case G_TYPE_UINT:
      asGuint = g_value_get_uint (aValue);
      rv = aPropertyBag->SetPropertyAsUint32(aProperty, asGuint);
      NS_ENSURE_SUCCESS (rv, rv);
      break;

    case G_TYPE_STRING:
      asGstr = g_value_get_string (aValue);
      NS_ENSURE_TRUE(asGstr, NS_ERROR_UNEXPECTED);
      rv = aPropertyBag->SetPropertyAsACString(aProperty, nsCString(asGstr));
      NS_ENSURE_SUCCESS (rv, rv);
      break;

    default:
      // Handle any types that have dynamic type identifiers.  They can't be
      // used as case labels because they aren't constant expressions

      if (type == GST_TYPE_BUFFER) {
        const GstBuffer * asGstBuffer = gst_value_get_buffer(aValue);
        NS_ENSURE_TRUE(asGstBuffer, NS_ERROR_UNEXPECTED);

        // Create a variant to hold the buffer data as an array of VTYPE_UINT8s
        nsCOMPtr<nsIWritableVariant> asVariant =
          do_CreateInstance("@mozilla.org/variant;1", &rv);
        NS_ENSURE_SUCCESS (rv, rv);
        rv = asVariant->SetAsArray(nsIDataType::VTYPE_UINT8,
                                   nsnull,
                                   GST_BUFFER_SIZE(asGstBuffer),
                                   GST_BUFFER_DATA(asGstBuffer));
        NS_ENSURE_SUCCESS (rv, rv);

        // QI to an nsIWritablePropertyBag to access the generic SetProperty()
        // method
        nsCOMPtr<nsIWritablePropertyBag> bagV1 =
          do_QueryInterface(aPropertyBag, &rv);
        NS_ENSURE_SUCCESS (rv, rv);
        rv = bagV1->SetProperty(aProperty, asVariant);
        NS_ENSURE_SUCCESS (rv, rv);
      }
      else {
        // Unexpected data type
#ifdef PR_LOGGING
        const gchar *typeName = g_type_name(type);
        LOG(("Unexpected GType [%u %s] for [%s]",
             unsigned(type),
             typeName,
             NS_LossyConvertUTF16toASCII(aProperty).get()));
#endif // #ifdef PR_LOGGING
        NS_ENSURE_TRUE(PR_FALSE, NS_ERROR_ILLEGAL_VALUE);
      }
      break;
  }

  return NS_OK;
}
