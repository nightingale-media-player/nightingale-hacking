/* vim: set sw=2 : */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-20010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

///// Class header include
#include "sbGStreamerTranscodeAudioConfigurator.h"

///// Gecko interface includes
#include <nsIArray.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIMutableArray.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>

///// Nightingale interface includes
#include <sbIDevice.h>
#include <sbIDeviceCapabilities.h>
#include <sbIMediaFormatMutable.h>
#include <sbITranscodeError.h>
#include <sbITranscodeManager.h>
#include <sbITranscodeProfile.h>

///// Gecko header includes
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsArrayUtils.h>
#include <prlog.h>

///// Nightingale header includes
#include <sbArrayUtils.h>
#include <sbMemoryUtils.h>
#include <sbStringUtils.h>
#include <sbTranscodeUtils.h>
#include <sbVariantUtils.h>
#include <sbFraction.h>
#include <sbPrefBranch.h>

///// GStreamer header includes
#if _MSC_VER
#  pragma warning (push)
#  pragma warning (disable: 4244)
#endif /* _MSC_VER */
#include <gst/gst.h>
#if _MSC_VER
#  pragma warning (pop)
#endif /* _MSC_VER */

///// System includes
#include <algorithm>
#include <functional>
#include <vector>

///// Local directory includes
#include "sbGStreamerMediacoreUtils.h"

///// NSPR Logging
/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbGStreamerTranscodeAudioConfigurator:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGstTranscodeConfiguratorLog = nsnull;
#define TRACE(args) PR_LOG(gGstTranscodeConfiguratorLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gGstTranscodeConfiguratorLog, PR_LOG_WARN, args)
#if __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

///// Helper functions

/**
 * Get a value in a sbIDevCapRange that is no smaller than the given target
 * value, unless it's larger than the maximum (in which case return that)
 *
 * @param aRange the range to look at
 * @param aTarget the value we want
 * @return a value in range that's no smaller than the target;
 *         or, the maximum in the range and throw
 *         NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA
 */
static nsresult
GetDevCapRangeUpper(sbIDevCapRange *aRange, PRInt32 aTarget, PRInt32 *_retval)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aRange);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRUint32 count;
  PRInt32 result = PR_INT32_MIN, max = PR_INT32_MIN;
  rv = aRange->GetValueCount(&count);
  NS_ENSURE_SUCCESS(rv, rv);
  if (count > 0) {
    for (PRUint32 i = 0; i < count; ++i) {
      PRInt32 v;
      rv = aRange->GetValue(i, &v);
      NS_ENSURE_SUCCESS(rv, rv);
      if ((result < aTarget || v < result) && v >= aTarget) {
        result = v;
      }
      else if (max < v) {
        max = v;
      }
    }
    if (result >= aTarget) {
      *_retval = result;
      return NS_OK;
    }
    else {
      *_retval = max;
      return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
    }
  }

  // no count, use min + step + max
  PRInt32 min, step;
  rv  = aRange->GetMin(&min);
  rv |= aRange->GetStep(&step);
  rv |= aRange->GetMax(&max);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(step > 0, NS_ERROR_UNEXPECTED);
  if (max < aTarget) {
    *_retval = max;
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  if (min > aTarget) {
    *_retval = min;
    return NS_OK;
  }
  // _retval = min + ceil((target - min) / step) * step
  *_retval = min + (aTarget - min + step - 1) / step * step;
  return NS_OK;

}

///// Class declarations
NS_IMPL_ISUPPORTS_INHERITED1(sbGStreamerTranscodeAudioConfigurator,
                             sbTranscodingConfigurator,
                             sbIDeviceTranscodingConfigurator);

sbGStreamerTranscodeAudioConfigurator::sbGStreamerTranscodeAudioConfigurator() :
    mProfileFromPrefs(PR_FALSE),
    mProfileFromGlobalPrefs(PR_FALSE)
{
  #if PR_LOGGING
    if (!gGstTranscodeConfiguratorLog) {
      gGstTranscodeConfiguratorLog =
        PR_NewLogModule("sbGStreamerTranscodeAudioConfigurator");
    }
  #endif /* PR_LOGGING */
  TRACE(("%s[%p]", __FUNCTION__, this));
  /* nothing */
}

sbGStreamerTranscodeAudioConfigurator::~sbGStreamerTranscodeAudioConfigurator()
{
  /* nothing */
}

