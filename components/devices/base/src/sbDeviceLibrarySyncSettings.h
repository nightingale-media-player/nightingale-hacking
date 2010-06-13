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
   * Returns true if the settings have changed
   */
  bool HasChanged() const;
  bool HasChangedNoLock() const
  {
    return mChanged;
  }
  /**
   * Resets the changed flag, this does not revert the settings themselvves
   */
  void ResetChanged();
  void ResetChangedNoLock();
  /**
   * Marks the settings object as modified and notifies the device library
   * if set to do so
   */
  void Changed(PRBool forceNotify = PR_FALSE);

  void NotifyDeviceLibrary()
  {
    mNotifyDeviceLibrary = true;
  }
  /**
   * Reads in the sync management settings and builds out the various objects
   * \param aDevice The device associated with the settings
   * \param aDeviceLibrary Optional device library. Can be null if device has
   *                       completed setup
   */
  nsresult Read(sbIDevice * aDevice,
                sbIDeviceLibrary * aDeviceLibrary);

  /**
   * Writes the sync management settings and builds out the various objects
   * \param aDevice The device associated with the settings
   */
  nsresult Write(sbIDevice * aDevice);
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

  nsresult GetSyncModePrefKey(nsAString& aPrefKey);

  nsresult GetMgmtTypePref(sbIDevice * aDevice,
                           PRUint32 aContentType,
                           PRUint32 & aMgmtTypes);

  nsresult ReadLegacySyncMode(sbIDevice * aDevice, PRUint32 & aSyncMode);

  static nsresult ReadPRUint32(sbIDevice * aDevice,
                               nsAString const & aPrefKey,
                               PRUint32 & aInt,
                               PRUint32 aDefault);

  static nsresult ReadAString(sbIDevice * aDevice,
                              nsAString const & aPrefKey,
                              nsAString & aString,
                              nsAString const & aDefault);

  template <class T>
  nsresult WritePref(sbIDevice * aDevice,
                     nsAString const & aPrefKey,
                     T aValue);

  nsresult ReadSyncMode(sbIDevice * aDevice, PRUint32 & aSyncMode);

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

  nsresult GetSyncListsPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  nsresult GetSyncFromFolderPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  nsresult GetSyncFolderPrefKey(PRUint32 aContentType, nsAString& aPrefKey);

  nsTArray<nsRefPtr<sbDeviceLibraryMediaSyncSettings> > mMediaSettings;

  nsID mDeviceID;
  nsString mDeviceLibraryGuid;
  PRUint32 mSyncMode;
  bool mChanged;
  bool mNotifyDeviceLibrary;
  PRLock * mLock;
};

#endif /* SBDEVICELIBRARYSYNCSETTINGS_H_ */
