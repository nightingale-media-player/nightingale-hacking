/* vim: set sw=2 :miv */
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

#ifndef __SBDEVICEUTILS__H__
#define __SBDEVICEUTILS__H__

#include <nscore.h>
#include <nsCOMPtr.h>
#include <nsIMutableArray.h>
#include <nsStringGlue.h>
#include <prlog.h>

#include <sbIMediaListListener.h>
#include <sbITranscodingConfigurator.h>

#include "sbBaseDevice.h"
#include "sbIDeviceStatus.h"

// Mozilla forwards
class nsIFile;
class nsIMutableArray;

// Songbird forwards
class sbIDeviceLibraryMediaSyncSettings;
class sbIDeviceLibrarySyncSettings;
class sbIMediaFormat;

/**
 * Map entry figuring out the container format and codec given an extension or
 * mime type
 */
struct sbExtensionToContentFormatEntry_t {
  char const * Extension;
  char const * MimeType;
  char const * ContainerFormat;
  char const * Codec;
  char const * VideoType;
  char const * AudioType;
  PRUint32 ContentType;
  PRUint32 TranscodeType;
};

/**
 * Utilities to aid in implementing devices
 */
class sbDeviceUtils
{
public:
  /**
   * Give back a nsIFile pointing to where a file should be transferred to
   * using the media item properties to build a directory structure.
   * Note that the resulting file may reside in a directory that does not yet
   * exist.
   * @param aParent the directory to place the file
   * @param aItem the media item to be transferred
   * @return the file path to store the item in
   */
  static nsresult GetOrganizedPath(/* in */ nsIFile *aParent,
                                   /* in */ sbIMediaItem *aItem,
                                   nsIFile **_retval);

  /**
   * Sets a property on all items that match a filter in a list / library.
   * \param aMediaList      [in] The list or library to process.
   * \param aPropertyId     [in] The id of the property to set.
   * \param aPropertyValue  [in] The value of the property to set to.
   * \param aPoprertyFilter [in] A set of properties to filter on.
   *                             Pass in null to set on all items.
   *
   * For example, to set all items in a library as unavailable, call:
   * sbDeviceUtils::BulkSetProperty(library,
   *                                NS_LITERAL_STRING(SB_PROPERTY_AVAILABILITY),
   *                                NS_LITERAL_STRING("0"));
   */
  static nsresult BulkSetProperty(sbIMediaList *aMediaList,
                                  const nsAString& aPropertyId,
                                  const nsAString& aPropertyValue,
                                  sbIPropertyArray* aPropertyFilter = nsnull,
                                  PRInt32 *aAbortFlag = nsnull);

  /**
   * Delete all items that have the property with the given value
   * \param aMediaList the list to remove items from
   * \param aPropertyId the ID of the property we're interested in
   * \param aValue The value of the property that if matched will be deleted
   */
  static nsresult DeleteByProperty(sbIMediaList *aMediaList,
                                   nsAString const & aProperty,
                                   nsAString const & aValue);
  /**
   * Delete all items that are marked not available (availability == 0)
   * in a medialist / library.
   * \param aMediaList The list or library to prune.
   */
  static nsresult DeleteUnavailableItems(/* in */ sbIMediaList *aMediaList);

  /**
   * Given a device and a media item, find the sbIDeviceLibrary for it
   * this is necessary because the device library is a wrapper
   */
  static nsresult GetDeviceLibraryForItem(/* in */  sbIDevice* aDevice,
                                          /* in */  sbIMediaItem* aItem,
                                          /* out */ sbIDeviceLibrary** _retval);

  /**
   * Return in aURI a URI produced using the URI spec specified by aSpec
   * resolved relative to the base URI for the device library specified by
   * aDeviceLibrary.  The device library base URI has the following form:
   * "x-device:///<device-guid>/<library-guid>/".
   *
   * \param aDeviceLibrary      Device library for the base URI.
   * \param aSpec               URI spec.
   * \param aURI                Returned URI.
   */
  static nsresult NewDeviceLibraryURI(sbIDeviceLibrary* aDeviceLibrary,
                                      const nsCString&  aSpec,
                                      nsIURI**          aURI);

  /**
   * Given a device and a sbILibrary, find the sbIDeviceLibrary for it
   * this is necessary because the device library is a wrapper
   */
  static nsresult GetDeviceLibraryForLibrary(
                                  /* in */  sbIDevice* aDevice,
                                  /* in */  sbILibrary* aLibrary,
                                  /* out */ sbIDeviceLibrary** _retval);

