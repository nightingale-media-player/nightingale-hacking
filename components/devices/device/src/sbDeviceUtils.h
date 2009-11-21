/* vim: set sw=2 :miv */
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

#ifndef __SBDEVICEUTILS__H__
#define __SBDEVICEUTILS__H__

#include <nscore.h>
#include <nsCOMPtr.h>
#include <nsIMutableArray.h>
#include <nsStringGlue.h>

#include <sbIMediaListListener.h>

#include "sbBaseDevice.h"
#include "sbIDeviceStatus.h"

// Mozilla forwards
class nsIFile;
class nsIMutableArray;

// Songbird forwards
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
   * Check if the device specified by aDevice is linked to the local sync
   * partner.  If it is, return true in aIsLinkedLocally; otherwise, return
   * false.  If aRequestPartnerChange is true and the device is not linked
   * locally, make a request to the user to change the device sync partner to
   * the local sync partner.
   *
   * \param aDevice                 Device to check.
   * \param aRequestPartnerChange   Request that the sync partner be changed to
   *                                the local sync partner.
   * \param aIsLinkedLocally        Returned true if the device is linked to the
   *                                local sync partner.
   */
  static nsresult SyncCheckLinkedPartner(sbIDevice* aDevice,
                                         PRBool     aRequestPartnerChange,
                                         PRBool*    aIsLinkedLocally);

  /**
   * Make a request of the user to change the sync partner of the device
   * specified by aDevice to the local sync partner.  If the user grants the
   * request, return true in aPartnerChangeGranted; otherwise, return false.
   *
   * \param aDevice                 Device.
   * \param aPartnerChangeGranted   Returned true if the user granted the
   *                                request.
   */
  static nsresult SyncRequestPartnerChange(sbIDevice* aDevice,
                                           PRBool*    aPartnerChangeGranted);
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
  static nsresult GetFormatTypeForMimeType
                    (const nsAString&                   aMimeType,
                     sbExtensionToContentFormatEntry_t& aFormatType);

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
   * Returns a list of transcode profiles that the device supports
   * \param aDevice the device to retrieve the profiles for.
   * \param aProfiles the array of profiles that were found
   * \return NS_OK if successful else some NS_ERROR value
   */
  static nsresult GetSupportedTranscodeProfiles(sbIDevice * aDevice,
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
                                                  nsCString &aCodec);

  /**
   * Returns true if the item is DRM protected
   *
   * \param aMediaItem The item to check
   * \return true if the item is DRM protected otherwise false.
   */
  static bool
  IsItemDRMProtected(sbIMediaItem * aMediaItem);

  /**
   * Returns the transcode type for the item
   */
  static PRUint32
  GetTranscodeType(sbIMediaItem * aMediaItem);

  /**
   * Returns the device capabilities type for an item
   */
  static PRUint32
  GetDeviceCapsMediaType(sbIMediaItem * aMediaItem) ;
};

extern sbExtensionToContentFormatEntry_t const
  MAP_FILE_EXTENSION_CONTENT_FORMAT[];
extern PRUint32 const MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH;

#endif /* __SBDEVICEUTILS__H__ */
