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

#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsIObserver.h>

#include <nsHashKeys.h>
#include <nsWeakReference.h>
#include <prmon.h>

#include <sbIMediacoreFactoryRegistrar.h>
#include <sbIMediacoreVoting.h>

class sbMediacoreManager : public sbIMediacoreManager,
                           public sbIMediacoreFactoryRegistrar,
                           public sbIMediacoreVoting,
                           public nsIClassInfo,
                           public nsIObserver,
                           public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREMANAGER
  NS_DECL_SBIMEDIACOREFACTORYREGISTRAR
  NS_DECL_SBIMEDIACOREVOTING
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER

  sbMediacoreManager();

  nsresult Init();

private:
  virtual ~sbMediacoreManager();

protected:
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

  nsresult GenerateInstanceName(nsAString &aInstanceName);

  nsresult VoteWithURIOrChannel(nsIURI *aURI, 
                                nsIChannel *aChannel, 
                                sbIMediacoreVotingChain **_retval);

  PRMonitor* mMonitor;
  PRUint32   mLastCore;

  nsInterfaceHashtableMT<nsStringHashKey, sbIMediacore> mCores;
  nsInterfaceHashtableMT<nsISupportsHashKey, sbIMediacoreFactory> mFactories;

  nsCOMPtr<sbIMediacore>          mPrimaryCore;
  nsCOMPtr<sbIMediacoreSequencer> mSequencer;
};