  /**
   * Search the library specified by aLibrary for an item with a device
   * persisent ID property matching aDevicePersistentId.  Return the found item
   * in aItem.
   *
   * \param aLibrary            [in]  Library in which to search for items.
   * \param aDevicePersistentId [in]  Device persistent ID for which to search.
   * \param aItem               [out] Found item.
   */
  static nsresult GetMediaItemByDevicePersistentId
                    (/* in */  sbILibrary*      aLibrary,
                     /* in */  const nsAString& aDevicePersistentId,
                     /* out */ sbIMediaItem**   aItem);

  /**
   * Search the library specified by aLibrary for an item with a device
   * persistent ID property matching aDevicePersistentId.  Return that item's
   * origin media item in aItem.
   *
   * \param aLibrary            [in]  Library in which to search for items.
   * \param aDevicePersistentId [in]  Device persistent ID for which to search.
   * \param aItem               [out] Found origin item.
   */

  static nsresult GetOriginMediaItemByDevicePersistentId
                    (/* in */  sbILibrary*      aLibrary,
                     /* in */  const nsAString& aDevicePersistentId,
                     /* out */ sbIMediaItem**   aItem);

  /**
   * Return in aWriteLength the length of the media item specified by aMediaItem
   * when written to the device library specified by aDeviceLibrary.
   *
   * \param aDeviceLibrary      Device library to which media item will be
   *                            written.
   * \param aMediaItem          Media item to write to device library.
   * \param aWriteLength        Returned write length of media item.
   */
  static nsresult GetDeviceWriteLength(sbIDeviceLibrary* aDeviceLibrary,
                                       sbIMediaItem*     aMediaItem,
                                       PRUint64*         aWriteLength);

  /**
   * Ask the user what action to take in response to an operation space exceeded
   * event for the device specified by aDevice and device library specified by
   * aLibrary.  The amount of space needed for the operation is specified by
   * aSpaceNeeded and the amount available by aSpaceAvailable.  If the user
   * chooses to abort the operation, true is returned in aAbort.
   *
   * \param aDevice         [in] Target device of operation.
   * \param aLibrary        [in] Target device library of operation.
   * \param aSpaceNeeded    [in] Space needed by operation.
   * \param aSpaceAvailable [in] Space available to operation.
   * \param aAbort          [out] True if user selected to abort operation.
   */

  static nsresult QueryUserSpaceExceeded
                    (/* in */  sbIDevice*        aDevice,
                     /* in */  sbIDeviceLibrary* aLibrary,
                     /* in */  PRInt64           aSpaceNeeded,
                     /* in */  PRInt64           aSpaceAvailable,
                     /* out */ PRBool*           aAbort);

  /**
   * Ask the user if they wish to abort ripping the cd, this will be called
   * when the user cancels the rip.
   *
   * \param aAbort          [out] True if user selected to stop rip.
   */

  static nsresult QueryUserAbortRip(PRBool*     aAbort);

  /**
   * Ask the user if they wish to see the errors for a device before ejecting
   * if there are any errors.
   *
   * \param aDevice         [in] Target device of operation
   */
  static nsresult QueryUserViewErrors(sbIDevice* aDevice);

  /**
   * Show the user any errors that occured for a device.
   *
   * \param aDevice         [in] Target device of operation
   */
  static nsresult ShowDeviceErrors(sbIDevice* aDevice);

  /**
   * Set the sync partener preference to the device if the device is not linked
   * to local sync partner.
   *
   * \param aDevice                 Device to check.
   */
  static nsresult SetLinkedSyncPartner(sbIDevice* aDevice);

  /**
   * Determines if the device given supports playlists
   */
  static bool ArePlaylistsSupported(sbIDevice * aDevice);

  /**
   * \brief For a media item, get format information describing it (extension,
   *        mime type, etc.
   */
  static nsresult GetFormatTypeForItem(
                     sbIMediaItem * aItem,
                     sbExtensionToContentFormatEntry_t & aFormatType,
                     PRUint32 & aBitRate,
                     PRUint32 & aSampleRate);

  /**
   * \brief For a media item, get format information describing it (extension,
   *        mime type, etc.
   */
  static nsresult GetFormatTypeForItem(
                     sbIMediaItem * aItem,
                     sbExtensionToContentFormatEntry_t & aFormatType,
                     PRUint32 & aSampleRate,
                     PRUint32 & aChannels,
                     PRUint32 & aBitRate);

  /**
   * \brief For a URI, get format information describing it (extension,
   *        mime type, etc.
   */
  static nsresult GetFormatTypeForURI
                    (nsIURI*                            aURI,
                     sbExtensionToContentFormatEntry_t& aFormatType);

  /**
   * \brief For a URL, get format information describing it (extension,
   *        mime type, etc.
   */
  static nsresult GetFormatTypeForURL
                    (const nsAString&                   aURL,
                     sbExtensionToContentFormatEntry_t& aFormatType);

