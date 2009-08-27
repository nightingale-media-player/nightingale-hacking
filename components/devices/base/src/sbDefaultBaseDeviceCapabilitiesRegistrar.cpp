/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#include "sbDefaultBaseDeviceCapabilitiesRegistrar.h"

// Mozilla includes
#include <nsArrayUtils.h>
#include <nsIVariant.h>
#include <nsMemory.h>

// Songbird includes
#include <sbIDevice.h>
#include <sbIDeviceCapabilities.h>
#include <sbIMediaItem.h>
#include <sbITranscodeManager.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>

PRInt32 const K = 1000;

sbDefaultBaseDeviceCapabilitiesRegistrar::
  sbDefaultBaseDeviceCapabilitiesRegistrar() :
    mLock(nsAutoLock::NewLock("sbDefaultBaseDeviceCapabilitiesRegistrar")),
    mTranscodeProfilesBuilt(false)
{
}

sbDefaultBaseDeviceCapabilitiesRegistrar::
  ~sbDefaultBaseDeviceCapabilitiesRegistrar()
{
  nsAutoLock::DestroyLock(mLock);
}

/* readonly attribute PRUint32 type; */
NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::GetType(PRUint32 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = sbIDeviceCapabilitiesRegistrar::DEFAULT;
  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::
  AddCapabilities(sbIDevice *aDevice,
                  sbIDeviceCapabilities *aCapabilities) {
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv;

  // Look for capabilities settings in the device preferences
  nsCOMPtr<nsIVariant> capabilitiesVariant;
  rv = aDevice->GetPreference(NS_LITERAL_STRING("capabilities"),
                              getter_AddRefs(capabilitiesVariant));
  if (NS_SUCCEEDED(rv)) {
    PRUint16 dataType;
    rv = capabilitiesVariant->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    if ((dataType == nsIDataType::VTYPE_INTERFACE) ||
        (dataType == nsIDataType::VTYPE_INTERFACE_IS)) {
      nsCOMPtr<nsISupports>           capabilitiesISupports;
      nsCOMPtr<sbIDeviceCapabilities> capabilities;
      rv = capabilitiesVariant->GetAsISupports
                                  (getter_AddRefs(capabilitiesISupports));
      NS_ENSURE_SUCCESS(rv, rv);
      capabilities = do_QueryInterface(capabilitiesISupports, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aCapabilities->AddCapabilities(capabilities);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  // Nothing in the capabilities preferences, so use defaults
  static PRUint32 functionTypes[] = {
    sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK,
    sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY
  };

  rv = aCapabilities->SetFunctionTypes(functionTypes,
                                       NS_ARRAY_LENGTH(functionTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  static PRUint32 audioContentTypes[] = {
    sbIDeviceCapabilities::CONTENT_AUDIO
  };

  rv = aCapabilities->AddContentTypes(sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK,
                                      audioContentTypes,
                                      NS_ARRAY_LENGTH(audioContentTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  static PRUint32 imageContentTypes[] = {
    sbIDeviceCapabilities::CONTENT_IMAGE,
  };

  rv = aCapabilities->AddContentTypes(sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY,
                                      imageContentTypes,
                                      NS_ARRAY_LENGTH(imageContentTypes));
  NS_ENSURE_SUCCESS(rv, rv);

  static char const * audioFormats[] = {
    "audio/mpeg",
  };

  rv = aCapabilities->AddFormats(sbIDeviceCapabilities::CONTENT_AUDIO,
                                 audioFormats,
                                 NS_ARRAY_LENGTH(audioFormats));
  NS_ENSURE_SUCCESS(rv, rv);

  static char const * imageFormats[] = {
    "image/jpeg"
  };
  rv = aCapabilities->AddFormats(sbIDeviceCapabilities::CONTENT_IMAGE,
                                 imageFormats,
                                 NS_ARRAY_LENGTH(imageFormats));
  NS_ENSURE_SUCCESS(rv, rv);

  /**
   * Default MP3 bit rates
   */
  static PRInt32 DefaultMinMP3BitRate = 8 * K;
  static PRInt32 DefaultMaxMP3BitRate = 320 * K;

  struct sbSampleRates
  {
    PRUint32 const Rate;
    char const * const TextValue;
  };
  /**
   * Default MP3 sample rates
   */
  static PRUint32 DefaultMP3SampleRates[] =
  {
    8000,
    11025,
    12000,
    16000,
    22050,
    24000,
    32000,
    44100,
    48000
  };
  // Build the MP3 bit rate range
  nsCOMPtr<sbIDevCapRange> bitRateRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = bitRateRange->Initialize(DefaultMinMP3BitRate, DefaultMaxMP3BitRate, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build the MP3 sample rate range
  nsCOMPtr<sbIDevCapRange> sampleRateRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  for (PRUint32 index = 0;
       index < NS_ARRAY_LENGTH(DefaultMP3SampleRates);
       ++index) {
    rv = sampleRateRange->AddValue(DefaultMP3SampleRates[index]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Build the MP3 Channel range
  nsCOMPtr<sbIDevCapRange> channelRange =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = channelRange->Initialize(1, 2, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIAudioFormatType> audioFormatType =
    do_CreateInstance(SB_IAUDIOFORMATTYPE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = audioFormatType->Initialize(NS_LITERAL_CSTRING("id3"),  // container type
                                   NS_LITERAL_CSTRING("mp3"),  // codec
                                   bitRateRange,
                                   sampleRateRange,
                                   channelRange,
                                   nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aCapabilities->AddFormatType(NS_LITERAL_STRING("audio/mpeg"),
                                    audioFormatType);
  NS_ENSURE_SUCCESS(rv, rv);

  /* We could add a default image (album art) format here too. However, we don't
     - we have no information about what the device might actually support. With
     no formats declared, we simply don't change the album art at all (so if the
     format/size IS supported by the device, it'll work fine).
     Specific devices should override this with device-specific information.
   */
  return NS_OK;
}

sbExtensionToContentFormatEntry_t const
MAP_FILE_EXTENSION_CONTENT_FORMAT[] = {
  /* audio */
  { "mp3",  "audio/mpeg",      "id3",  "mp3",    sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wma",  "audio/x-ms-wma",  "asf",  "wmav2",  sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aac",  "audio/aac",       "mov",  "aac",    sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "m4a",  "audio/aac",       "mov",  "aac",    sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aa",   "audio/audible",   "",     "",       sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "ogg",  "application/ogg", "ogg",  "vorbis", sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "flac", "audio/x-flac",    "",     "flac",   sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "wav",  "audio/x-wav",     "wav",  "pcm-int",sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aiff", "audio/x-aiff",    "aiff", "pcm-int",sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },
  { "aif",  "audio/x-aiff",    "aiff", "pcm-int",sbITranscodeProfile::TRANSCODE_TYPE_AUDIO },

  /* video */
  { "mp4",  "video/mp4",       "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "mpg",  "video/mpeg",      "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "mpeg", "video/mpeg",      "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "wmv",  "video/x-ms-wmv",  "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "avi",  "video/x-msvideo", "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "3gp",  "video/3gpp",      "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "3g2",  "video/3gpp",      "",    "",      sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },

  /* images */
  { "png",  "image/png",      "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "jpg",  "image/jpeg",     "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "gif",  "image/gif",      "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "bmp",  "image/bmp",      "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "ico",  "image/x-icon",   "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "tiff", "image/tiff",     "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "tif",  "image/tiff",     "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "wmf",  "application/x-msmetafile", "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "jp2",  "image/jp2",      "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "jpx",  "image/jpx",      "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "fpx",  "application/vnd.netfpx", "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "pcd",  "image/x-photo-cd", "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO },
  { "pict", "image/pict",     "", "", sbITranscodeProfile::TRANSCODE_TYPE_VIDEO }
};

PRUint32 const MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH =
  NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
/**
 * Convert the string bit rate to an integer
 * \param aBitRate the string version of the bit rate
 * \return the integer version of the bit rate or 0 if it fails
 */
PRUint32 ParseBitRate(nsAString const & aBitRate)
{
  nsresult rv;

  if (aBitRate.IsEmpty()) {
    return 0;
  }
  PRUint32 rate = aBitRate.ToInteger(&rv, 10);
  if (NS_FAILED(rv)) {
    rate = 0;
  }
  return rate * K;
}

/**
 * Convert the string sample rate to an integer
 * \param aSampleRate the string version of the sample rate
 * \return the integer version of the sample rate or 0 if it fails
 */
PRUint32 ParseSampleRate(nsAString const & aSampleRate) {
  nsresult rv;
  if (aSampleRate.IsEmpty()) {
    return 0;
  }
  PRUint32 rate = aSampleRate.ToInteger(&rv, 10);
  if (NS_FAILED(rv)) {
    rate = 0;
  }
  return rate;
}

/**
 * Returns the formatting information for an item
 * \param aItem The item we want the format stuff for
 * \param aFormatType the formatting map entry for the item
 * \param aBitRate The bit rate for the item
 * \param aSampleRate the sample rate for the item
 */
static nsresult
GetFormatTypeForItem(sbIMediaItem * aItem,
                     sbExtensionToContentFormatEntry_t & aFormatType,
                     PRUint32 & aBitRate,
                     PRUint32 & aSampleRate)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;
  nsCOMPtr<nsIThread> mainThread;

  // If we fail to get it from the mime type, attempt to get it from the
  // file extension.
  nsString contentURL;
  rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                          contentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 const lastDot = contentURL.RFind(NS_LITERAL_STRING("."));
  if (lastDot != -1) {
    nsDependentSubstring fileExtension(contentURL,
                                       lastDot + 1,
                                       contentURL.Length() - lastDot - 1);
    for (PRUint32 index = 0;
         index < NS_ARRAY_LENGTH(MAP_FILE_EXTENSION_CONTENT_FORMAT);
         ++index) {
      sbExtensionToContentFormatEntry_t const & entry =
        MAP_FILE_EXTENSION_CONTENT_FORMAT[index];
      if (fileExtension.EqualsLiteral(entry.Extension)) {
        aFormatType = entry;
        nsString bitRate;
        rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_BITRATE),
                                      bitRate);
        NS_ENSURE_SUCCESS(rv, rv);

        aBitRate = ParseBitRate(bitRate);

        nsString sampleRate;
        rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_SAMPLERATE),
                                      sampleRate);
        NS_ENSURE_SUCCESS(rv, rv);
        aSampleRate = ParseSampleRate(sampleRate);

        return NS_OK;
      }
    }
  }
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::InterestedInDevice(sbIDevice *aDevice,
                                                             PRBool *retval)
{
  NS_ENSURE_ARG_POINTER(retval);
  *retval = PR_FALSE;
  return NS_OK;
}

/**
 * Maps between the device caps and the transcode profile content types
 */
static PRUint32 TranscodeToCapsContentTypeMap[] = {
  sbIDeviceCapabilities::CONTENT_UNKNOWN,
  sbIDeviceCapabilities::CONTENT_AUDIO,
  sbIDeviceCapabilities::CONTENT_IMAGE,
  sbIDeviceCapabilities::CONTENT_VIDEO
};

/**
 * Gets the format information for a format type
 */
static nsresult
GetContainerFormatAndCodec(nsISupports * aFormatType,
                           PRUint32 aContentType,
                           nsAString & aContainerFormat,
                           nsAString & aCodec,
                           sbIDevCapRange ** aBitRateRange = nsnull,
                           sbIDevCapRange ** aSampleRateRange = nsnull) {

  switch (aContentType) {
    case sbIDeviceCapabilities::CONTENT_AUDIO: {
      nsCOMPtr<sbIAudioFormatType> audioFormat =
        do_QueryInterface(aFormatType);
      if (audioFormat) {
        nsCString temp;
        audioFormat->GetContainerFormat(temp);
        aContainerFormat = NS_ConvertUTF8toUTF16(temp);
        audioFormat->GetAudioCodec(temp);
        aCodec = NS_ConvertUTF8toUTF16(temp);
        if (aBitRateRange) {
          audioFormat->GetSupportedBitrates(aBitRateRange);
        }
        if (aSampleRateRange) {
          audioFormat->GetSupportedSampleRates(aSampleRateRange);
        }
      }
    }
    break;
    case sbIDeviceCapabilities::CONTENT_IMAGE: {
      nsCOMPtr<sbIImageFormatType> imageFormat =
        do_QueryInterface(aFormatType);
      if (imageFormat) {
        nsCString temp;
        imageFormat->GetImageFormat(temp);
        aContainerFormat = NS_ConvertUTF8toUTF16(temp);
        if (aBitRateRange) {
          *aBitRateRange = nsnull;
        }
        if (aSampleRateRange) {
          *aSampleRateRange = nsnull;
        }
      }
    }
    break;
    case sbIDeviceCapabilities::CONTENT_VIDEO:
      if (aBitRateRange) {
        *aBitRateRange = nsnull;
      }
      if (aSampleRateRange) {
        *aSampleRateRange = nsnull;
      }
      // TODO: For when video format types are created
      break;
    default: {
      NS_WARNING("Unexpected content type in "
                 "sbDefaultBaseDeviceCapabilitiesRegistrar::"
                 "GetSupportedTranscodeProfiles");
      if (aBitRateRange) {
        *aBitRateRange = nsnull;
      }
      if (aSampleRateRange) {
        *aSampleRateRange = nsnull;
      }
    }
    break;
  }
  return NS_OK;
}

nsresult
sbDefaultBaseDeviceCapabilitiesRegistrar::GetSupportedTranscodeProfiles
                                            (sbIDevice * aDevice,
                                             TranscodeProfiles ** aProfiles)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aProfiles);

  nsresult rv;

  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);
  nsAutoLock lock(mLock);

  // If we haven't built the profiles then go build them
  if (!mTranscodeProfilesBuilt) {

      // Don't hold lock while building the caps
    {
      nsAutoUnlock unlock(mLock);

      nsCOMPtr<sbITranscodeManager> tcManager =
        do_ProxiedGetService("@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1",
                      &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIArray> profiles;
      rv = tcManager->GetTranscodeProfiles(getter_AddRefs(profiles));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIDeviceCapabilities> devCaps;
      rv = aDevice->GetCapabilities(getter_AddRefs(devCaps));
      NS_ENSURE_SUCCESS(rv, rv);

      // These are the content types we care about
      static PRUint32 contentTypes[] = {
        sbIDeviceCapabilities::CONTENT_AUDIO,
        sbIDeviceCapabilities::CONTENT_IMAGE,
        sbIDeviceCapabilities::CONTENT_VIDEO
      };

      // Process each content type
      for (PRUint32 contentTypeIndex = 0;
           contentTypeIndex < NS_ARRAY_LENGTH(contentTypes);
           ++contentTypeIndex) {
        PRUint32 const contentType = contentTypes[contentTypeIndex];
        PRUint32 formatsLength;
        char ** formats;
        rv = devCaps->GetSupportedFormats(contentType,
                                          &formatsLength,
                                          &formats);
        // Not found error is expected, we'll not do anything in that case, but we
        // need to finish out processing and not return early
        if (rv != NS_ERROR_NOT_AVAILABLE) {
          NS_ENSURE_SUCCESS(rv, rv);
        }
        if (NS_SUCCEEDED(rv)) {
          for (PRUint32 formatIndex = 0;
               formatIndex < formatsLength && NS_SUCCEEDED(rv);
               ++formatIndex) {

            nsString format;
            format.AssignLiteral(formats[formatIndex]);
            nsMemory::Free(formats[formatIndex]);

            nsCOMPtr<nsISupports> formatTypeSupports;
            devCaps->GetFormatType(format,
                                   getter_AddRefs(formatTypeSupports));
            nsString containerFormat;
            nsString codec;
            rv = GetContainerFormatAndCodec(formatTypeSupports,
                                            contentType,
                                            containerFormat,
                                            codec);
            NS_ENSURE_SUCCESS(rv, rv);

            // Look for a match among our transcoding profile
            PRUint32 length;
            rv = profiles->GetLength(&length);
            NS_ENSURE_SUCCESS(rv, rv);

            for (PRUint32 index = 0;
                 index < length && NS_SUCCEEDED(rv);
                 ++index) {
              nsCOMPtr<sbITranscodeProfile> profile = do_QueryElementAt(profiles,
                                                                        index,
                                                                        &rv);
              NS_ENSURE_SUCCESS(rv, rv);
              nsString profileContainerFormat;
              rv = profile->GetContainerFormat(profileContainerFormat);
              NS_ENSURE_SUCCESS(rv, rv);

              PRUint32 profileType;
              rv = profile->GetType(&profileType);
              NS_ENSURE_SUCCESS(rv, rv);

              nsString audioCodec;
              rv = profile->GetAudioCodec(audioCodec);
              NS_ENSURE_SUCCESS(rv, rv);

              nsString videoCodec;
              rv = profile->GetVideoCodec(videoCodec);

              if (TranscodeToCapsContentTypeMap[profileType] == contentType &&
                profileContainerFormat.Equals(containerFormat)) {
                if ((contentType == sbIDeviceCapabilities::CONTENT_AUDIO &&
                      audioCodec.Equals(codec)) ||
                    (contentType == sbIDeviceCapabilities::CONTENT_VIDEO &&
                      videoCodec.Equals(codec))) {
                  if (mTranscodeProfiles.AppendObject(profile) == nsnull) {
                    rv = NS_ERROR_OUT_OF_MEMORY;
                  }
                }
              }
            }
          }
          nsMemory::Free(formats);
        }
      }

    } // nsAutoUnlock will unlock now

    mTranscodeProfilesBuilt = true;
  }
  *aProfiles = &mTranscodeProfiles;
  return NS_OK;
}

/**
 * Helper to determine if a value is in the range
 * \param aValue the value to check for
 * \param aRange the range to check for the value
 * \return true if the value is in the range else false
 */
inline
bool IsValueInRange(PRInt32 aValue, sbIDevCapRange * aRange) {
  if (!aValue || !aRange) {
    return true;
  }
  PRBool inRange;
  nsresult rv = aRange->IsValueInRange(aValue, &inRange);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return inRange != PR_FALSE;
}

/**
 * Determine if an item needs transcoding
 * \param aFormatType The format type mapping of the item
 * \param aBitRate the bit rate of the item
 * \param aDevice The device we're transcoding to
 * \param aNeedsTranscoding where we put our flag denoting it needs or does not
 *                          need transcoding
 */
static nsresult
DoesItemNeedTranscoding(sbExtensionToContentFormatEntry_t & aFormatType,
                        PRUint32 & aBitRate,
                        PRUint32 & aSampleRate,
                        sbIDevice * aDevice,
                        bool & aNeedsTranscoding)
{
  nsCOMPtr<sbIDeviceCapabilities> devCaps;
  nsresult rv = aDevice->GetCapabilities(getter_AddRefs(devCaps));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 const devCapContentType = TranscodeToCapsContentTypeMap[aFormatType.Type];

  nsString itemContainerFormat;
  itemContainerFormat.AssignLiteral(aFormatType.ContainerFormat);
  nsString itemCodec;
  itemCodec.AssignLiteral(aFormatType.Codec);

  PRUint32 formatsLength;
  char ** formats;
  rv = devCaps->GetSupportedFormats(devCapContentType,
                                    &formatsLength,
                                    &formats);
  // If we know of transcoding formats than check them
  if (NS_SUCCEEDED(rv) && formatsLength > 0) {
    aNeedsTranscoding = true;
    for (PRUint32 formatIndex = 0;
         formatIndex < formatsLength && NS_SUCCEEDED(rv);
         ++formatIndex) {

      nsString format;
      format.AssignLiteral(formats[formatIndex]);
      nsMemory::Free(formats[formatIndex]);
      // If we've figured out it doesn't need transcoding no need to compare
      if (!aNeedsTranscoding) {
        continue;
      }
      nsCOMPtr<nsISupports> formatType;
      rv = devCaps->GetFormatType(format, getter_AddRefs(formatType));
      if (NS_SUCCEEDED(rv)) {
        nsString containerFormat;
        nsString codec;

        nsCOMPtr<sbIDevCapRange> bitRateRange;
        nsCOMPtr<sbIDevCapRange> sampleRateRange;

        rv = GetContainerFormatAndCodec(formatType,
                                        devCapContentType,
                                        containerFormat,
                                        codec,
                                        getter_AddRefs(bitRateRange),
                                        getter_AddRefs(sampleRateRange));
        if (NS_SUCCEEDED(rv)) {
          if (containerFormat.Equals(itemContainerFormat) &&
              codec.Equals(itemCodec) &&
              IsValueInRange(aBitRate, bitRateRange) &&
              IsValueInRange(aSampleRate, sampleRateRange)) {
            aNeedsTranscoding = false;
          }
        }
      }
    }
    nsMemory::Free(formats);
  }
  else { // We don't know the transcoding formats of the device so just copy
    aNeedsTranscoding = false;
  }
  return NS_OK;
}

nsresult
sbDefaultBaseDeviceCapabilitiesRegistrar::ApplyPropertyPreferencesToProfile
                                            (sbIDevice* aDevice,
                                             nsIArray*  aPropertyArray,
                                             nsString   aPrefNameBase)
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

    nsString prefName = aPrefNameBase;
    prefName.AppendLiteral(".");
    prefName.Append(propName);

    nsCOMPtr<nsIVariant> prefVariant;
    rv = aDevice->GetPreference(prefName, getter_AddRefs(prefVariant));
    NS_ENSURE_SUCCESS(rv, rv);

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

NS_IMETHODIMP
sbDefaultBaseDeviceCapabilitiesRegistrar::ChooseProfile
                                            (sbIMediaItem*         aMediaItem,
                                             sbIDevice*            aDevice,
                                             sbITranscodeProfile** retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(retval);

  nsresult rv;

  sbExtensionToContentFormatEntry_t formatType;
  PRUint32 bitRate = 0;
  PRUint32 sampleRate = 0;
  rv = GetFormatTypeForItem(aMediaItem, formatType, bitRate, sampleRate);
  // Check for expected error, unable to find format type
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);


  bool needsTranscoding = false;
  rv = DoesItemNeedTranscoding(formatType,
                               bitRate,
                               sampleRate,
                               aDevice,
                               needsTranscoding);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!needsTranscoding) {
    *retval = nsnull;
    return NS_OK;
  }

  PRBool hasProfilePref = PR_FALSE;
  // See if we have a preference for the transcoding profile.
  nsCOMPtr<nsIVariant> profileIdVariant;
  nsString prefProfileId;
  rv = aDevice->GetPreference(NS_LITERAL_STRING("transcode_profile.profile_id"),
                              getter_AddRefs(profileIdVariant));
  if (NS_SUCCEEDED(rv))
  {
    hasProfilePref = PR_TRUE;
    rv = profileIdVariant->GetAsAString(prefProfileId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 const contentType = formatType.Type;

  TranscodeProfiles * profiles; // We don't own this, just renting it
  rv = GetSupportedTranscodeProfiles(aDevice,
                                     &profiles);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 bestPriority = 0;
  nsCOMPtr<sbITranscodeProfile> bestProfile;
  nsCOMPtr<sbITranscodeProfile> prefProfile;

  PRUint32 const length = profiles->Count();
  for (PRUint32 index = 0; index < length; ++index) {
    nsCOMPtr<sbITranscodeProfile> profile = profiles->ObjectAt(index);
    if (profile) {
      PRUint32 profileContentType;
      rv = profile->GetType(&profileContentType);
      NS_ENSURE_SUCCESS(rv, rv);

      // If the content types don't match, skip (we don't want to use a video
      // transcoding profile to transcode audio, for example)
      if (profileContentType == contentType) {
        if (hasProfilePref) {
          nsString profileId;
          rv = profile->GetId(profileId);
          NS_ENSURE_SUCCESS(rv, rv);

          if (profileId.Equals(prefProfileId))
            prefProfile = profile;
        }

        // Also track the highest-priority profile. This is our default.
        PRUint32 priority;
        rv = profile->GetPriority(&priority);
        NS_ENSURE_SUCCESS(rv, rv);

        if (!bestProfile || priority > bestPriority) {
          bestProfile = profile;
          bestPriority = priority;
        }
      }
    }
  }

  if (prefProfile) {
    // We found the profile selected in the preferences. Apply relevant
    // preferenced properties to it as well...
    nsCOMPtr<nsIArray> audioProperties;
    rv = prefProfile->GetAudioProperties(getter_AddRefs(audioProperties));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ApplyPropertyPreferencesToProfile(aDevice, audioProperties,
            NS_LITERAL_STRING("transcode_profile.audio_properties"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> videoProperties;
    rv = prefProfile->GetVideoProperties(getter_AddRefs(videoProperties));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ApplyPropertyPreferencesToProfile(aDevice, videoProperties,
            NS_LITERAL_STRING("transcode_profile.video_properties"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIArray> containerProperties;
    rv = prefProfile->GetContainerProperties(
            getter_AddRefs(containerProperties));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = ApplyPropertyPreferencesToProfile(aDevice, containerProperties,
            NS_LITERAL_STRING("transcode_profile.container_properties"));
    NS_ENSURE_SUCCESS(rv, rv);

    prefProfile.forget(retval);
    return NS_OK;
  }
  else if (bestProfile) {
    bestProfile.forget(retval);
    return NS_OK;
  }

  // Indicate no appropriate transoding profile available
  return NS_ERROR_NOT_AVAILABLE;
}

