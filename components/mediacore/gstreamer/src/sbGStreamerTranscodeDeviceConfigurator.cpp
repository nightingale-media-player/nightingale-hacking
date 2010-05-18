/* vim: set sw=2 : */
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

#if defined(XP_WIN)
// needed before math.h to import HUGE_VAL correctly
#define _DLL
#endif /* XP_WIN */

///// Class header include
#include "sbGStreamerTranscodeDeviceConfigurator.h"

///// Gecko interface includes
#include <nsIArray.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIMutableArray.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag.h>
#include <nsIWritablePropertyBag2.h>

///// Songbird interface includes
#include <sbIDevice.h>
#include <sbIDeviceCapabilities.h>
#include <sbIMediaFormatMutable.h>
#include <sbITranscodeError.h>
#include <sbITranscodeManager.h>
#include <sbITranscodeProfile.h>

///// Gecko header includes
#include <nsArrayUtils.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <prlog.h>

///// Songbird header includes
#include <sbArrayUtils.h>
#include <sbMemoryUtils.h>
#include <sbStringUtils.h>
#include <sbTranscodeUtils.h>
#include <sbVariantUtils.h>

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
#include <math.h>
#include <algorithm>
#include <functional>
#include <vector>

///// Local directory includes
#include "sbGStreamerMediacoreUtils.h"

///// NSPR Logging
/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbGStreamerTranscodeDeviceConfigurator:5
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
NS_IMPL_ISUPPORTS_INHERITED2(sbGStreamerTranscodeDeviceConfigurator,
                             sbTranscodingConfigurator,
                             sbIDeviceTranscodingConfigurator,
                             sbPIGstTranscodingConfigurator);

sbGStreamerTranscodeDeviceConfigurator::sbGStreamerTranscodeDeviceConfigurator()
  : mQuality(-HUGE_VAL),
    mVideoBitrate(PR_INT32_MIN)
{
  #if PR_LOGGING
    if (!gGstTranscodeConfiguratorLog) {
      gGstTranscodeConfiguratorLog =
        PR_NewLogModule("sbGStreamerTranscodeDeviceConfigurator");
    }
  #endif /* PR_LOGGING */
  TRACE(("%s[%p]", __FUNCTION__, this));
  /* nothing */
}

sbGStreamerTranscodeDeviceConfigurator::~sbGStreamerTranscodeDeviceConfigurator()
{
  /* nothing */
}

/**
 * make a GstCaps structure from a caps name and an array of attributes
 *
 * @param aMimeType [in] the name (e.g. "audio/x-pcm-int")
 * @param aAttributes [in] the attributes
 * @param aResultCaps [out] the generated GstCaps, with an outstanding refcount
 */
nsresult
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

  // set up a caps structure
  sbGstCaps caps = GetCapsForMimeType (aMimeType, aType);
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

