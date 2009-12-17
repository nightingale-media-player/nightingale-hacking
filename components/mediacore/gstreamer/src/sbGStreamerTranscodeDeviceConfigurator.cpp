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

#define MOOK_HARD_CODE_CONFIGURATE

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
#include <sbITranscodeProfile.h>

///// Gecko header includes
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>

///// Songbird header includes
#include <sbArrayUtils.h>
#include <sbMemoryUtils.h>

///// GStreamer header includes
#include <gst/gst.h>

///// System includes
#include <math.h>
#include <algorithm>
#include <functional>
#include <vector>

///// Local directory includes
#include "sbGstreamerMediacoreUtils.h"

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
SB_AUTO_CLASS(sbGstCaps, GstCaps*, !!mValue, gst_caps_unref(mValue), mValue = NULL);

NS_IMPL_ISUPPORTS_INHERITED2(sbGStreamerTranscodeDeviceConfigurator,
                             sbTranscodingConfigurator,
                             sbIDeviceTranscodingConfigurator,
                             sbPIGstTranscodingConfigurator);

sbGStreamerTranscodeDeviceConfigurator::sbGStreamerTranscodeDeviceConfigurator()
  : mQuality(-HUGE_VAL),
    mVideoBitrate(PR_INT32_MIN)
{
  /* nothing */
}

sbGStreamerTranscodeDeviceConfigurator::~sbGStreamerTranscodeDeviceConfigurator()
{
  /* nothing */
}

/**
 * make a GstCaps structure from a caps name and an array of properties
 *
 * @param aCapsName [in] the name (e.g. "audio/x-raw-int")
 * @param aProps [in] the properties
 * @param aResultCaps [out] the generated GstCaps, with an outstanding refcount
 */