/**
 * make a GstCaps structure from a caps name and an array of attributes
 *
 * @param aType [in] type of caps we're converting
 * @param aMimeType [in] the name (e.g. "audio/x-pcm-int")
 * @param aAttributes [in] the attributes
 * @param aResultCaps [out] the generated GstCaps, with an outstanding refcount
 */
static nsresult
MakeCapsFromAttributes(enum sbGstCapsMapType aType,
                       const nsACString& aMimeType,
                       nsIArray *aAttributes,
                       GstCaps** aResultCaps)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aAttributes);
  NS_ENSURE_ARG_POINTER(aResultCaps);

  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> attrsEnum;
  rv = aAttributes->Enumerate(getter_AddRefs(attrsEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  sbGstCaps caps = GetCapsForMimeType (aMimeType, aType);
  NS_ENSURE_TRUE(caps, NS_ERROR_FAILURE);
  GstStructure* capsStruct = gst_caps_get_structure(caps.get(), 0);

  PRBool hasMore;
  while (NS_SUCCEEDED(rv = attrsEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> attrSupports;
    rv = attrsEnum->GetNext(getter_AddRefs(attrSupports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbITranscodeProfileAttribute> attr =
      do_QueryInterface(attrSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsString attrName;
    rv = attr->GetName(attrName);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIVariant> attrValue;
    rv = attr->GetValue(getter_AddRefs(attrValue));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint16 attrType;
    rv = attrValue->GetDataType(&attrType);
    NS_ENSURE_SUCCESS(rv, rv);
    switch(attrType) {
      case nsIDataType::VTYPE_INT8:    case nsIDataType::VTYPE_UINT8:
      case nsIDataType::VTYPE_INT16:   case nsIDataType::VTYPE_UINT16:
      case nsIDataType::VTYPE_INT32:   case nsIDataType::VTYPE_UINT32:
      case nsIDataType::VTYPE_INT64:   case nsIDataType::VTYPE_UINT64:
      {
        PRInt32 intValue;
        rv = attrValue->GetAsInt32(&intValue);
        NS_ENSURE_SUCCESS(rv, rv);
        gst_structure_set(capsStruct,
                          NS_LossyConvertUTF16toASCII(attrName).get(),
                          G_TYPE_INT,
                          intValue,
                          NULL);
        break;
      }
      case nsIDataType::VTYPE_UTF8STRING:
      case nsIDataType::VTYPE_DOMSTRING:
      case nsIDataType::VTYPE_CSTRING:
      case nsIDataType::VTYPE_ASTRING:
      {
        nsCString stringValue;
        rv = attrValue->GetAsACString(stringValue);
        NS_ENSURE_SUCCESS (rv, rv);

        gst_structure_set(capsStruct,
                          NS_LossyConvertUTF16toASCII(attrName).get(),
                          G_TYPE_STRING,
                          stringValue.BeginReading(),
                          NULL);
        break;
      }
      default:
        NS_NOTYETIMPLEMENTED("Unknown attribute type");
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);

  *aResultCaps = caps.forget();
  return NS_OK;
}

/* Check that the profile is the right type, and that we have appropriate
   GStreamer elements to use it. Returns NS_OK if the profile appears to be
   useable.
 */
nsresult
sbGStreamerTranscodeAudioConfigurator::EnsureProfileAvailable(sbITranscodeProfile *aProfile)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aProfile);

  nsresult rv;

  // Only get audio profiles
  PRUint32 type;
  rv = aProfile->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);

  if (type != sbITranscodeProfile::TRANSCODE_TYPE_AUDIO)
    return NS_ERROR_NOT_AVAILABLE;

  EncoderProfileData elementNames;

  // check that we have a muxer available
  nsString capsName;
  rv = aProfile->GetContainerFormat(capsName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!capsName.IsEmpty()) {
    nsCOMPtr<nsIArray> attrs;
    rv = aProfile->GetContainerAttributes(getter_AddRefs(attrs));
    NS_ENSURE_SUCCESS(rv, rv);

    GstCaps* caps = NULL;
    rv = MakeCapsFromAttributes(SB_GST_CAPS_MAP_CONTAINER,
                                NS_LossyConvertUTF16toASCII(capsName),
                                attrs,
                                &caps);
    NS_ENSURE_SUCCESS(rv, rv);

    const char* muxerCodecName = FindMatchingElementName(caps, "Muxer");
    if (!muxerCodecName) {
      // things like id3 are called Formatter instead, but are the same for
      // our purposes
      muxerCodecName = FindMatchingElementName(caps, "Formatter");
    }
    gst_caps_unref(caps);
    if (!muxerCodecName) {
      TRACE(("no muxer available for %s",
             NS_LossyConvertUTF16toASCII(capsName).get()));
      return NS_ERROR_UNEXPECTED;
    }
    elementNames.muxer = muxerCodecName;
  }

  /// Check that we have an audio encoder available
  rv = aProfile->GetAudioCodec(capsName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!capsName.IsEmpty()) {
    nsCOMPtr<nsIArray> attrs;
    rv = aProfile->GetAudioAttributes(getter_AddRefs(attrs));
    NS_ENSURE_SUCCESS(rv, rv);

    GstCaps* caps = NULL;
    rv = MakeCapsFromAttributes(SB_GST_CAPS_MAP_AUDIO,
                                NS_LossyConvertUTF16toASCII(capsName),
                                attrs,
                                &caps);
    NS_ENSURE_SUCCESS(rv, rv);

    const char* audioEncoder = FindMatchingElementName(caps, "Encoder");
    gst_caps_unref(caps);
    if (!audioEncoder) {
      TRACE(("no audio encoder available for %s",
             NS_LossyConvertUTF16toASCII(capsName).get()));
      return NS_ERROR_UNEXPECTED;
    }
    elementNames.audioEncoder = audioEncoder;
  }

  PRBool success = mElementNames.Put(aProfile, elementNames);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

/* Check if this profile is useable by the device.
 * 
 * If successful, aFormat will contain the format object constraining the
 * device appropriate for this profile.
 */
nsresult
sbGStreamerTranscodeAudioConfigurator::CheckProfileSupportedByDevice(
        sbITranscodeProfile *aProfile,
        sbIAudioFormatType **aFormat)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // get the device caps
  nsresult rv;
  nsCOMPtr<sbIDeviceCapabilities> caps;
  rv = mDevice->GetCapabilities(getter_AddRefs(caps));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mimeTypesCount;
  char **mimeTypes;
  rv = caps->GetSupportedMimeTypes(sbIDeviceCapabilities::CONTENT_AUDIO,
                                   &mimeTypesCount,
                                   &mimeTypes);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mimeTypesCount == 0) {
    // If our device supports zero audio formats, but we're being asked to do
    // an audio transcode, then just accept all profiles without a specific
    // format type. This is used for CD ripping, for example.
    *aFormat = NULL;
    return NS_OK;
  }

  sbAutoFreeXPCOMArrayByRef<char**> autoMimeTypes(mimeTypesCount, mimeTypes);

  for (PRUint32 mimeTypeIndex = 0;
       mimeTypeIndex < mimeTypesCount;
       ++mimeTypeIndex)
  {
    nsISupports** formatTypes;
    PRUint32 formatTypeCount;
    rv = caps->GetFormatTypes(sbIDeviceCapabilities::CONTENT_AUDIO,
                              NS_ConvertASCIItoUTF16(mimeTypes[mimeTypeIndex]),
                              &formatTypeCount,
                              &formatTypes);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoFreeXPCOMPointerArray<nsISupports> freeFormats(formatTypeCount,
                                                         formatTypes);

    for (PRUint32 formatIndex = 0;
         formatIndex < formatTypeCount;
         formatIndex++)
    {
      nsCOMPtr<sbIAudioFormatType> format = do_QueryInterface(
          formatTypes[formatIndex], &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      // check the container
      nsCString formatContainer;
      rv = format->GetContainerFormat(formatContainer);
      NS_ENSURE_SUCCESS(rv, rv);
      nsString encoderContainer;
      rv = aProfile->GetContainerFormat(encoderContainer);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!encoderContainer.Equals(NS_ConvertUTF8toUTF16(formatContainer))) {
        // mismatch, try the next device format
        continue;
      }

      // check the audio codec
      nsString encoderAudioCodec;
      rv = aProfile->GetAudioCodec(encoderAudioCodec);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCString formatAudioCodec;
      rv = format->GetAudioCodec(formatAudioCodec);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!encoderAudioCodec.Equals(NS_ConvertUTF8toUTF16(formatAudioCodec))) {
        // mismatch, try the next device format
        continue;
      }

      // If we got to here, it's compatible.
      NS_ADDREF (*aFormat = format);
      return NS_OK;
    }
  }

  return NS_ERROR_FAILURE;
}

/**
 * Select the encoding profile to use
 *
 * @precondition the device has been set
 * @postcondition mSelectedProfile is the encoder profile to use
 * @postcondition mSelectedFormat is the device format matching the profile (or
 *                                NULL if no specific format is in use)
 */
nsresult
sbGStreamerTranscodeAudioConfigurator::SelectProfile()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_PRECONDITION(mDevice,
                  "attempted to select profile with no device");

  nsCOMPtr<sbIAudioFormatType> prefFormat;
  nsCOMPtr<sbIAudioFormatType> globalPrefFormat;
  nsCOMPtr<sbIAudioFormatType> bestFormat;
  nsCOMPtr<sbITranscodeProfile> prefProfile;
  nsCOMPtr<sbITranscodeProfile> globalPrefProfile;
  nsCOMPtr<sbITranscodeProfile> bestProfile;
  PRBool hasProfilePref = PR_FALSE;
  PRBool hasGlobalProfilePref = PR_FALSE;
  PRUint32 bestPriority = 0;
  nsresult rv;

  // See if we have a preference for the transcoding profile. Check for a device
  // specific preference, or if not present, look for a global one.
  nsString prefProfileId;
  nsString globalPrefProfileId;

  nsCOMPtr<nsIVariant> profileIdVariant;
  rv = mDevice->GetPreference(NS_LITERAL_STRING("transcode_profile.profile_id"),
                              getter_AddRefs(profileIdVariant));
  if (NS_SUCCEEDED(rv))
  {
    rv = profileIdVariant->GetAsAString(prefProfileId);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!prefProfileId.IsEmpty()) {
      TRACE(("%s: found a profile", __FUNCTION__));
      hasProfilePref = PR_TRUE;
    }
  }

  if (!hasProfilePref) {
    sbPrefBranch prefs("nightingale.device.transcode_profile.", &rv);
    NS_ENSURE_SUCCESS (rv, rv);

    rv = prefs.GetPreference(NS_LITERAL_STRING("profile_id"),
                             getter_AddRefs(profileIdVariant));
    if (NS_SUCCEEDED(rv)) {
      rv = profileIdVariant->GetAsAString(globalPrefProfileId);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!globalPrefProfileId.IsEmpty()) {
        TRACE(("%s: found a global pref for profile", __FUNCTION__));
        hasGlobalProfilePref = PR_TRUE;
      }
    }
  }

  // get available profiles (these are the ones that we actually have
  // appropriate encoders/etc for)
  nsCOMPtr<nsIArray> profilesArray;
  rv = GetAvailableProfiles(getter_AddRefs(profilesArray));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> profilesEnum;
  rv = profilesArray->Enumerate(getter_AddRefs(profilesEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // Find the profile from the pref, or otherwise the highest-ranked one.
  PRBool hasMoreProfiles;
  while (NS_SUCCEEDED(profilesEnum->HasMoreElements(&hasMoreProfiles)) &&
         hasMoreProfiles)
  {
    nsCOMPtr<nsISupports> profileSupports;
    rv = profilesEnum->GetNext(getter_AddRefs(profileSupports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbITranscodeProfile> profile =
      do_QueryInterface(profileSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString profileId;
    rv = profile->GetId(profileId);
    NS_ENSURE_SUCCESS(rv, rv);

    if (hasProfilePref) {
      if (profileId.Equals(prefProfileId)) {
        nsCOMPtr<sbIAudioFormatType> deviceFormat;
        rv = CheckProfileSupportedByDevice (profile,
                                            getter_AddRefs(deviceFormat));
        if (NS_SUCCEEDED (rv)) {
          prefProfile = profile;
          prefFormat = deviceFormat;
        }
      }
    }

    if (hasGlobalProfilePref) {
      if (profileId.Equals(globalPrefProfileId)) {
        nsCOMPtr<sbIAudioFormatType> deviceFormat;
        rv = CheckProfileSupportedByDevice (profile,
                                            getter_AddRefs(deviceFormat));
        if (NS_SUCCEEDED (rv)) {
          globalPrefProfile = profile;
          globalPrefFormat = deviceFormat;
        }
      }
    }

    // Also track the highest-priority profile. This is our default if there is
    // no preference set or the preferenced profile is unavailable.
    PRUint32 priority;
    rv = profile->GetPriority(&priority);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!bestProfile || priority > bestPriority) {
      nsCOMPtr<sbIAudioFormatType> deviceFormat;
      rv = CheckProfileSupportedByDevice (profile,
                                          getter_AddRefs(deviceFormat));
      if (NS_SUCCEEDED (rv)) {
        bestProfile = profile;
        bestPriority = priority;
        bestFormat = deviceFormat;
      }
    }
  }

  if (prefProfile) {
    LOG(("Using device pref profile"));
    mProfileFromPrefs = PR_TRUE;
    mSelectedProfile = prefProfile;
    mSelectedFormat = prefFormat;
  }
  else if (globalPrefProfile) {
    LOG(("Using global pref profile"));
    mProfileFromGlobalPrefs = PR_TRUE;
    mSelectedProfile = globalPrefProfile;
    mSelectedFormat = globalPrefFormat;
  }
  else if (bestProfile) {
    LOG(("Using best available profile"));
    mSelectedProfile = bestProfile;
    mSelectedFormat = bestFormat;
  }
  else {
    // No usable profiles at all.
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

/**
 * Select and set the output format (sample rate/channels/etc)
 */
nsresult
sbGStreamerTranscodeAudioConfigurator::SelectOutputAudioFormat()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_PRECONDITION(mSelectedProfile,
                  "attempted to set audio properties without selecting profile");

  nsresult rv;

  if (!mAudioFormat) {
    mAudioFormat = do_CreateInstance(SB_MEDIAFORMATAUDIO_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIMediaFormatAudioMutable> audioFormat =
    do_QueryInterface(mAudioFormat, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaFormatAudio> inputFormat;
  rv = mInputFormat->GetAudioStream(getter_AddRefs(inputFormat));
  NS_ENSURE_SUCCESS(rv, rv);

  // This shouldn't be called if we don't have audio input
  if (!inputFormat)
    return NS_ERROR_UNEXPECTED;

  if (!mSelectedFormat) {
    // If we don't have a selected format, we force the output rate/channels
    // to be the same as the input
    PRInt32 sampleRate;
    rv = inputFormat->GetSampleRate(&sampleRate);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = audioFormat->SetSampleRate(sampleRate);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 channels;
    rv = inputFormat->GetChannels(&channels);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = audioFormat->SetChannels(channels);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // Otherwise, we have a selected format: force these in range.

  PRBool isInRange;

  nsCOMPtr<sbIDevCapRange> sampleRateRange;
  rv = mSelectedFormat->GetSupportedSampleRates(getter_AddRefs(sampleRateRange));
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 sampleRate;
  rv = inputFormat->GetSampleRate(&sampleRate);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = sampleRateRange->IsValueInRange(sampleRate, &isInRange);
  if (NS_FAILED(rv) || !isInRange) {
    // won't fit; pick anything for now
    rv = GetDevCapRangeUpper(sampleRateRange, sampleRate, &sampleRate);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = audioFormat->SetSampleRate(sampleRate);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> channelsRange;
  rv = mSelectedFormat->GetSupportedChannels(getter_AddRefs(channelsRange));
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 channels;
  rv = inputFormat->GetChannels(&channels);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = channelsRange->IsValueInRange(channels, &isInRange);
  if (NS_FAILED(rv) || !isInRange) {
    // won't fit; pick anything for now
    PRInt32 newChannels;
    rv = GetDevCapRangeUpper(channelsRange, channels, &newChannels);
    if (NS_SUCCEEDED(rv)) {
      channels = newChannels;
    }
    else {
      // no channel information; assume supports mono + stereo
      channels = (channels < 2 ? 1 : 2);
    }
  }
  rv = audioFormat->SetChannels(channels);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Set audio-related properties
 *
 * @precondition a profile has been selected
 * @postcondition mAudioEncoderProperties contains the audio properties desired
 */
nsresult
sbGStreamerTranscodeAudioConfigurator::SetAudioProperties()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_PRECONDITION(mSelectedProfile,
                  "attempted to set audio properties without selecting profile");

  nsresult rv;

  if (!mAudioEncoderProperties) {
    mAudioEncoderProperties =
      do_CreateInstance("@getnightingale.com/moz/xpcom/sbpropertybag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIWritablePropertyBag> writableBag =
    do_QueryInterface(mAudioEncoderProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> propsSrc;
  rv = mSelectedProfile->GetAudioProperties(getter_AddRefs(propsSrc));
  NS_ENSURE_SUCCESS(rv, rv);

  // If we selected the profile from the preferences, also apply the properties
  // set in those prefs.
  if (mProfileFromPrefs) {
    rv = ApplyPreferencesToPropertyArray(
            mDevice,
            propsSrc,
            NS_LITERAL_STRING("transcode_profile.audio_properties"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (mProfileFromGlobalPrefs) {
    rv = ApplyPreferencesToPropertyArray(
            nsnull,
            propsSrc,
            NS_LITERAL_STRING("nightingale.device.transcode_profile.audio_properties"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CopyPropertiesIntoBag(propsSrc, writableBag, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerTranscodeAudioConfigurator::ApplyPreferencesToPropertyArray(
        sbIDevice *aDevice,
        nsIArray  *aPropertyArray,
        nsString aPrefNameBase)
{
  nsresult rv;

  if(!aPropertyArray) {
    // Perfectly ok; this just means there aren't any properties.
    return NS_OK;
  }

  PRUint32 numProperties;
  rv = aPropertyArray->GetLength(&numProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < numProperties; i++) {
    nsCOMPtr<sbITranscodeProfileProperty> property =
        do_QueryElementAt(aPropertyArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propName;
    rv = property->GetPropertyName(propName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIVariant> prefVariant;

    if (aDevice) {
      nsString prefName = aPrefNameBase;
      prefName.AppendLiteral(".");
      prefName.Append(propName);

      rv = aDevice->GetPreference(prefName, getter_AddRefs(prefVariant));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      sbPrefBranch prefs(NS_ConvertUTF16toUTF8(aPrefNameBase).get(), &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = prefs.GetPreference(propName, getter_AddRefs(prefVariant));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Only use the property if we have a real value (not a void/empty variant)
    PRUint16 dataType;
    rv = prefVariant->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dataType != nsIDataType::VTYPE_VOID &&
        dataType != nsIDataType::VTYPE_EMPTY)
    {
      rv = property->SetValue(prefVariant);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}


/**
 * Copy properties, either audio or video.
 * @param aSrcProps the properties to copy from
 * @param aDstBag the property bag to output
 * @param aIsVideo true if this is for video, false for audio
 */
nsresult
sbGStreamerTranscodeAudioConfigurator::CopyPropertiesIntoBag(nsIArray * aSrcProps,
                                                              nsIWritablePropertyBag * aDstBag,
                                                              PRBool aIsVideo)
{
  NS_ENSURE_ARG_POINTER(aSrcProps);
  NS_ENSURE_ARG_POINTER(aDstBag);

  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> propsEnum;
  rv = aSrcProps->Enumerate(getter_AddRefs(propsEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasMore;
  while (NS_SUCCEEDED(rv = propsEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = propsEnum->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbITranscodeProfileProperty> prop =
      do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hidden;
    rv = prop->GetHidden(&hidden);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hidden) {
      continue;
    }
    nsString propName;
    rv = prop->GetPropertyName(propName);
    NS_ENSURE_SUCCESS(rv, rv);

    // get the value
    nsCOMPtr<nsIVariant> value;
    rv = prop->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aDstBag->SetProperty(propName, value);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscodeAudioConfigurator::GetAvailableProfiles(nsIArray * *aAvailableProfiles)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (mAvailableProfiles) {
    NS_IF_ADDREF (*aAvailableProfiles = mAvailableProfiles);
    return NS_OK;
  }

  /* If we haven't already cached it, then figure out what we have */

  if (!mElementNames.IsInitialized()) {
    PRBool initSuccess = mElementNames.Init();
    NS_ENSURE_TRUE(initSuccess, NS_ERROR_OUT_OF_MEMORY);
  }

  nsresult rv;
  PRBool hasMoreElements;
  nsCOMPtr<nsISimpleEnumerator> dirEnum;

  nsCOMPtr<nsIURI> profilesDirURI;
  rv = NS_NewURI(getter_AddRefs(profilesDirURI),
          NS_LITERAL_STRING("resource://app/gstreamer/encode-profiles"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> profilesDirFileURL =
      do_QueryInterface(profilesDirURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> profilesDir;
  rv = profilesDirFileURL->GetFile(getter_AddRefs(profilesDir));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> array =
      do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITranscodeProfileLoader> profileLoader =
      do_CreateInstance("@getnightingale.com/Nightingale/Transcode/ProfileLoader;1",
              &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = profilesDir->GetDirectoryEntries(getter_AddRefs(dirEnum));
  NS_ENSURE_SUCCESS (rv, rv);

  while (PR_TRUE) {
    rv = dirEnum->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements)
      break;

    nsCOMPtr<nsIFile> file;
    rv = dirEnum->GetNext(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbITranscodeProfile> profile;

    rv = profileLoader->LoadProfile(file, getter_AddRefs(profile));
    if (NS_FAILED(rv))
      continue;

    rv = EnsureProfileAvailable(profile);
    if (NS_FAILED(rv)) {
      // Not able to use this profile; don't return it.
      continue;
    }

    rv = array->AppendElement(profile, PR_FALSE);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  mAvailableProfiles = do_QueryInterface(array, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  NS_ADDREF(*aAvailableProfiles = mAvailableProfiles);

  return NS_OK;
}

/**** sbIDeviceTranscodingConfigurator implementation *****/
/* attribute sbIDevice device; */
NS_IMETHODIMP
sbGStreamerTranscodeAudioConfigurator::GetDevice(sbIDevice * *aDevice)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_IF_ADDREF(*aDevice = mDevice);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscodeAudioConfigurator::SetDevice(sbIDevice * aDevice)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_FALSE(mConfigurateState > CONFIGURATE_NOT_STARTED,
                  NS_ERROR_ALREADY_INITIALIZED);
  mDevice = aDevice;
  return NS_OK;
}

/**** sbITranscodingConfigurator implementation *****/

/* void determineOutputType (); */
NS_IMETHODIMP
sbGStreamerTranscodeAudioConfigurator::DetermineOutputType()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  // check our inputs
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mConfigurateState >= CONFIGURATE_OUTPUT_SET,
                  NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv;

  // Get the preferred encoding profile
  rv = SelectProfile();
  NS_ENSURE_SUCCESS(rv, rv);

  EncoderProfileData elementNames;
  PRBool success = mElementNames.Get(mSelectedProfile, &elementNames);
  NS_ENSURE_TRUE (success, NS_ERROR_UNEXPECTED);
  CopyASCIItoUTF16(elementNames.muxer, mMuxer);
  CopyASCIItoUTF16(elementNames.audioEncoder, mAudioEncoder);

  if (!mMuxer.IsEmpty())
    mUseMuxer = PR_TRUE;

  if (!mAudioEncoder.IsEmpty())
    mUseAudioEncoder = PR_TRUE;

  rv = mSelectedProfile->GetFileExtension(mFileExtension);
  NS_ENSURE_SUCCESS (rv, rv);

  mConfigurateState = CONFIGURATE_OUTPUT_SET;

  return NS_OK;
}

/* void configurate (); */

NS_IMETHODIMP
sbGStreamerTranscodeAudioConfigurator::Configurate()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  // check our inputs
  NS_ENSURE_TRUE(mInputFormat, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mConfigurateState >= CONFIGURATE_FINISHED,
                  NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv;

  if (mConfigurateState < CONFIGURATE_OUTPUT_SET) {
    // no output set yet, do that now
    rv = DetermineOutputType();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIMediaFormatAudio> audioFormat;
  rv = mInputFormat->GetAudioStream(getter_AddRefs(audioFormat));
  NS_ENSURE_SUCCESS(rv, rv);

  /* If we have an audio encoder selected, AND the input has audio, we can
     configure the audio details */
  if (audioFormat && !mAudioEncoder.IsEmpty()) {
    rv = SelectOutputAudioFormat();
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the audio properties we want to use.
    rv = SetAudioProperties();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    /* Otherwise, error out */
    LOG(("No audio, cannot configurate for audio-only"));
    return NS_ERROR_NOT_AVAILABLE;
  }

  /* Ensure video is disabled - this is an audio only configurator. */
  mVideoEncoder.SetIsVoid(PR_TRUE);

  // all done
  mConfigurateState = CONFIGURATE_FINISHED;
  return NS_OK;
}
