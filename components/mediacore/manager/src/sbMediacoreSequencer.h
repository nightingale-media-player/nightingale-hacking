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

#include <sbIMediacoreSequencer.h>

#include <nsIMutableArray.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>
#include <nsIWeakReference.h>

#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsTHashtable.h>
#include <prmon.h>

#include <sbIMediacoreEventListener.h>
#include <sbIMediacoreManager.h>
#include <sbIMediacoreSequenceGenerator.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>

#include <vector>

class sbMediacoreSequencer : public sbIMediacoreSequencer,
                             public sbIMediacoreEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACORESEQUENCER
  NS_DECL_SBIMEDIACOREEVENTLISTENER

  sbMediacoreSequencer();

  typedef std::vector<PRUint32> sequence_t;

  nsresult Init();

  // Sequence management
  nsresult RecalculateSequence();
  //nsresult RecalculatePartialSequence();

  // Fetching of items, item manipulation.
  nsresult GetItem(const sequence_t &aSequence,
                   PRUint32 aPosition, 
                   sbIMediaItem **aItem);

private:
  virtual ~sbMediacoreSequencer();

protected:
  PRMonitor *mMonitor;
  
  PRUint32                       mStatus;
  PRBool                         mIsWaitingForPlayback;
  
  PRUint32                       mChainIndex;
  nsCOMPtr<nsIArray>             mChain;
  
  nsCOMPtr<sbIMediacore>                mCore;
  nsCOMPtr<sbIMediacorePlaybackControl> mPlaybackControl;

  PRUint32                       mMode;
  PRUint32                       mRepeatMode;

  nsCOMPtr<sbIMediaListView>     mView;
  sequence_t                     mSequence;
  PRUint32                       mPosition;

  nsCOMPtr<sbIMediacoreSequenceGenerator> mCustomGenerator;
  nsCOMPtr<sbIMediacoreSequenceGenerator> mShuffleGenerator;

  nsCOMPtr<nsIWeakReference> mMediacoreManager;
};