nsresult
MakeCapsFromProperties(const nsACString& aCapsName,
                       nsIArray *aProps,
                       GstCaps** aResultCaps)
{
  NS_ENSURE_ARG_POINTER(aProps);
  NS_ENSURE_ARG_POINTER(aResultCaps);

  nsresult rv;

  nsCOMPtr<nsISimpleEnumerator> propsEnum;
  rv = aProps->Enumerate(getter_AddRefs(propsEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // set up a caps structure
  sbGstCaps caps = gst_caps_new_simple(aCapsName.BeginReading(), NULL);
  NS_ENSURE_TRUE(caps, NS_ERROR_FAILURE);
  GstStructure* capsStruct = gst_caps_get_structure(caps.get(), 0);

  PRBool hasMore;
  while (NS_SUCCEEDED(rv = propsEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> propSupports;
    rv = propsEnum->GetNext(getter_AddRefs(propSupports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbITranscodeProfileProperty> prop =
      do_QueryInterface(propSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsString propName;
    rv = prop->GetPropertyName(propName);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIVariant> propMin, propMax;
    rv = prop->GetValueMin(getter_AddRefs(propMin));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = prop->GetValueMax(getter_AddRefs(propMax));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint16 propType;
    rv = propMin->GetDataType(&propType);
    NS_ENSURE_SUCCESS(rv, rv);
    #if DEBUG
    { // debug only - make sure min/max is the same type
      PRUint16 propMaxType;
      rv = propMax->GetDataType(&propMaxType);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(propType == propMaxType, NS_ERROR_UNEXPECTED);
    }
    #endif /* DEBUG */
    switch(propType) {
      case nsIDataType::VTYPE_INT8:    case nsIDataType::VTYPE_UINT8:
      case nsIDataType::VTYPE_INT16:   case nsIDataType::VTYPE_UINT16:
      case nsIDataType::VTYPE_INT32:   case nsIDataType::VTYPE_UINT32:
      case nsIDataType::VTYPE_INT64:   case nsIDataType::VTYPE_UINT64:
      {
        PRInt32 minValue, maxValue;
        rv = propMin->GetAsInt32(&minValue);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = propMax->GetAsInt32(&maxValue);
        NS_ENSURE_SUCCESS(rv, rv);
        gst_structure_set(capsStruct,
                          NS_LossyConvertUTF16toASCII(propName).get(),
                          GST_TYPE_INT_RANGE,
                          minValue,
                          maxValue,
                          NULL);
        break;
      }
      default:
        NS_NOTYETIMPLEMENTED("Unknown property type");
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);
  
  *aResultCaps = caps.forget();
  return NS_OK;
}

nsresult
sbGStreamerTranscodeDeviceConfigurator::EnsureProfileAvailable(sbITranscodeEncoderProfile *aProfile)
{
  NS_ENSURE_ARG_POINTER(aProfile);
  
  nsresult rv;

  // for now, only support video profiles
  PRUint32 type;
  rv = aProfile->GetType(&type);
  NS_ENSURE_SUCCESS(rv, rv);
  switch(type) {
    case sbITranscodeProfile::TRANSCODE_TYPE_VIDEO:
    case sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO:
      break;
    default:
      return NS_ERROR_NOT_IMPLEMENTED;
  }

  // check that we have a muxer available
  nsString capsName;
  rv = aProfile->GetContainerFormat(capsName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!capsName.IsEmpty()) {
    nsCOMPtr<nsIArray> props;
    rv = aProfile->GetContainerProperties(getter_AddRefs(props));
    NS_ENSURE_SUCCESS(rv, rv);

    GstCaps* caps = NULL;
    rv = MakeCapsFromProperties(NS_LossyConvertUTF16toASCII(capsName),
                                props,
                                &caps);
    NS_ENSURE_SUCCESS(rv, rv);

    const char* muxerCodecName = FindMatchingElementName(caps, "Muxer");
    if (!muxerCodecName) {
      // things like id3 are called Formatter instead, but are the same for
      // our purposes
      muxerCodecName = FindMatchingElementName(caps, "Formatter");
    }
    gst_caps_unref(caps);
    NS_ENSURE_TRUE(muxerCodecName, NS_ERROR_UNEXPECTED);
  }

  /// Check that we have an audio encoder available
  rv = aProfile->GetAudioCodec(capsName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!capsName.IsEmpty()) {
    nsCOMPtr<nsIArray> props;
    rv = aProfile->GetAudioProperties(getter_AddRefs(props));
    NS_ENSURE_SUCCESS(rv, rv);

    GstCaps* caps = NULL;
    rv = MakeCapsFromProperties(NS_LossyConvertUTF16toASCII(capsName),
                                props,
                                &caps);
    NS_ENSURE_SUCCESS(rv, rv);

    const char* audioCodecName = FindMatchingElementName(caps, "Encoder");
    gst_caps_unref(caps);
    NS_ENSURE_TRUE(audioCodecName, NS_ERROR_UNEXPECTED);
  }

  /// Check that we have an video encoder available
  rv = aProfile->GetVideoCodec(capsName);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!capsName.IsEmpty()) {
    nsCOMPtr<nsIArray> props;
    rv = aProfile->GetVideoProperties(getter_AddRefs(props));
    NS_ENSURE_SUCCESS(rv, rv);

    GstCaps* caps = NULL;
    rv = MakeCapsFromProperties(NS_LossyConvertUTF16toASCII(capsName),
                                props,
                                &caps);
    NS_ENSURE_SUCCESS(rv, rv);

    const char* videoCodecName = FindMatchingElementName(caps, "Encoder");
    gst_caps_unref(caps);
    NS_ENSURE_TRUE(videoCodecName, NS_ERROR_UNEXPECTED);
  }

  return NS_OK;
}

/**
 * Select the encoding profile to use
 *
 * @postcondition mSelectedProfile is the encoder profile to use
 * @postcondition mSelectedFormat is ths device format matching the profile
 */
nsresult
sbGStreamerTranscodeDeviceConfigurator::SelectProfile()
{
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
  PRUint32 formatCount;
  char **formatStrings;
  rv = caps->GetSupportedFormats(sbIDeviceCapabilities::CONTENT_VIDEO,
                                 &formatCount,
                                 &formatStrings);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoFreeXPCOMArrayByRef<char**> formats(formatCount, formatStrings);
  
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
    rv = profile->GetPriority(mQuality, &priority);
    NS_ENSURE_SUCCESS(rv, rv);
    if (priority <= selectedPriority) {
      continue;
    }

    nsCOMPtr<nsISupports> supports;
    for (PRUint32 formatIndex = 0; formatIndex < formatCount; ++formatIndex) {
      rv = caps->GetFormatType(NS_ConvertASCIItoUTF16(formatStrings[formatIndex]),
                               getter_AddRefs(supports));
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<sbIVideoFormatType> format = do_QueryInterface(supports, &rv);
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
      rv = profile->GetPriority(mQuality, &selectedPriority);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    // if we reach here, we have either added it to the available list or
    // the encoder profile is not compatible; either way, nothing to do here
  }

  mSelectedProfile = selectedProfile;
  if (!mSelectedProfile) {
    // no suitable encoder profile found
    return NS_ERROR_FAILURE;
  }
  mSelectedFormat = selectedFormat;
  
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
  NS_PRECONDITION(mSelectedProfile,
                  "attempted to set audio properties without selecting profile");

  nsresult rv;

  if (!mAudioEncoderProperties) {
    mAudioEncoderProperties =
      do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIWritablePropertyBag> writableBag =
    do_QueryInterface(mAudioEncoderProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIArray> propsSrc;
  rv = mSelectedProfile->GetAudioProperties(getter_AddRefs(propsSrc));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsISimpleEnumerator> propsEnum;
  rv = propsSrc->Enumerate(getter_AddRefs(propsEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasMore;
  while (NS_SUCCEEDED(rv = propsEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = propsEnum->GetNext(getter_AddRefs(supports));
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbITranscodeProfileProperty> prop =
      do_QueryInterface(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsString propName;
    rv = prop->GetPropertyName(propName);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIVariant> value;
    rv = prop->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = writableBag->SetProperty(propName, value);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // set the bitrate separately
  double bitrate;
  rv = mSelectedProfile->GetAudioBitrate(mQuality, &bitrate);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mAudioEncoderProperties->SetPropertyAsDouble(NS_LITERAL_STRING("bitrate"),
                                                    bitrate);
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
  Dimensions input;
  rv = inputFormat->GetVideoWidth(&input.width);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = inputFormat->GetVideoHeight(&input.height);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapVideoStream> outputCaps;
  rv = mSelectedFormat->GetVideoStream(getter_AddRefs(outputCaps));
  NS_ENSURE_SUCCESS(rv, rv);

  // if we can't match the PAR, adjust to square pixels
  { /* scope */
    PRUint32 count;
    char** parData = nsnull;
    rv = outputCaps->GetSupportedVideoPARs(&count, &parData);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoFreeXPCOMArray<char**> supportedPARDestroryer(count, parData);
    PRUint32 num, denom;
    rv = inputFormat->GetVideoPAR(&num, &denom);
    NS_ENSURE_SUCCESS(rv, rv);
    sbFraction inputPAR(num, denom);
    sbFraction outputPAR;
    PRUint32 index;
    for (index = 0; index < count; ++index) {
      rv = sbFractionFromString(parData[index], outputPAR);
      NS_ENSURE_SUCCESS(rv, rv);
      if (inputPAR.IsEqual(outputPAR)) {
        break;
      }
    }
    if (index >= count) {
      // we didn't find a match; just... kinda give up and blow them up into
      // square-looking pixels by duplicating pixels
      input.width *= inputPAR.Denominator();
      input.height *= inputPAR.Numerator();
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
    nsCOMPtr<sbIImageSize> size = do_QueryInterface(supports);
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
  Dimensions result, best(0, 0), maxSize;
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
    }
  }
  NS_ENSURE_SUCCESS(rv, rv);
  if (best.width < 1) {
    // best width is still empty?
    return NS_ERROR_FAILURE;
  }

  mPreferredDimensions = best;
  return NS_OK;
}

sbGStreamerTranscodeDeviceConfigurator::Dimensions
sbGStreamerTranscodeDeviceConfigurator::GetMaximumFit(
  const sbGStreamerTranscodeDeviceConfigurator::Dimensions& aInput,
  const sbGStreamerTranscodeDeviceConfigurator::Dimensions& aMaximum)
{
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
  NS_PRECONDITION(mPreferredDimensions.width > 0 &&
                    mPreferredDimensions.height > 0,
                  "FinalizeOutputSize needs preferred dimensions");
  NS_PRECONDITION(mSelectedProfile,
                  "FinalizeOutputSize called with no profile selected");
  NS_PRECONDITION(mSelectedFormat,
                  "FinalizeOutputSize called with no output format");
  nsresult rv;

  sbFraction outputFrameRate(HUGE_VAL, 1);
  // get the desired frame rate
  { /* scope */
    nsCOMPtr<sbIMediaFormatVideo> inputFormat;
    rv = mInputFormat->GetVideoStream(getter_AddRefs(inputFormat));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 num, denom;
    rv = inputFormat->GetVideoFrameRate(&num, &denom);
    NS_ENSURE_SUCCESS(rv, rv);
    sbFraction inputFrameRate(num, denom);
    nsCOMPtr<sbIDevCapVideoStream> videoCaps;
    rv = mSelectedFormat->GetVideoStream(getter_AddRefs(videoCaps));
    NS_ENSURE_SUCCESS(rv, rv);
    char ** frameRates;
    PRUint32 frameRateCount;
    rv = videoCaps->GetSupportedFrameRates(&frameRateCount, &frameRates);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoFreeXPCOMArray<char**> frameRatesDestroryer(frameRateCount, frameRates);
    
    for (PRUint32 i = 0; i < frameRateCount; ++i) {
      sbFraction candidate;
      rv = sbFractionFromString(frameRates[i], candidate);
      NS_ENSURE_SUCCESS(rv, rv);
      double lastDifference = fabs(outputFrameRate - inputFrameRate);
      double difference = fabs(candidate - inputFrameRate);
      if (difference < lastDifference) {
        outputFrameRate = candidate;
      }
    }
    mVideoFrameRate = outputFrameRate;
  }

  // get the desired bpp
  double videoBPP;
  rv = mSelectedProfile->GetVideoBitsPerPixel(mQuality, &videoBPP);
  NS_ENSURE_SUCCESS(rv, rv);

  // the bit rate desired
  if (mVideoBitrate == PR_INT32_MIN) {
    mVideoBitrate = videoBPP *
                    mPreferredDimensions.width *
                    mPreferredDimensions.height;
  }
  else {
    // we already have some sort of bitrate set
    videoBPP = mVideoBitrate /
               mPreferredDimensions.width /
               mPreferredDimensions.height;
  }
  // cap the video bit rate to what the device supports
  nsCOMPtr<sbIDevCapVideoStream> videoCaps;
  rv = mSelectedFormat->GetVideoStream(getter_AddRefs(videoCaps));
  NS_ENSURE_SUCCESS(rv, rv);
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
    PRBool hasMore;
    while (NS_SUCCEEDED(sizesEnum->HasMoreElements(&hasMore)) && sizesEnum) {
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
      mOutputDimensions = size;
      usefulPixels = usefulSize.width * usefulSize.height;
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
  
  return NS_OK;
}

/***** sbPIGstTranscodingConfigurator implementation *****/
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::GetAvailableProfiles(nsIArray * *aAvailableProfiles)
{
  if (mAvailableProfiles) {
    NS_IF_ADDREF (*aAvailableProfiles = mAvailableProfiles);
    return NS_OK;
  }

  /* If we haven't already cached it, then figure out what we have */

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
  NS_ENSURE_ARG_POINTER(aQuality);
  *aQuality = mQuality;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::SetQuality(double aQuality)
{
  NS_ENSURE_FALSE(isConfigurated, NS_ERROR_ALREADY_INITIALIZED);
  mQuality = aQuality;
  return NS_OK;
}

/**** sbIDeviceTranscodingConfigurator implementation *****/
/* attribute sbIDevice device; */
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::GetDevice(sbIDevice * *aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_IF_ADDREF(*aDevice = mDevice);
  return NS_OK;
}
NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::SetDevice(sbIDevice * aDevice)
{
  NS_ENSURE_FALSE(isConfigurated, NS_ERROR_ALREADY_INITIALIZED);
  mDevice = aDevice;
  // clear the desired sizes
  mPreferredDimensions = Dimensions();
  return NS_OK;
}


/**** sbITranscodingConfigurator implementation *****/

NS_IMETHODIMP
sbGStreamerTranscodeDeviceConfigurator::Configurate()
{
  // check our inputs
  NS_ENSURE_TRUE(mDevice, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mInputFormat, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(mQuality == -HUGE_VAL, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_FALSE(isConfigurated, NS_ERROR_ALREADY_INITIALIZED);

  nsresult rv;

#ifdef MOOK_HARD_CODE_CONFIGURATE
  // XXX Mook: temporary hack
  mMuxer = NS_LITERAL_STRING("qtmux");
  mVideoEncoder = NS_LITERAL_STRING("jpegenc");
  mAudioEncoder = NS_LITERAL_STRING("adpcmenc");

  nsCOMPtr<sbIMediaFormatVideoMutable> videoFormat =
    do_CreateInstance(SB_MEDIAFORMATVIDEO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoWidth(128);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoHeight(128);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoPAR(1, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoFormat->SetVideoFrameRate(24, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  mVideoFormat = do_QueryInterface(videoFormat, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIMediaFormatAudioMutable> audioFormat =
    do_CreateInstance(SB_MEDIAFORMATAUDIO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioFormat->SetSampleRate(22050);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioFormat->SetChannels(2);
  NS_ENSURE_SUCCESS(rv, rv);
  mAudioFormat = do_QueryInterface(audioFormat, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWritablePropertyBag2> videoProps =
    do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = videoProps->SetPropertyAsInt32(NS_LITERAL_STRING("quality"), 30);
  mVideoEncoderProperties = do_QueryInterface(videoProps, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIWritablePropertyBag2> audioProps =
    do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mAudioEncoderProperties = do_QueryInterface(audioProps, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  isConfigurated = PR_TRUE;

  return NS_OK;
#else
  // Get the referred encoding profile
  rv = SelectProfile();
  NS_ENSURE_SUCCESS(rv, rv);
  // Get the audio parameters
  rv = SetAudioProperties();
  NS_ENSURE_SUCCESS(rv, rv);
  // Calculate ideal video size
  rv = DetermineIdealOutputSize();
  NS_ENSURE_SUCCESS(rv, rv);
  // Calculate video bitrate and scale video as necessary
  rv = FinalizeOutputSize();
  NS_ENSURE_SUCCESS(rv, rv);
  // Set video parameters
  
  return NS_ERROR_NOT_IMPLEMENTED;
#endif
}
