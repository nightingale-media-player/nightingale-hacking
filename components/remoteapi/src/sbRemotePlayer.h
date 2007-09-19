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

#include "sbRemoteAPI.h"
#include "sbRemoteNotificationManager.h"

#include <sbIDataRemote.h>
#include <sbIMediaList.h>
#include <sbIPlaylistPlayback.h>
#include <sbIPlaylistWidget.h>
#include <sbIRemoteLibrary.h>
#include <sbIRemotePlayer.h>
#include <sbISecurityAggregator.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsIClassInfo.h>
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
{ /* 645e064c-e547-444c-bb41-8f2e5b12700b */            \
  0x645e064c,                                           \
  0xe547,                                               \
  0x444c,                                               \
  {0xbb, 0x41, 0x8f, 0x2e, 0x5b, 0x12, 0x70, 0x0b}      \
}

struct sbRemoteObserver {
  nsCOMPtr<nsIObserver> observer;
  nsCOMPtr<sbIDataRemote> remote;
};

class sbRemoteCommands;
class sbIDataRemote;

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
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)
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
  static nsresult DispatchEvent( nsIDOMDocument *aDocument,
                                 const nsAString &aClass,
                                 const nsAString &aType,
                                 PRBool aIsTrusted );
  sbRemoteNotificationManager* GetNotificationManager();

protected:
  virtual ~sbRemotePlayer();

  static already_AddRefed<nsPIDOMWindow> GetWindowFromJS();

  // Helper Methods
  nsresult Init();
  nsresult AcquirePlaylistWidget();
  nsresult RegisterCommands( PRBool aUseDefaultCommands );
  nsresult UnregisterCommands();
  nsresult ConfirmPlaybackControl();
  nsresult GetBrowser( nsIDOMElement** aElement );
  nsresult TakePlaybackControl( nsIURI* aURI );

  // Data members
  PRBool mInitialized;
  PRBool mUseDefaultCommands;
  nsCOMPtr<sbIPlaylistPlayback> mGPPS;

  // The documents for the web page and for the tabbrowser
  nsCOMPtr<nsIDOMDocument> mContentDoc;
  nsCOMPtr<nsIDOMDocument> mChromeDoc;

  // the remote impl for the playlist binding
  nsRefPtr<sbIPlaylistWidget> mWebPlaylistWidget;

  // Like the site libraries, this may want to be a collection
  // The commands registered by the page.
  nsRefPtr<sbRemoteCommands> mCommandsObject;

  // Hashtable to hold the observers registered by the webpage
  nsDataHashtable<nsStringHashKey, sbRemoteObserver> mRemObsHash;

  // Hashtable to hold all the libraries that we generate
  nsInterfaceHashtable<nsStringHashKey, sbIRemoteLibrary> mCachedLibraries;

  // stash these for quick reference
  nsCOMPtr<sbIDataRemote> mdrCurrentArtist;
  nsCOMPtr<sbIDataRemote> mdrCurrentAlbum;
  nsCOMPtr<sbIDataRemote> mdrCurrentTrack;
  nsCOMPtr<sbIDataRemote> mdrPlaying;
  nsCOMPtr<sbIDataRemote> mdrPaused;
  nsCOMPtr<sbIDataRemote> mdrRepeat;
  nsCOMPtr<sbIDataRemote> mdrShuffle;
  nsCOMPtr<sbIDataRemote> mdrPosition;
  nsCOMPtr<sbIDataRemote> mdrVolume;
  nsCOMPtr<sbIDataRemote> mdrMute;

  // SecurityCheckedComponent vars
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsRefPtr<sbRemoteNotificationManager> mNotificationMgr;
};

#endif // __SB_REMOTE_PLAYER_H__
