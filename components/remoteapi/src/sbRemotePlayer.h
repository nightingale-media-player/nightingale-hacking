/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __SB_REMOTE_PLAYER_H__
#define __SB_REMOTE_PLAYER_H__

#include <sbIDataRemote.h>
#include <sbIPlaylistPlayback.h>
#include <sbIPlaylistWidget.h>
#include <sbIRemoteCommands.h>
#include <sbIRemotePlayer.h>
#include <sbISecurityAggregator.h>
#include <sbISecurityMixin.h>

#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsIDOMEventListener.h>
#include <nsIGenericFactory.h>
#include <nsISecurityCheckedComponent.h>
#include <nsPIDOMWindow.h>
#include <nsStringGlue.h>
#include <nsWeakReference.h>

#define SONGBIRD_REMOTEPLAYER_CONTRACTID                \
  "@songbirdnest.com/remoteapi/remoteplayer;1"
#define SONGBIRD_REMOTEPLAYER_CLASSNAME                 \
  "Songbird Remote Player"
#define SONGBIRD_REMOTEPLAYER_CID                       \
{ /* 4b67df4e-bb96-42c7-b5ef-65f95402e446 */            \
  0x4b67df4e,                                           \
  0xbb96,                                               \
  0x42c7,                                               \
  {0xb5, 0xef, 0x65, 0xf9, 0x54, 0x02, 0xe4, 0x46}      \
}

struct sbRemoteObserver {
  nsCOMPtr<nsIObserver> observer;
  nsCOMPtr<sbIDataRemote> remote;
};

class sbRemotePlayer : public sbIRemotePlayer,
                       public nsIClassInfo,
                       public nsIDOMEventListener,
                       public nsISecurityCheckedComponent,
                       public nsSupportsWeakReference,
                       public sbISecurityAggregator
{
public:
  NS_DECL_SBIREMOTEPLAYER
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSISECURITYCHECKEDCOMPONENT
  NS_DECL_SBISECURITYAGGREGATOR

  sbRemotePlayer();
  static sbRemotePlayer* GetInstance();

  static NS_METHOD Register(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char *aLoaderStr,
                            const char *aType,
                            const nsModuleComponentInfo *aInfo);
  static NS_METHOD Unregister(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char *aLoaderStr,
                              const nsModuleComponentInfo *aInfo);

protected:
  virtual ~sbRemotePlayer();

  // Helper Methods
  nsresult FireRemoteAPIAccessedEvent();
  already_AddRefed<nsPIDOMWindow> GetWindowFromJS();
  nsresult DispatchEvent(nsIDOMDocument *aDocument, const nsAString &aClass, const nsAString &aType, PRBool aIsTrusted);
  nsresult Init();
  nsresult AcquirePlaylistWidget();
  nsresult RegisterCommands(PRBool);
  nsresult UnregisterCommands();

  // Data members
  PRBool mInitialized;
  PRBool mUseDefaultCommands;
  nsCOMPtr<sbIPlaylistPlayback> mGPPS;

  // The documents for the web page and for the tabbrowser
  nsCOMPtr<nsIDOMDocument> mContentDoc;
  nsCOMPtr<nsIDOMDocument> mChromeDoc;

  // the UI playlist binding's dom element QI'd
  nsCOMPtr<sbIPlaylistWidget> mWebPlaylistWidget;

  // Like the site libraries, this may want to be a collection
  // The commands registered by the page.
  nsCOMPtr<sbIRemoteCommands> mCommandsObject;

  // Site library for the page that has been loaded
  // Theoretically there _could_ be more than one library requested by the page
  //    so this will probably have to grow to be a hashtable.
  nsCOMPtr<sbIRemoteLibrary> mSiteLibrary;

  // Hashtable to hold the observers registered by the webpage
  nsDataHashtable<nsStringHashKey, sbRemoteObserver> mRemObsHash;

  // SecurityCheckedComponent vars
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;
  nsCOMPtr<sbIDataRemote> mCurrentArtist;
  nsCOMPtr<sbIDataRemote> mCurrentAlbum;
  nsCOMPtr<sbIDataRemote> mCurrentTrack;
};

#endif // __SB_REMOTE_PLAYER_H__