nsresult
sbGStreamerTranscodeDeviceConfigurator::EnsureProfileAvailable(sbITranscodeEncoderProfile *aProfile)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aProfile);

  nsresult rv;

  // for now, only support video profiles
  PRUint32 type;
  rv = aProfile->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);
  switch(type) {
    case sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO:
      break;
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }

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
      TRACE(("%s: no muxer available for %s",
             __FUNCTION__,
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
      TRACE(("%s: no audio encoder available for %s",
             __FUNCTION__,
             NS_LossyConvertUTF16toASCII(capsName).get()));
      return NS_ERROR_UNEXPECTED;
    }
    elementNames.audioEncoder = audioEncoder;
  }

  /// Check that we have an video encoder available
  rv = aProfile->GetVideoCodec(capsName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!capsName.IsEmpty()) {
    nsCOMPtr<nsIArray> attrs;
    rv = aProfile->GetVideoAttributes(getter_AddRefs(attrs));
    NS_ENSURE_SUCCESS(rv, rv);

    GstCaps* caps = NULL;
    rv = MakeCapsFromAttributes(SB_GST_CAPS_MAP_VIDEO,
                                NS_LossyConvertUTF16toASCII(capsName),
                                attrs,
                                &caps);
    NS_ENSURE_SUCCESS(rv, rv);

    const char* videoEncoder = FindMatchingElementName(caps, "Encoder");
    gst_caps_unref(caps);
    if (!videoEncoder) {
      TRACE(("%s: no video encoder available for %s",
             __FUNCTION__,
             NS_LossyConvertUTF16toASCII(capsName).get()));
      return NS_ERROR_UNEXPECTED;
    }
    elementNames.videoEncoder = videoEncoder;
  }

  PRBool success = mElementNames.Put(aProfile, elementNames);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbGStreamerTranscodeDeviceConfigurator::SelectQuality()
{
  nsresult rv;
  if (mQuality != -HUGE_VAL) {
    // already set
    return NS_OK;
  }
  if (!mDevice) {
    // we don't have a device to read things from :(
    // default to 1
    rv = SetQuality(1.0);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  double quality = 1.0;
  nsCOMPtr<nsIVariant> qualityVar;
  rv = mDevice->GetPreference(NS_LITERAL_STRING("transcode.quality.video"),
                              getter_AddRefs(qualityVar));
  if (NS_FAILED(rv)) {
    rv = SetQuality(quality);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
  PRUint16 variantType;
  rv = qualityVar->GetDataType(&variantType);
  NS_ENSURE_SUCCESS(rv, rv);
  switch (variantType) {
    case nsIDataType::VTYPE_EMPTY:
    case nsIDataType::VTYPE_VOID:
      break;
    default:
      rv = qualityVar->GetAsDouble(&quality);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
  }
  rv = SetQuality(quality);
  NS_ENSURE_SUCCESS(rv, rv);
  TRACE(("%s: set quality to %f", __FUNCTION__, quality));
  return NS_OK;
}

/**
 * Select the encoding profile to use
 *
 * @precondition the device has been set
 * @precondition the quality has been set
 * @postcondition mSelectedProfile is the encoder profile to use
 * @postcondition mSelectedFormat is ths device format matching the profile
 */
nsresult
sbGStreamerTranscodeDeviceConfigurator::SelectProfile()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  /**
   * Because there's no useful way to enumerate the device capabilities, we
   * must instead step through the encoder profiles and inspect each to see if
   * it's acceptable on a particular device.
   */
  NS_PRECONDITION(mDevice, "SelectProfile called with no device set!");
  NS_PRECONDITION(mQuality != -HUGE_VAL, "quality is not set!");

  nsresult rv;

  // the best compatible encoder profile
  nsCOMPtr<sbITranscodeEncoderProfile> selectedProfile;
  // the priority we got for that profile
  PRUint32 selectedPriority = 0;
  // the device format for the selected profile
  nsCOMPtr<sbIVideoFormatType> selectedFormat;

  // get available encoder profiles
  nsCOMPtr<nsIArray> profilesArray;
  rv = GetAvailableProfiles(getter_AddRefs(profilesArray));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> profilesEnum;
  rv = profilesArray->Enumerate(getter_AddRefs(profilesEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // get the device caps
  nsCOMPtr<sbIDeviceCapabilities> caps;
  rv = mDevice->GetCapabilities(getter_AddRefs(caps));
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXMook: video only for now
  PRUint32 mimeTypesCount;
  char **mimeTypes;
  rv = caps->GetSupportedMimeTypes(sbIDeviceCapabilities::CONTENT_VIDEO,
                                   &mimeTypesCount,
                                   &mimeTypes);
  if (NS_FAILED(rv)) {
    // report an error
    nsresult rv2;
    nsString deviceName;
    rv2 = mDevice->GetName(deviceName);
    if (NS_FAILED(rv2)) {
      deviceName =
        SBLocalizedString("transcode.error.device_no_video.unknown_device");
    }
    nsTArray<nsString> detailParams;
    detailParams.AppendElement(deviceName);
    nsString detail =
      SBLocalizedString("transcode.error.device_no_video.details", detailParams);
    rv2 = SB_NewTranscodeError(NS_LITERAL_STRING("transcode.error.device_no_video.hasitem"),
                               NS_LITERAL_STRING("transcode.error.device_no_video.withoutitem"),
                               detail,
                               mInputUri,
                               nsnull,
                               getter_AddRefs(mLastError));
    NS_ENSURE_SUCCESS(rv2, rv2);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  sbAutoFreeXPCOMArrayByRef<char**> autoMimeTypes(mimeTypesCount, mimeTypes);

  PRBool hasMoreProfiles;
  while (NS_SUCCEEDED(profilesEnum->HasMoreElements(&hasMoreProfiles)) &&
         hasMoreProfiles)
  {
    nsCOMPtr<nsISupports> profileSupports;
    rv = profilesEnum->GetNext(getter_AddRefs(profileSupports));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbITranscodeEncoderProfile> profile =
      do_QueryInterface(profileSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // check the priority first, that's easier
    PRUint32 priority;
    rv = profile->GetEncoderProfilePriority(mQuality, &priority);
    NS_ENSURE_SUCCESS(rv, rv);
    if (priority <= selectedPriority) {
      continue;
    }

    for (PRUint32 mimeTypeIndex = 0;
         mimeTypeIndex < mimeTypesCount;
         ++mimeTypeIndex)
    {
      /* We get the preferred format types here - these are the ones that it's
         ok to transcode to (rather than the full set of things supported by
         the device) */
      nsISupports** formatTypes;
      PRUint32 formatTypeCount;
      rv = caps->GetPreferredFormatTypes(
              sbIDeviceCapabilities::CONTENT_VIDEO,
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
        nsCOMPtr<sbIVideoFormatType> format = do_QueryInterface(
            formatTypes[formatIndex], &rv);
        if (NS_FAILED(rv) || !format) {
          // XXX Mook: we only support video for now
          continue;
        }

        // check the container
        nsCString formatContainer;
        rv = format->GetContainerType(formatContainer);
        NS_ENSURE_SUCCESS(rv, rv);
        nsString encoderContainer;
        rv = profile->GetContainerFormat(encoderContainer);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!encoderContainer.Equals(NS_ConvertUTF8toUTF16(formatContainer))) {
          // mismatch, try the next device format
          continue;
        }

        // check the audio codec
        nsString encoderAudioCodec;
        rv = profile->GetAudioCodec(encoderAudioCodec);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<sbIDevCapAudioStream> audioCaps;
        rv = format->GetAudioStream(getter_AddRefs(audioCaps));
        NS_ENSURE_SUCCESS(rv, rv);
        if (!audioCaps) {
          if (!encoderAudioCodec.IsEmpty()) {
            // this device format doesn't support audio, but the encoder does
            // skip for now, I think?
            continue;
          }
        }
        else {
          nsCString formatAudioCodec;
          rv = audioCaps->GetType(formatAudioCodec);
          NS_ENSURE_SUCCESS(rv, rv);
          if (!encoderAudioCodec.Equals(NS_ConvertUTF8toUTF16(formatAudioCodec))) {
            // mismatch, try the next device format
            continue;
          }
          // XXX Mook: TODO: match properties
        }
  
        // check the video codec
        nsString encoderVideoCodec;
        rv = profile->GetVideoCodec(encoderVideoCodec);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<sbIDevCapVideoStream> videoCaps;
        rv = format->GetVideoStream(getter_AddRefs(videoCaps));
        NS_ENSURE_SUCCESS(rv, rv);
        if (!videoCaps) {
          if (!encoderVideoCodec.IsEmpty()) {
            // this device format doesn't support video, but the encoder does
            // skip for now, I think?
            continue;
          }
        }
        else {
          nsCString formatVideoCodec;
          rv = videoCaps->GetType(formatVideoCodec);
          NS_ENSURE_SUCCESS(rv, rv);
          if (!encoderVideoCodec.Equals(NS_ConvertUTF8toUTF16(formatVideoCodec))) {
            // mismatch, try the next device format
            continue;
          }
          // XXX Mook: TODO: match properties
        }
  
        // assume we match here
        selectedProfile = profile;
        selectedFormat = format;
        rv = profile->GetEncoderProfilePriority(mQuality, &selectedPriority);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    // if we reach here, we have either added it to the available list or
    // the encoder profile is not compatible; either way, nothing to do here
  }

  mSelectedProfile = selectedProfile;
  if (!mSelectedProfile) {
    // no suitable encoder profile found
    TRACE(("%s: no suitable encoder profile found", __FUNCTION__));
    // report an error
    nsString deviceName;
    rv = mDevice->GetName(deviceName);
    if (NS_FAILED(rv)) {
      deviceName =
        SBLocalizedString("transcode.error.device_no_video.unknown_device");
    }
    nsTArray<nsString> detailParams;
    detailParams.AppendElement(deviceName);
    nsString detail =
      SBLocalizedString("transcode.error.no_profile.details", detailParams);
    rv = SB_NewTranscodeError(NS_LITERAL_STRING("transcode.error.no_profile.hasitem"),
                              NS_LITERAL_STRING("transcode.error.no_profile.withoutitem"),
                              detail,
                              mInputUri,
                              nsnull,
                              getter_AddRefs(mLastError));
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_ERROR_FAILURE;
  }
  mSelectedFormat = selectedFormat;

  EncoderProfileData elementNames;
  PRBool success = mElementNames.Get(selectedProfile, &elementNames);
  NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);
  CopyASCIItoUTF16(elementNames.muxer, mMuxer);
  CopyASCIItoUTF16(elementNames.audioEncoder, mAudioEncoder);
  CopyASCIItoUTF16(elementNames.videoEncoder, mVideoEncoder);
  rv = selectedProfile->GetFileExtension(mFileExtension);
  NS_ENSURE_SUCCESS(rv, rv);
  
  TRACE(("%s: profile selected, using muxer [%s] audio [%s] video [%s] extension [%s]",
         __FUNCTION__,
         elementNames.muxer.get(),
         elementNames.audioEncoder.get(),
         elementNames.videoEncoder.get(),
         mFileExtension.get()));

  /* Set whether we're using these - in this configurator, this is based
     entirely on whether we've selected a specific element */
  if (!mMuxer.IsEmpty())
    mUseMuxer = PR_TRUE;
  if (!mAudioEncoder.IsEmpty())
    mUseAudioEncoder = PR_TRUE;
  if (!mVideoEncoder.IsEmpty())
    mUseVideoEncoder = PR_TRUE;

  return NS_OK;
}

/**
 * Set audio-related properties
 *
 * @precondition a profile has been selected
 * @postcondition mAudioEncoderProperties contains the audio properties desired
 */
nsresult
sbGStreamerTranscodeDeviceConfigurator::SetAudioProperties()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_PRECONDITION(mSelectedProfile,
                  "attempted to set audio properties without selecting profile");
  NS_PRECONDITION(mSelectedFormat,
                  "attempted to set audio properties without selected output format");

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
  if (inputFormat) {
    // Get the device output audio info
    nsCOMPtr<sbIDevCapAudioStream> outputCaps;
    rv = mSelectedFormat->GetAudioStream(getter_AddRefs(outputCaps));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isInRange;

    nsCOMPtr<sbIDevCapRange> sampleRateRange;
    rv = outputCaps->GetSupportedSampleRates(getter_AddRefs(sampleRateRange));
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
    rv = outputCaps->GetSupportedChannels(getter_AddRefs(channelsRange));
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
  }
  else {
    // no audio stream
    mAudioEncoder.SetIsVoid(PR_TRUE);
  }
  if (!mAudioEncoderProperties) {
    mAudioEncoderProperties =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIWritablePropertyBag> writableBag =
    do_QueryInterface(mAudioEncoderProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> propsSrc;
  rv = mSelectedProfile->GetAudioProperties(getter_AddRefs(propsSrc));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CopyPropertiesIntoBag(propsSrc, writableBag, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * Determine the desired output size (ignoring bitrate constraints)
 *
 * @precondition the profile has been selected via SelectProfile()
 * @postcondition mPreferredDimensions is the desired output size
 */
nsresult
sbGStreamerTranscodeDeviceConfigurator::DetermineIdealOutputSize()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_PRECONDITION(mSelectedProfile,
                  "DetermineIdealOutputSize called without selected profile");
  NS_PRECONDITION(mSelectedFormat,
                  "DetermineIdealOutputSize called without selected format");
  NS_PRECONDITION(mInputFormat,
                  "DetermineIdealOutputSize called without input format");

  nsresult rv;

  /*
   * find the smallest output format that has at least many pixels (horizontally
   * as well as vertically) as the input format
   */

  nsCOMPtr<sbIMediaFormatVideo> inputFormat;
  rv = mInputFormat->GetVideoStream(getter_AddRefs(inputFormat));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!inputFormat) {
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  Dimensions input;
  rv = inputFormat->GetVideoWidth(&input.width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inputFormat->GetVideoHeight(&input.height);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapVideoStream> outputCaps;
  rv = mSelectedFormat->GetVideoStream(getter_AddRefs(outputCaps));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(outputCaps, NS_ERROR_FAILURE);

  // If we can't match the PAR, adjust to square pixels
  { /* scope */
    PRUint32 num, denom;
    rv = inputFormat->GetVideoPAR(&num, &denom);
    NS_ENSURE_SUCCESS(rv, rv);
    sbFraction inputPAR(num, denom);

    // Check to see if the PAR values for the device caps is a min/max or a
    // set range of values.
    PRBool supportsPARRange;
    rv = outputCaps->GetDoesSupportPARRange(&supportsPARRange);
    NS_ENSURE_SUCCESS(rv, rv);

    if (supportsPARRange) {
      // The PAR value is a min/max value range. Pick the closet match.
      nsCOMPtr<sbIDevCapFraction> minPARFraction;
      rv = outputCaps->GetMinimumSupportedPAR(getter_AddRefs(minPARFraction));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = minPARFraction->GetNumerator(&num);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = minPARFraction->GetDenominator(&denom);
      NS_ENSURE_SUCCESS(rv, rv);
      sbFraction minPAR(num, denom);

      nsCOMPtr<sbIDevCapFraction> maxPARFraction;
      rv = outputCaps->GetMaximumSupportedPAR(getter_AddRefs(maxPARFraction));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = maxPARFraction->GetNumerator(&num);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = maxPARFraction->GetDenominator(&denom);
      NS_ENSURE_SUCCESS(rv, rv);
      sbFraction maxPAR(num, denom);

      // If the input PAR is between the min and max PAR values use the input
      // PAR value. If not, use the closest PAR value.
      if (inputPAR >= minPAR && inputPAR <= maxPAR) {
        mOutputPAR = inputPAR;
      }
      else if (inputPAR < minPAR) {
        mOutputPAR = minPAR;
      }
      else if (inputPAR > maxPAR) {
        mOutputPAR = maxPAR;
      }
    }
    else {
      nsCOMPtr<nsIArray> parRanges;
      rv = outputCaps->GetSupportedPARs(getter_AddRefs(parRanges));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 count, index = 0;
      rv = parRanges->GetLength(&count);
      NS_ENSURE_SUCCESS(rv, rv);

      for (PRUint32 i = 0; i < count; i++) {
        nsCOMPtr<sbIDevCapFraction> curFraction =
          do_QueryElementAt(parRanges, i, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = curFraction->GetNumerator(&num);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = curFraction->GetDenominator(&denom);
        NS_ENSURE_SUCCESS(rv, rv);

        sbFraction curPARFraction(num, denom);
        if (inputPAR.IsEqual(curPARFraction)) {
          mOutputPAR = inputPAR;
          break;
        }
      }

      if (index >= count) {
        // we didn't find a match; just... kinda give up and blow them up into
        // square-looking pixels by duplicating pixels
        input.width *= inputPAR.Denominator();
        input.height *= inputPAR.Numerator();
        mOutputPAR = sbFraction(1, 1); // XXX Mook: we need to adjust to something
        // we have output PAR for!
      }
    }
  }

  { /* scope - try to use ranges */
    PRBool hasRange = PR_TRUE;
    std::vector<PRInt32> widths, heights;
    Dimensions output(0, 0);
    nsCOMPtr<sbIDevCapRange> range;
    rv = outputCaps->GetSupportedWidths(getter_AddRefs(range));
    NS_ENSURE_SUCCESS(rv, rv);
    if (range) {
      PRUint32 count;
      rv = range->GetValueCount(&count);
      NS_ENSURE_SUCCESS(rv, rv);
      if (count > 0) {
        for (PRUint32 i = 0; i < count; ++i) {
          PRInt32 v;
          rv = range->GetValue(i, &v);
          NS_ENSURE_SUCCESS(rv, rv);
          widths.push_back(v);
        }
        std::sort(widths.begin(), widths.end());
      }
      else {
        // no count, use min + step + max
        PRInt32 min, step, max;
        rv  = range->GetMin(&min);
        rv |= range->GetStep(&step);
        rv |= range->GetMax(&max);
        NS_ENSURE_SUCCESS(rv, rv);
        for (PRInt32 v = min; v <= max; v += step) {
          widths.push_back(v);
        }
      }
    }
    else {
      hasRange = PR_FALSE;
    }
    if (hasRange) {
      rv = outputCaps->GetSupportedHeights(getter_AddRefs(range));
      NS_ENSURE_SUCCESS(rv, rv);
      if (range) {
        PRUint32 count;
        rv = range->GetValueCount(&count);
        NS_ENSURE_SUCCESS(rv, rv);
        if (count > 0) {
          for (PRUint32 i = 0; i < count; ++i) {
            PRInt32 v;
            rv = range->GetValue(i, &v);
            NS_ENSURE_SUCCESS(rv, rv);
            heights.push_back(v);
          }
          std::sort(heights.begin(), heights.end());
        }
        else {
          // no count, use min + step + max
          PRInt32 min, step, max;
          rv  = range->GetMin(&min);
          rv |= range->GetStep(&step);
          rv |= range->GetMax(&max);
          NS_ENSURE_SUCCESS(rv, rv);
          for (PRInt32 v = min; v <= max; v += step) {
            heights.push_back(v);
          }
        }
      }
      else {
        hasRange = PR_FALSE;
      }
    }
    if (hasRange) {
      // we got a set of ranges, let's try to get some sort of output size
      output = input;
      // try to scale things up to the minimum size, preseving aspect ratio
      if (output.width < widths.front()) {
        output.width = widths.front();
        output.height = PRUint64(input.height) * widths.front() / input.width;
      }
      if (output.height < heights.front()) {
        output.height = heights.front();
        output.width = PRUint64(input.width) * heights.front() / input.height;
      }
      // cap to maximum size (this may affect aspect ratio)
      output = GetMaximumFit(output, Dimensions(widths.back(), heights.back()));
      // get it to a multiple of step size at least as big as the desired size
      // (this will also force it to at least minimum size if it has been
      // scaled down; nothing we can do about that, we'll just need black bars)
      std::vector<PRInt32>::const_iterator it;
      it = std::lower_bound(widths.begin(), widths.end(), output.width);
      if (it == widths.end()) {
        --it;
      }
      output.width = *it;
      it = std::lower_bound(heights.begin(), heights.end(), output.height);
      if (it == heights.end()) {
        --it;
      }
      output.height = *it;

      // we have something set via the ranges.  no need to worry about the aspect
      // ratio, because the transcoder will deal with adding padding for us
      mPreferredDimensions = output;
      return NS_OK;
    }
  } /* end scope */

  // no ranges, we need to use explicit sizes
  // find the smallest explicit size that will fit
  Dimensions output(PR_INT32_MAX, PR_INT32_MAX);
  nsCOMPtr<nsIArray> explicitSizes;
  rv = outputCaps->GetSupportedExplicitSizes(getter_AddRefs(explicitSizes));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> sizeEnum;
  rv = explicitSizes->Enumerate(getter_AddRefs(sizeEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasMore;
  while (NS_SUCCEEDED(rv = sizeEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = sizeEnum->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbIImageSize> size = do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 width, height;
    rv = size->GetWidth(&width);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = size->GetHeight(&height);
    NS_ENSURE_SUCCESS(rv, rv);
    if (width < input.width || height < input.height) {
      // too small
      continue;
    }
    if (width > output.width && height > output.height) {
      // larger than what we already selected; not useful
      continue;
    }
    output.width = width;
    output.height = height;
  }
  NS_ENSURE_SUCCESS(rv, rv);
  if (output.width != PR_INT32_MAX || output.height != PR_INT32_MAX) {
    // found a size
    mPreferredDimensions = output;
    return NS_OK;
  }

  // the device has explicit supported sizes, but the source image is too big
  // to fit. Try to find the best fit.
  // As a simple algorithm, find the size the output will be when scaled down,
  // and use the one with the maximum width (since aspect ratio will be
  // preserved, and we assume square pixels, that is equivalent to the largest
  // number of pixels)
  rv = explicitSizes->Enumerate(getter_AddRefs(sizeEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  Dimensions result, best(0, 0), selected(0, 0), maxSize;
  while (NS_SUCCEEDED(rv = sizeEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = sizeEnum->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbIImageSize> size = do_QueryInterface(supports);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = size->GetWidth(&maxSize.width);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = size->GetHeight(&maxSize.height);
    NS_ENSURE_SUCCESS(rv, rv);
    result = GetMaximumFit(input, maxSize);
    if (result.width > best.width) {
      best = result;
      selected = maxSize;
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);
  if (best.width < 1) {
    // best width is still empty?
    return NS_ERROR_FAILURE;
  }

  mPreferredDimensions = selected;
  return NS_OK;
}

sbGStreamerTranscodeDeviceConfigurator::Dimensions
sbGStreamerTranscodeDeviceConfigurator::GetMaximumFit(
  const sbGStreamerTranscodeDeviceConfigurator::Dimensions& aInput,
  const sbGStreamerTranscodeDeviceConfigurator::Dimensions& aMaximum)
{
  TRACE(("%s", __FUNCTION__));
  if (aInput.width <= aMaximum.width && aInput.height < aMaximum.height) {
    // things fit anyway! there was no need to call this.
    return aInput;
  }
  // At least one side exceeds the maximum rectangle; figure out which it is
  Dimensions result = aMaximum;
  if (PRUint64(aInput.width) * aMaximum.height > PRUint64(aInput.height) * aMaximum.width) {
    // the horzontal bounds will be hit first
    result.height = PRUint64(aInput.height) * aMaximum.width / aInput.width;
  }
  else {
    result.width = PRUint64(aInput.width) * aMaximum.height / aInput.height;
  }
  return result;
}

/**
 * Scale the video to something useful for the selected video bit rate
 * @precondition mPreferredDimensions has been set
 * @precondition mSelectedProfile has been selected
 * @precondition mSelectedFormat has been selected
 * @postcondition mVideoBitrate will be set (if it hasn't already been, or
 *                if the existing setting is too high for the device)
 * @postcondition mOutputDimensions will be the desired output dimensions
 */
nsresult
sbGStreamerTranscodeDeviceConfigurator::FinalizeOutputSize()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_PRECONDITION(mPreferredDimensions.width > 0 &&
                    mPreferredDimensions.height > 0,
                  "FinalizeOutputSize needs preferred dimensions");
  NS_PRECONDITION(mSelectedProfile,
                  "FinalizeOutputSize called with no profile selected");
  NS_PRECONDITION(mSelectedFormat,
                  "FinalizeOutputSize called with no output format");
  nsresult rv;

  // set the video quality setting
  mVideoQuality = mQuality;

  sbFraction outputFrameRate(PR_UINT32_MAX, 1);
  // get the desired frame rate
  { /* scope */
    nsCOMPtr<sbIMediaFormatVideo> inputFormat;
    rv = mInputFormat->GetVideoStream(getter_AddRefs(inputFormat));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(inputFormat, NS_ERROR_FAILURE);
    PRUint32 num, denom;
    rv = inputFormat->GetVideoFrameRate(&num, &denom);
    NS_ENSURE_SUCCESS(rv, rv);
    sbFraction inputFrameRate(num, denom);
    nsCOMPtr<sbIDevCapVideoStream> videoCaps;
    rv = mSelectedFormat->GetVideoStream(getter_AddRefs(videoCaps));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(videoCaps, NS_ERROR_FAILURE);

    PRBool isFrameRatesRange;
    rv = videoCaps->GetDoesSupportFrameRateRange(&isFrameRatesRange);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isFrameRatesRange) {
      nsCOMPtr<sbIDevCapFraction> minFrameRate;
      rv = videoCaps->GetMinimumSupportedFrameRate(
          getter_AddRefs(minFrameRate));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = minFrameRate->GetNumerator(&num);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = minFrameRate->GetDenominator(&denom);
      NS_ENSURE_SUCCESS(rv, rv);
      sbFraction minFrameRateFraction(num, denom);

      nsCOMPtr<sbIDevCapFraction> maxFrameRate;
      rv = videoCaps->GetMaximumSupportedFrameRate(
          getter_AddRefs(maxFrameRate));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = maxFrameRate->GetNumerator(&num);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = maxFrameRate->GetDenominator(&denom);
      NS_ENSURE_SUCCESS(rv, rv);
      sbFraction maxFrameRateFraction(num, denom);

      if (inputFrameRate >= minFrameRateFraction &&
          inputFrameRate <= maxFrameRateFraction)
      {
        outputFrameRate = inputFrameRate;
      }
      else if (inputFrameRate < minFrameRateFraction) {
        outputFrameRate = minFrameRateFraction;
      }
      else if (inputFrameRate > maxFrameRateFraction) {
        outputFrameRate = maxFrameRateFraction;
      }
    }
    else {
      nsCOMPtr<nsIArray> frameRatesRange;
      rv = videoCaps->GetSupportedFrameRates(getter_AddRefs(frameRatesRange));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length = 0;
      rv = frameRatesRange->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      for (PRUint32 i = 0; i < length; i++) {
        nsCOMPtr<sbIDevCapFraction> curFraction =
          do_QueryElementAt(frameRatesRange, i, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 num, denom;
        rv = curFraction->GetNumerator(&num);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = curFraction->GetDenominator(&denom);
        NS_ENSURE_SUCCESS(rv, rv);

        sbFraction candidate(num, denom);

        double lastDifference = fabs(outputFrameRate - inputFrameRate);
        double difference = fabs(candidate - inputFrameRate);
        if (difference < lastDifference) {
          outputFrameRate = candidate;
        }
      }
    }

    mVideoFrameRate = outputFrameRate;
  }

  // get the desired bpp
  double videoBPP;
  PRBool bitrateForced = PR_FALSE;
restartQualityConfiguration:
  rv = mSelectedProfile->GetVideoBitsPerPixel(mVideoQuality, &videoBPP);
  NS_ENSURE_SUCCESS(rv, rv);

  // the bit rate desired
  if (mVideoBitrate == PR_INT32_MIN) {
    mVideoBitrate = videoBPP *
                    mPreferredDimensions.width *
                    mPreferredDimensions.height *
                    mVideoFrameRate;
  }
  else {
    // we already have some sort of bitrate set
    bitrateForced = PR_TRUE;
    videoBPP = mVideoBitrate /
               mPreferredDimensions.width /
               mPreferredDimensions.height /
               mVideoFrameRate;
  }
  // cap the video bit rate to what the device supports
  nsCOMPtr<sbIDevCapVideoStream> videoCaps;
  rv = mSelectedFormat->GetVideoStream(getter_AddRefs(videoCaps));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(videoCaps, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDevCapRange> videoBitrateRange;
  rv = videoCaps->GetSupportedBitRates(getter_AddRefs(videoBitrateRange));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GetDevCapRangeUpper(videoBitrateRange, mVideoBitrate, &mVideoBitrate);
  NS_ENSURE_SUCCESS(rv, rv);
  if (rv != NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA) {
    // there were no problems getting the desired size
    mOutputDimensions = mPreferredDimensions;
    return NS_OK;
  }

  // if we reach this point, we need to scale the video down.
  // mVideoBitrate is the largest bitrate the device supports

  // first, get the input sizes
  nsCOMPtr<sbIMediaFormatVideo> inputFormat;
  rv = mInputFormat->GetVideoStream(getter_AddRefs(inputFormat));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(inputFormat, NS_ERROR_FAILURE);
  Dimensions input;
  rv = inputFormat->GetVideoWidth(&input.width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inputFormat->GetVideoHeight(&input.height);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapRange> widths, heights;
  rv = videoCaps->GetSupportedWidths(getter_AddRefs(widths));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoCaps->GetSupportedHeights(getter_AddRefs(heights));
  NS_ENSURE_SUCCESS(rv, rv);
  if ((!widths) || (!heights)) {
    // this device does not support arbitrary sizes; use the given ones instead
    PRUint64 usefulPixels = 0;
    mOutputDimensions = Dimensions(0, 0);
    nsCOMPtr<nsIArray> sizes;
    rv = videoCaps->GetSupportedExplicitSizes(getter_AddRefs(sizes));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsISimpleEnumerator> sizesEnum;
    rv = sizes->Enumerate(getter_AddRefs(sizesEnum));
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool hasMore, foundOutput = PR_FALSE;
    while (NS_SUCCEEDED(sizesEnum->HasMoreElements(&hasMore)) && hasMore) {
      nsCOMPtr<nsISupports> supports;
      rv = sizesEnum->GetNext(getter_AddRefs(supports));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<sbIImageSize> imageSize = do_QueryInterface(supports, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      Dimensions size;
      rv = imageSize->GetWidth(&size.width);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = imageSize->GetHeight(&size.height);
      NS_ENSURE_SUCCESS(rv, rv);
      if (size.width * size.height * mVideoFrameRate * videoBPP > mVideoBitrate) {
        // this is too big
        continue;
      }
      Dimensions usefulSize = GetMaximumFit(input, size);
      if (usefulSize.width * usefulSize.height < usefulPixels) {
        // we had a better fit before
        continue;
      }
      foundOutput = PR_TRUE;
      mOutputDimensions = size;
      usefulPixels = usefulSize.width * usefulSize.height;
    }
    if (!foundOutput && !bitrateForced && mVideoQuality > 0) {
      // no available format found, try again with lower quality
      mVideoQuality = PR_MIN(0, mVideoQuality - 0.1);
      mVideoBitrate = PR_INT32_MIN;
      goto restartQualityConfiguration;
    }
    if (!foundOutput) {
      return NS_ERROR_FAILURE;
    }
    rv = GetDevCapRangeUpper(videoBitrateRange,
                             usefulPixels * mVideoFrameRate * videoBPP,
                             &mVideoBitrate);
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }

  // this device supports arbitrary sizes
  double pixels = double(mVideoBitrate) / videoBPP / mVideoFrameRate; // number of pixels
  double width = sqrt(pixels / mPreferredDimensions.height * mPreferredDimensions.width);
  double height = pixels / width;
  rv = GetDevCapRangeUpper(widths, width, &mOutputDimensions.width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = GetDevCapRangeUpper(heights, height, &mOutputDimensions.height);
  NS_ENSURE_SUCCESS(rv, rv);
  // so, output dimensions are the sizes we want; and the video bitrate is the
  // maximum we can support.

  // check if we can lower the quality if the image is too small
  if (!bitrateForced && mVideoQuality > 0.2) {
    double outputPixels = mOutputDimensions.width * mOutputDimensions.height;
    double maximumPixels = mPreferredDimensions.width *
                           mPreferredDimensions.height;
    if (outputPixels / maximumPixels < mVideoQuality) {
      // the ratio of pixels is way too small!
      mVideoQuality -= 0.1;
      mVideoBitrate = PR_INT32_MIN;
      goto restartQualityConfiguration;
    }
  }

  return NS_OK;
}

nsresult
sbGStreamerTranscodeDeviceConfigurator::SetVideoProperties()
{
  NS_PRECONDITION(mOutputDimensions.width > 0 && mOutputDimensions.height > 0,
                  "attempting to set video properties with no output dimensions!");
  NS_PRECONDITION(mVideoBitrate,
                  "attempting to set video properties with no video bitrate!");

  nsresult rv;
  nsCOMPtr<sbIMediaFormatVideoMutable> videoFormat =
    do_CreateInstance(SB_MEDIAFORMATVIDEO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // set the video format data
  rv = videoFormat->SetVideoWidth(mOutputDimensions.width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoHeight(mOutputDimensions.height);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoPAR(mOutputPAR.Numerator(),
                                mOutputPAR.Denominator());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoFrameRate(mVideoFrameRate.Numerator(),
                                      mVideoFrameRate.Denominator());
  NS_ENSURE_SUCCESS(rv, rv);
  mVideoFormat = do_QueryInterface(videoFormat, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mVideoEncoderProperties) {
    mVideoEncoderProperties =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // set arbitrary properties
  nsCOMPtr<nsIWritablePropertyBag> writableBag =
    do_QueryInterface(mVideoEncoderProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> propsSrc;
  rv = mSelectedProfile->GetVideoProperties(getter_AddRefs(propsSrc));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CopyPropertiesIntoBag(propsSrc, writableBag, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_POSTCONDITION(mVideoFormat,
                   "setVideoProperties failed to set video format");
  NS_POSTCONDITION(mVideoEncoderProperties,
                   "setVideoProperties failed to set video properties");
  return NS_OK;
}

/**
 * Copy properties, either audio or video.
 * @param aSrcProps the properties to copy from
 * @param aDstBag the property bag to output
 * @param aIsVideo true if this is for video, false for audio
 */
nsresult
sbGStreamerTranscodeDeviceConfigurator::CopyPropertiesIntoBag(nsIArray * aSrcProps,
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
    PRUint16 dataType;
    rv = value->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCString mapping;
    rv = prop->GetMapping(mapping);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!mapping.IsEmpty()) {
      if (aIsVideo && mapping.Equals("bitrate", CaseInsensitiveCompare)) {
        value = sbNewVariant(mVideoBitrate);
        NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);
      }
      else if (!aIsVideo && mapping.Equals("bitrate", CaseInsensitiveCompare)) {
        double audioBitrate;
        rv = mSelectedProfile->GetAudioBitrate(mQuality, &audioBitrate);
        NS_ENSURE_SUCCESS(rv, rv);
        value = sbNewVariant(audioBitrate);
        NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);
      }
      else if (mapping.Equals("quality", CaseInsensitiveCompare)) {
        value = sbNewVariant(mQuality);
        NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);
      }
      else if (mapping.Equals("video-quality", CaseInsensitiveCompare)) {
        value = sbNewVariant(mVideoQuality);
        NS_ENSURE_TRUE(value, NS_ERROR_OUT_OF_MEMORY);
      }
      else {
        TRACE(("%s[%p]: mapping %s not implemented",
               __FUNCTION__, this, mapping.get()));
        return NS_ERROR_NOT_IMPLEMENTED;
      }
    }

    // do any scaling
    nsCString scaleString;
    rv = prop->GetScale(scaleString);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!scaleString.IsEmpty()) {
      sbFraction scale;
      rv = sbFractionFromString(scaleString, scale);
      NS_ENSURE_SUCCESS(rv, rv);
      double val;
      rv = value->GetAsDouble(&val);
      NS_ENSURE_SUCCESS(rv, rv);
      val *= scale;
      nsCOMPtr<nsIWritableVariant> var = do_QueryInterface(value, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = var->SetAsDouble(val);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // the gstreamer side wants the properties to be the right type :|
    switch (dataType) {
      case nsIDataType::VTYPE_INT32: {
        nsCOMPtr<nsIWritableVariant> var = do_QueryInterface(value, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        PRInt32 val;
        rv = var->GetAsInt32(&val);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<nsIVariant> limit;
        rv = prop->GetValueMax(getter_AddRefs(limit));
        if (NS_SUCCEEDED(rv) && limit) {
          PRInt32 maxInt;
          rv = limit->GetAsInt32(&maxInt);
          if (NS_SUCCEEDED(rv) && val > maxInt) {
            val = maxInt;
          }
        }
        rv = prop->GetValueMin(getter_AddRefs(limit));
        if (NS_SUCCEEDED(rv) && limit) {
          PRInt32 minInt;
          rv = limit->GetAsInt32(&minInt);
          if (NS_SUCCEEDED(rv) && val < minInt) {
            val = minInt;
          }
        }
        rv = var->SetAsInt32(val);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
    }

    rv = aDstBag->SetProperty(propName, value);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::GetAvailableProfiles(nsIArray * *aAvailableProfiles)
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
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITranscodeProfileLoader> profileLoader =
      do_CreateInstance("@songbirdnest.com/Songbird/Transcode/ProfileLoader;1",
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

    nsCOMPtr<sbITranscodeEncoderProfile> encoderProfile =
      do_QueryInterface(profile);
    NS_ENSURE_TRUE(encoderProfile, NS_ERROR_NO_INTERFACE);
    rv = EnsureProfileAvailable(encoderProfile);
    if (NS_FAILED(rv)) {
      // Not able to use this profile; don't return it.
      continue;
    }

    rv = array->AppendElement(encoderProfile, PR_FALSE);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  mAvailableProfiles = do_QueryInterface(array, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  NS_ADDREF(*aAvailableProfiles = mAvailableProfiles);

  return NS_OK;
}

/* attribute duoble quality; */
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::GetQuality(double *aQuality)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aQuality);
  *aQuality = mQuality;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::SetQuality(double aQuality)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_FALSE(mConfigurateState > CONFIGURATE_NOT_STARTED,
                  NS_ERROR_ALREADY_INITIALIZED);
  mQuality = aQuality;
  return NS_OK;
}

/**** sbIDeviceTranscodingConfigurator implementation *****/
/* attribute sbIDevice device; */
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::GetDevice(sbIDevice * *aDevice)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_IF_ADDREF(*aDevice = mDevice);
  return NS_OK;
}
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::SetDevice(sbIDevice * aDevice)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_FALSE(mConfigurateState > CONFIGURATE_NOT_STARTED,
                  NS_ERROR_ALREADY_INITIALIZED);
  mDevice = aDevice;
  // clear the desired sizes
  mPreferredDimensions = Dimensions();
  return NS_OK;
}

/**** sbITranscodingConfigurator implementation *****/

/* void determineOutputType (); */
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::DetermineOutputType()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  // check our inputs
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mConfigurateState >= CONFIGURATE_OUTPUT_SET,
                  NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv;

  // Get the quality preference
  rv = SelectQuality();
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the referred encoding profile
  rv = SelectProfile();
  NS_ENSURE_SUCCESS(rv, rv);

  mConfigurateState = CONFIGURATE_OUTPUT_SET;

  return NS_OK;
}

/* void configurate (); */
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::Configurate()
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

  // Get the audio parameters
  rv = SetAudioProperties();
  NS_ENSURE_SUCCESS(rv, rv);
  // Calculate ideal video size
  rv = DetermineIdealOutputSize();
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA == rv) {
    // No video
    mVideoEncoder.SetIsVoid(PR_TRUE);
  }
  else {
    // Calculate video bitrate and scale video as necessary
    rv = FinalizeOutputSize();
    NS_ENSURE_SUCCESS(rv, rv);
    // Set video parameters
    rv = SetVideoProperties();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // all done
  mConfigurateState = CONFIGURATE_FINISHED;
  return NS_OK;
}
