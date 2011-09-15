/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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
#ifndef SBDEVICELIBRARYSYNCSETTINGS_H_
#define SBDEVICELIBRARYSYNCSETTINGS_H_

// Mozilla includes
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsIArray.h>
#include <nsInterfaceHashtable.h>
#include <nsTArray.h>

// Songbird includes
#include <sbIDeviceLibrary.h>
#include <sbIDeviceLibrarySyncSettings.h>
#include <sbIDeviceLibraryMediaSyncSettings.h>

class sbDeviceLibraryMediaSyncSettings;
class sbDeviceLibrary;

class sbDeviceLibrarySyncSettings : public sbIDeviceLibrarySyncSettings
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICELIBRARYSYNCSETTINGS

  nsresult Assign(sbDeviceLibrarySyncSettings * aSource);
  nsresult CreateCopy(sbDeviceLibrarySyncSettings ** aSettings);
  PRLock * GetLock()
  {
    return mLock;
  }

  /**
   * Reads in the sync management settings and builds out the various objects
   * \param aDevice The device associated with the settings
   * \param aDeviceLibrary Optional device library. Can be null if device has
   *                       completed setup
   */
  nsresult Read(sbIDevice * aDevice,
                sbIDeviceLibrary * aDeviceLibrary);

  static  sbDeviceLibrarySyncSettings * New(
                                          nsID const & aDeviceID,
                                          nsAString const & aDeviceLibraryGuid);
private:
  sbDeviceLibrarySyncSettings(nsID const & aDeviceID,
                              nsAString const & aDeviceLibraryGuid);

  ~sbDeviceLibrarySyncSettings();
  nsresult GetMediaSettingsNoLock(
                           PRUint32 aMediaType,
                           sbIDeviceLibraryMediaSyncSettings ** aMediaSettings);

  nsresult GetMgmtTypePref(sbIDevice * aDevice,
                           PRUint32 aContentType,
                           PRUint32 & aMgmtTypes);

  /**
   * Returns the import flag for a given content type that was stored as a pref.
   * This defaults to not importing if the pref does not exist.
   * \param aDevice The device used to retrieve the preference from
   * \param aMediaType The media type to look up the import flag.
   *                   sbIDeviceLibrary::MEDIATYPE_*
   * \param aImport The returned import flag
   */
  nsresult GetImportPref(sbIDevice * aDevice,
                         PRUint32 aMediaType,
                         PRBool & aImport);

  static nsresult ReadAString(sbIDevice * aDevice,
                              nsAString const & aPrefKey,
                              nsAString & aString,
                              nsAString const & aDefault);

  template <class T>
  nsresult WritePref(sbIDevice * aDevice,
                     nsAString const & aPrefKey,
                     T aValue);

  nsresult WriteMediaSyncSettings(
                         sbIDevice * aDevice,
                         PRUint32 aMediaType,
                         sbDeviceLibraryMediaSyncSettings * aMediaSyncSettings);

  nsresult ReadMediaSyncSettings(
                        sbIDevice * aDevice,
                        sbIDeviceLibrary * aDeviceLibrary,
                        PRUint32 aMediaType,
                        sbDeviceLibraryMediaSyncSettings ** aMediaSyncSettings);

  nsresult GetMgmtTypePrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  /**
   * Returns the preference key for the import flag of media settings
   * \param aMediaType The media type to get the key
   *                   sbIDeviceLibrary::MEDIATYPE_*
   * \param aPrefKey The returned preference key
   */
  nsresult GetImportPrefKey(PRUint32 aMediaType,
                            nsAString& aPrefKey);

  nsresult GetSyncListsPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  nsresult GetSyncFromFolderPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  nsresult GetSyncFolderPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  nsTArray<nsRefPtr<sbDeviceLibraryMediaSyncSettings> > mMediaSettings;

  nsID mDeviceID;
  nsString mDeviceLibraryGuid;
  PRLock * mLock;
};

#endif /* SBDEVICELIBRARYSYNCSETTINGS_H_ */