  /**
   * \brief For a MIME type, get format information describing it (extension,
   *        mime type, etc.
   */
  static nsresult GetFormatTypesForMimeType
    (const nsAString&                             aMimeType,
     const PRUint32                               aContentType,
     nsTArray<sbExtensionToContentFormatEntry_t>& aFormatTypeList);

  /**
   * Return in aMimeType the audio MIME type for the container specified by
   * aContainer and codec specified by aCodec.
   *
   * \param aContainer            Container type.
   * \param aCodec                Codec type.
   * \param aMimeType             Returned MIME type.
   */
  static nsresult GetAudioMimeTypeForFormatTypes(const nsAString& aContainer,
                                                 const nsAString& aCodec,
                                                 nsAString&       aMimeType);

  /**
   * \brief Determine if an item needs transcoding
   * \param aFormatType The format type mapping of the item
   * \param aBitRate the bit rate of the item in bps
   * \param aSampleRate The sample rate of the item
   * \param aDevice The device we're transcoding to
   * \param aNeedsTranscoding where we put our flag denoting it needs or does
   *        not need transcoding
   */
  static  nsresult
  DoesItemNeedTranscoding(sbExtensionToContentFormatEntry_t & aFormatType,
                          PRUint32 & aBitRate,
                          PRUint32 & aSampleRate,
                          sbIDevice * aDevice,
                          bool & aNeedsTranscoding);

  /**
    * \brief Determine if an item needs transcoding
    * NOTE: This is deprecated, use other overload
    * \param aTrascodeType The transcode type to use
    * \param aMediaFormat The media format to check
    * \param aDevice The device we're transcoding to
    * \param aNeedsTranscoding where we put our flag denoting it needs or does
    *        not need transcoding
    */
  static nsresult DoesItemNeedTranscoding(PRUint32 aTranscodeType,
                                          sbIMediaFormat * aMediaFormat,
                                          sbIDevice * aDevice,
                                          bool & aNeedsTranscoding);
  /**
   * Returns the list or transcode profiles for a particular transcoding type
   * supported by the system
   */
  static nsresult GetTranscodeProfiles(PRUint32 aType, nsIArray ** aProfiles);

  /**
   * Returns a list of transcode profiles that the device supports
   * \param aType the type of transcoding profiles to retrieve.
   * \param aDevice the device to retrieve the profiles for.
   * \param aProfiles the array of profiles that were found
   * \return NS_OK if successful else some NS_ERROR value
   */
  static nsresult GetSupportedTranscodeProfiles(PRUint32 aType,
                                                sbIDevice * aDevice,
                                                nsIArray ** aProfiles);

  /** For each transcoding profile property in aPropertyArray, look up a
   *  preference in aDevice starting with aPrefNameBase, and set the property
   *  value to the preference value if any.
   */
  static nsresult ApplyPropertyPreferencesToProfile(sbIDevice *aDevice,
                                                    nsIArray *aPropertyArray,
                                                    nsString aPrefNameBase);

  /** Get the appropriate file extension for files ended with this profile.
   */
  static nsresult GetTranscodedFileExtension(sbITranscodeProfile *aProfile,
                                             nsCString &aExtension);

  /** Get the most-likely container type and codec type for a given mime type.
   * Returns NS_ERROR_NOT_AVAILABLE if not known for this mime type
   */
  static nsresult GetCodecAndContainerForMimeType(nsCString aMimeType,
                                                  nsCString &aContainer,
                                                  nsCString &aCodec,
                                                  nsCString &aVideoType,
                                                  nsCString &aAudioType);

  /* Get an appropriate transcoding configurator for this type of transcoding */
  static nsresult GetTranscodingConfigurator(
                              PRUint32 aTranscodeType,
                              sbIDeviceTranscodingConfigurator **aConfigurator);


  /**
   * Returns true if the item is DRM protected
   *
   * \param aMediaItem The item to check
   * \return true if the item is DRM protected otherwise false.
   */
  static bool
  IsItemDRMProtected(sbIMediaItem * aMediaItem);

  /**
   * Gets the device capabilities type for the list content type.
   * \param aListContentType Map the device capabilities from list content type
   * \param aContentType Returns the device capabilities content type
   * \param aFunctionType Returns the device capabilities function type
   */
  static nsresult
  GetDeviceCapsTypeFromListContentType(PRUint16 aListContentType,
                                       PRUint32 *aContentType,
                                       PRUint32 *aFunctionType);

  /**
   * Gets the device capabilities type for an item
   * \param aMediaItem Map the device capabilities from the item content type
   * \param aContentType Returns the device capabilities content type
   * \param aFunctionType Returns the device capabilities function type
   */
  static nsresult
  GetDeviceCapsTypeFromMediaItem(sbIMediaItem *aMediaItem,
                                 PRUint32 *aContentType,
                                 PRUint32 *aFunctionType);

