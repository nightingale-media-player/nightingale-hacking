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

#include <sbIMediacoreManager.h>

#include <nsAutoPtr.h>
#include <nsIClassInfo.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMXULElement.h>
#include <nsInterfaceHashtable.h>
#include <nsIObserver.h>

#include <nsHashKeys.h>
#include <nsWeakReference.h>
#include <prmon.h>

// Interfaces
#include <sbIDataRemote.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreFactoryRegistrar.h>
#include <sbIMediacoreVideoWindow.h>
#include <sbIMediacoreVoting.h>

#include <sbBaseMediacoreMultibandEqualizer.h>
#include <sbBaseMediacoreVolumeControl.h>

// Forward declared classes
class sbBaseMediacoreEventTarget;

class sbMediacoreManager : public sbBaseMediacoreMultibandEqualizer,
                           public sbBaseMediacoreVolumeControl,
                           public sbIMediacoreManager,
                           public sbPIMediacoreManager,
                           public sbIMediacoreEventTarget,
                           public sbIMediacoreFactoryRegistrar,
                           public sbIMediacoreVideoWindow,
                           public sbIMediacoreVoting,
                           public nsIClassInfo,
                           public nsIObserver,
                           public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREMANAGER
  NS_DECL_SBPIMEDIACOREMANAGER
  NS_DECL_SBIMEDIACOREEVENTTARGET
  NS_DECL_SBIMEDIACOREFACTORYREGISTRAR
  NS_DECL_SBIMEDIACOREVIDEOWINDOW
  NS_DECL_SBIMEDIACOREVOTING
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER

  NS_FORWARD_SBIMEDIACOREVOLUMECONTROL(sbBaseMediacoreVolumeControl::)

  sbMediacoreManager();

  nsresult Init();
  nsresult PreShutdown();
  nsresult Shutdown();

  // sbBaseMediacoreMultibandEqualizer overrides
  virtual nsresult OnInitBaseMediacoreMultibandEqualizer();
  virtual nsresult OnSetEqEnabled(PRBool aEqEnabled);
  virtual nsresult OnGetBandCount(PRUint32 *aBandCount);
  virtual nsresult OnGetBand(PRUint32 aBandIndex, sbIMediacoreEqualizerBand *aBand);
  virtual nsresult OnSetBand(sbIMediacoreEqualizerBand *aBand);

  // sbBaseMediacoreVolumeControl overrides
  virtual nsresult OnInitBaseMediacoreVolumeControl();
  virtual nsresult OnSetMute(PRBool aMute);
  virtual nsresult OnSetVolume(PRFloat64 aVolume);

  nsresult SetVolumeDataRemote(PRFloat64 aVolume);

  nsresult GetAndEnsureEQBandHasDataRemote(PRUint32 aBandIndex,
                                           sbIDataRemote **aRemote);
  nsresult SetAndEnsureEQBandHasDataRemote(sbIMediacoreEqualizerBand *aBand);

  nsresult CreateDataRemoteForEqualizerBand(PRUint32 aBandIndex, 
                                            sbIDataRemote **aRemote);

  nsresult VideoWindowUnloaded();

protected:
  virtual ~sbMediacoreManager();

  // copies the given hash table into the given mutable array
  template<class T>
  static NS_HIDDEN_(PLDHashOperator)
    EnumerateIntoArrayStringKey(const nsAString& aKey,
                                T* aData,
                                void* aArray);
  template<class T>
  static NS_HIDDEN_(PLDHashOperator)
    EnumerateIntoArrayISupportsKey(nsISupports *aKey,
                                   T* aData,
                                   void* aArray);
  template<class T>
  static NS_HIDDEN_(PLDHashOperator)
    EnumerateIntoArrayUint32Key(const PRUint32 &aKey,
                                T* aData,
                                void* aArray);

  nsresult GenerateInstanceName(nsAString &aInstanceName);

  nsresult VoteWithURIOrChannel(nsIURI *aURI,
                                nsIChannel *aChannel,
                                sbIMediacoreVotingChain **_retval);

  PRMonitor* mMonitor;
  PRUint32   mLastCore;

  nsInterfaceHashtableMT<nsStringHashKey, sbIMediacore> mCores;
  nsInterfaceHashtableMT<nsISupportsHashKey, sbIMediacoreFactory> mFactories;

  nsCOMPtr<sbIMediacore>                mPrimaryCore;
  nsCOMPtr<sbIMediacoreSequencer>       mSequencer;
  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;

  nsCOMPtr<sbIDataRemote> mDataRemoteEqualizerEnabled;
  nsInterfaceHashtableMT<nsUint32HashKey, sbIDataRemote> mDataRemoteEqualizerBands;

  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateVolume;
  nsCOMPtr<sbIDataRemote> mDataRemoteFaceplateMute;

  PRPackedBool mFullscreen;
  
  PRMonitor* mVideoWindowMonitor;
  nsCOMPtr<nsIDOMXULElement> mVideoWindow;
  PRUint32 mLastVideoWindow;
};

class sbMediacoreVideoWindowListener : public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER

  sbMediacoreVideoWindowListener();
  virtual ~sbMediacoreVideoWindowListener();

  nsresult Init(sbMediacoreManager *aManager, nsIDOMEventTarget *aTarget);
  PRBool IsWindowReady();

protected:
  PRPackedBool                  mWindowReady;
  nsRefPtr<sbMediacoreManager>  mManager;
  nsCOMPtr<nsIDOMEventTarget>        mTarget;
};
