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

#include <sbStandardProperties.h>
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
    const gchar * gstvalue = NS_ConvertUTF16toUTF8(value).BeginReading(); \
    gst_tag_list_add (taglist, GST_TAG_MERGE_APPEND, (gchar *)gstname, \
            gstvalue, NULL); \
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

  PROPERTY_CONVERT_STRING (SB_PROPERTY_ALBUMNAME, GST_TAG_ALBUM);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_ARTISTNAME, GST_TAG_ARTIST);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_TRACKNAME, GST_TAG_TITLE);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COMPOSERNAME, GST_TAG_COMPOSER);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_GENRE, GST_TAG_GENRE);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COMMENT, GST_TAG_COMMENT);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_ORIGINURL, GST_TAG_LOCATION);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COPYRIGHT, GST_TAG_COPYRIGHT);
  PROPERTY_CONVERT_STRING (SB_PROPERTY_COPYRIGHTURL, GST_TAG_COPYRIGHT_URI);
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