  /**
   * Returns true if the device supports the media item.
   * \param aDevice The device to look up capabilities with.
   * \param aMediaItem The media item to check for.
   * \return True if the media item is supported on the device.
   */
  static PRBool
  IsMediaItemSupported(sbIDevice *aDevice,
                       sbIMediaItem *aMediaItem);

  /**
   * Returns true if the device supports the content type of the media list.
   * \param aDevice The device to look up capabilities with.
   * \param aListContentType The list content type to check for.
   * \return True if the content type is supported on the device.
   */
  static PRBool
  IsMediaListContentTypeSupported(sbIDevice *aDevice,
                                  PRUint16 aListContentType);

  /**
   * Add to the file extension list specified by aFileExtensionList the list of
   * file extensions of the content type specified by aContentType supported by
   * the device specified by aDevice.
   *
   * \param aDevice             Device for which to add file extensions.
   * \param aContentType        Content type for which to add file extensions.
   * \param aFileExtensionList  List of file extensions to which to add.
   */
  static nsresult AddSupportedFileExtensions
                    (sbIDevice*          aDevice,
                     PRUint32            aContentType,
                     nsTArray<nsString>& aFileExtensionList);

  /**
   * Helper function to return the media settings object for a library for a
   * given type
   * \param aDevLib the device library to retrieve the setttings
   * \param aMediaType The media type you're interested in
   * \param aMediaSettings The returned media sync settings
   */
  static nsresult GetMediaSettings(
                            sbIDeviceLibrary * aDevLib,
                            PRUint32 aMediaType,
                            sbIDeviceLibraryMediaSyncSettings ** aMediaSettings);

  /**
   * Helper function to return the management type for a given device library
   * and media type
   * \param aDevLib the device library to retrieve the setttings
   * \param aMediaType The media type you're interested in
   * \param aMgmtType The management type returned
   */

  static nsresult GetMgmtTypeForMedia(sbIDeviceLibrary * aDevLib,
                                      PRUint32 aMediaType,
                                      PRUint32 & aMgmtType);

  /**
   * Returns the device library given a library ID and an optional device ID
   * \param aDevLibGuid The device library's guid
   * \param aDevice The device to search for the library
   * \param aDeviceLibrary The library found, or null if none found
   */
  static nsresult GetDeviceLibrary(nsAString const & aDeviceLibGuid,
                                    sbIDevice * aDevice,
                                    sbIDeviceLibrary ** aDeviceLibrar);

  /**
   * Returns the device library given a library ID and an optional device ID
   * \param aDevLibGuid The device library's guid
   * \param aDeviceID An optional device ID pass null if you have none
   * \param aDeviceLibrary The library found, or null if none found
   */
  static nsresult GetDeviceLibrary(nsAString const & aDevLibGuid,
                                   nsID const * aDeviceID,
                                   sbIDeviceLibrary ** aDeviceLibrary);
#ifdef PR_LOGGING
  /**
   * Outputs a the device's capabilites to a PR_Log.
   */
  static nsresult
  LogDeviceCapabilities(sbIDeviceCapabilities *aDeviceCaps,
                        PRLogModuleInfo *aLogModule);
#endif
};

/**
 * Class to help temporary turn off listening
 */
class sbDeviceListenerIgnore
{
public:
  enum ListenerType
  {
    MEDIA_LIST = 1,
    LIBRARY = 2,
    ALL = 3
  };
  /**
   * Turns off listeners for all items
   */
  sbDeviceListenerIgnore(sbBaseDevice * aDevice,
                         PRUint32 aListenerType = ALL) :
    mDevice(aDevice),
    mIgnoring(PR_FALSE),
    mListenerType(aListenerType),
    mMediaItem(nsnull) {

    SetIgnore(PR_TRUE);
  }
  /**
   * Turns off listeners for a specific item
   */
  sbDeviceListenerIgnore(sbBaseDevice * aDevice,
                         sbIMediaItem * aItem);

  /**
   * Turns the listeners back on if they are turned off
   */
  ~sbDeviceListenerIgnore();

  /**
   * Turns the listeners on or off if they are not already
   */
  void SetIgnore(PRBool aIgnore);
private:
  sbBaseDevice * mDevice; // Non-owning pointer
  PRBool mIgnoring;
  PRUint32 mListenerType;
  sbIMediaItem * mMediaItem; // nsCOMPtr required definition of sbIMediaItem

  // Prevent copying and assignment
  sbDeviceListenerIgnore(sbDeviceListenerIgnore const &);
  sbDeviceListenerIgnore & operator =(sbDeviceListenerIgnore const &);
};

extern sbExtensionToContentFormatEntry_t const
  MAP_FILE_EXTENSION_CONTENT_FORMAT[];
extern PRUint32 const MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH;

#endif /* __SBDEVICEUTILS__H__ */
