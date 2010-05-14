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

#ifndef SBDEVICELIBRARYMEDIASYNCSETTINGS_H_
#define SBDEVICELIBRARYMEDIASYNCSETTINGS_H_

// Mozilla includes
#include <nsDataHashtable.h>
#include <nsStringAPI.h>
#include <nsTArray.h>

// Somgbird includes
#include <sbIDeviceLibraryMediaSyncSettings.h>

// Forwards
struct PRLock;
class sbIDevice;
class sbDeviceLibrarySyncSettings;

class sbDeviceLibraryMediaSyncSettings : public sbIDeviceLibraryMediaSyncSettings
{
public:
  // This allows our owner to much with us within it's lock
  friend class sbDeviceLibrarySyncSettings;
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICELIBRARYMEDIASYNCSETTINGS;

  nsresult Read(sbIDevice * aDevice);
  typedef nsDataHashtable<nsISupportsHashKey, PRBool> PlaylistSelection;

private:
  sbDeviceLibraryMediaSyncSettings(sbDeviceLibrarySyncSettings * aSyncSettings,
                                   PRUint32 aMediaType,
                                   PRLock * aLock);
  ~sbDeviceLibraryMediaSyncSettings();

  /**
   * Private interface to only be used by sbDeviceLibrarySyncSettings
   */
  static sbDeviceLibraryMediaSyncSettings * New(
                                    sbDeviceLibrarySyncSettings * aSyncSettings,
                                    PRUint32 aMediaType,
                                    PRLock * aLock);
  nsresult Assign(sbDeviceLibraryMediaSyncSettings * aSettings);
  nsresult CreateCopy(sbDeviceLibraryMediaSyncSettings ** aSettings);
  void ResetChanged() {
    mChanged = false;
  }
  bool HasChanged() const
  {
    return mChanged;
  }
  void Changed();
  nsresult GetSyncPlaylistsNoLock(nsIArray ** aSyncPlaylists);
  /**
   * Management type, SYNC_MGMT_NONE, SYNC_MGMT_ALL, SYNC_MGMT_PLAYLISTS
   */
  PRUint32 mSyncMgmtType;
  PRUint32 mMediaType;
  PlaylistSelection mPlaylistsSelection;
  nsString mSyncFolder;
  nsCOMPtr<nsIFile> mSyncFromFolder;
  bool mChanged;
  PRLock * mLock;
  // Non-owning reference to our owner. We should never live past
  sbDeviceLibrarySyncSettings * mSyncSettings;
};

#endif /* SBDEVICELIBRARYMEDIASYNCSETTINGS_H_ */
