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

#include "sbIDeviceLibrary.h"
#include "sbILocalDatabaseSimpleMediaList.h"

#include <nsCOMPtr.h>
#include <nsWeakReference.h>

class sbBaseDevice;
class sbIDevice;

class sbBaseDeviceLibraryListener : public sbIDeviceLibraryListener,
                                    public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICELIBRARYLISTENER

  sbBaseDeviceLibraryListener();
  virtual ~sbBaseDeviceLibraryListener();

  nsresult Init(sbBaseDevice* aDevice);

  nsresult SetIgnoreListener(PRBool aIgnoreListener);

protected:
  // The device owns the listener, so use a non-owning reference here
  sbBaseDevice* mDevice;

  PRBool mIgnoreListener;
};

class sbDeviceBaseLibraryCopyListener : public sbILocalDatabaseMediaListCopyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEMEDIALISTCOPYLISTENER

  sbDeviceBaseLibraryCopyListener();
  virtual ~sbDeviceBaseLibraryCopyListener();

  nsresult Init(sbBaseDevice* aDevice);

protected:
  // The device owns the listener, so use a non-owning reference here
  sbBaseDevice* mDevice;

};
