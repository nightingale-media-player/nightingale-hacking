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

#ifndef __SBDEVICELIBRARY_H__
#define __SBDEVICELIBRARY_H__

#include <sbIDeviceLibrary.h>
#include <sbILibrary.h>
#include <sbIMediaListListener.h>
#include <sbILocalDatabaseSimpleMediaList.h>

#include <nsInterfaceHashtable.h>
#include <prlock.h>

class sbDeviceLibrary : public sbIDeviceLibrary,
                        public sbIMediaListListener,
                        public sbILocalDatabaseMediaListCopyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICELIBRARY
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBILOCALDATABASEMEDIALISTCOPYLISTENER

  sbDeviceLibrary();
  virtual ~sbDeviceLibrary();

  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mDeviceLibrary)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mDeviceLibrary)
  NS_FORWARD_SAFE_SBIMEDIALIST(mDeviceLibrary)
  NS_FORWARD_SAFE_SBILIBRARY(mDeviceLibrary)

  nsresult Init(const nsAString& aDeviceIdentifier);

private:

  /**
   * \brief This callback adds the enumerated listeners to an nsCOMArray.
   *
   * \param aKey      - An nsISupport pointer to a listener for the key.
   * \param aEntry    - An sbIDeviceLibrary entry.
   * \param aUserData - An nsCOMArray to hold the enumerated pointers.
   *
   * \return PL_DHASH_NEXT on success
   * \return PL_DHASH_STOP on failure
   */
  static PLDHashOperator PR_CALLBACK
    AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                   sbIDeviceLibraryListener* aEntry,
                                   void* aUserData);

  /**
   * \brief Create a library for a device instance.
   *
   * Creating a library provides you with storage for all data relating
   * to media items present on the device. After creating a library you will
   * typically want to register it so that it may be shown to the user.
   * 
   * When a library is created, two listeners are added to it. One listener
   * takes care of advising the sbIDeviceLibrary interface instance when items
   * need to be transferred to it, deleted from it or updated because data
   * relating to those items have changed.
   *
   * The second listener is responsible for detecting when items are transferred
   * from the devices library to another library.
   * 
   * \param aDeviceDatabaseURI Optional URI for the database.
   * \sa RemoveDeviceLibrary, RegisterDeviceLibrary, UnregisterDeviceLibrary
   */
  nsresult CreateDeviceLibrary(const nsAString &aDeviceIdentifier,
                               nsIURI *aDeviceDatabaseURI);

  /**
   * \brief Register the device library with the library manager.
   *
   * Registering the device library with the library manager enables the user
   * to view the library. It becomes accessible to others programmatically as well
   * through the library manager.
   *
   * \param aDeviceLibrary The library instance to register.
   */
  nsresult RegisterDeviceLibrary(sbILibrary* aDeviceLibrary);

  /**
   * \brief Unregister a device library with the library manager.
   *
   * Unregister a device library with the library manager will make the library
   * vanish from the list of libraries and block out access to it programatically 
   * as well.
   * 
   * A device should always unregister the device library when the application
   * shuts down, when the device is disconnected suddenly and when the user ejects
   * the device.
   *
   * \param aDeviceLibrary The library instance to unregister.
   */
  nsresult UnregisterDeviceLibrary(sbILibrary* aDeviceLibrary);

  /**
   * \brief library for this device, location is specified by the
   *        aDeviceDatabaseURI param for CreateDeviceLibrary or the default
   *        location under the users profile.
   */
  nsCOMPtr<sbILibrary> mDeviceLibrary;

  /**
   * \brief A list of listeners.
   */
  nsInterfaceHashtable<nsISupportsHashKey, sbIDeviceLibraryListener> mListeners;

  PRLock* mLock;
};

#endif /* __SBDEVICELIBRARY_H__ */
