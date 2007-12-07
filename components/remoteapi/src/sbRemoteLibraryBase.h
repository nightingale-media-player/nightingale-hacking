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

#ifndef __SB_REMOTE_LIBRARYBASE_H__
#define __SB_REMOTE_LIBRARYBASE_H__

#include "sbRemoteAPI.h"
#include "sbRemoteMediaItem.h"
#include "sbRemoteMediaList.h"
#include "sbRemoteSiteMediaList.h"
#include "sbRemoteSiteMediaItem.h"
#include "sbXPCScriptableStub.h"

#include <nsIFile.h>
#include <nsISecurityCheckedComponent.h>
#include <nsIXPCScriptable.h>
#include <sbILibrary.h>
#include <sbILibraryResource.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>
#include <sbIMetadataJob.h>
#include <sbIRemoteLibrary.h>
#include <sbIRemoteMediaList.h>
#include <sbIScriptableFilterResult.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIWrappedMediaList.h>

#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#ifdef PR_LOGGING
extern PRLogModuleInfo *gRemoteLibraryLog;
#endif

#define LOG_LIB(args) PR_LOG(gRemoteLibraryLog, PR_LOG_WARN, args)
#define TRACE_LIB(args) PR_LOG(gRemoteLibraryLog, PR_LOG_DEBUG, args)

class nsIStringEnumerator;
class sbIMediaItem;
class sbRemotePlayer;

// PURE VIRTUAL: Inherits but doesn't impl nsIClassInfo
//               derived classes must impl.
class sbRemoteLibraryBase : public nsIClassInfo,
                            public nsISecurityCheckedComponent,
                            public sbIScriptableFilterResult,
                            public sbISecurityAggregator,
                            public sbIRemoteLibrary,
                            public sbIRemoteMediaList,
                            public sbIWrappedMediaList,
                            public sbIMediaList,
                            public sbIMediaListEnumerationListener,
                            public sbXPCScriptableStub
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISCRIPTABLEFILTERRESULT
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTELIBRARY
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  NS_FORWARD_SAFE_SBIREMOTEMEDIALIST(mRemMediaList)
  NS_FORWARD_SAFE_SBIMEDIALIST(mRemMediaList)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mRemMediaList)
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mRemMediaList)
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  sbRemoteLibraryBase(sbRemotePlayer* aRemotePlayer);
  
  // nsIXPCScriptable
  NS_IMETHOD GetClassName(char * *aClassName);
  NS_IMETHOD GetScriptableFlags(PRUint32 *aScriptableFlags);
  NS_IMETHOD GetProperty(nsIXPConnectWrappedNative *wrapper, JSContext * cx, JSObject * obj, jsval id, jsval * vp, PRBool *_retval);

  // sbIWrappedMediaList
  virtual already_AddRefed<sbIMediaItem> GetMediaItem();
  virtual already_AddRefed<sbIMediaList> GetMediaList();

  // Gets the GUID for the built in libraries: main, web, download
  static nsresult GetLibraryGUID( const nsAString &aLibraryID,
                                  nsAString &aLibraryGUID );
protected:
  virtual ~sbRemoteLibraryBase();

  // PURE VIRTUAL: Needs to be implemented by derived class
  // on connection to a library, set the internal remote medialist
  virtual nsresult InitInternalMediaList() = 0;

  already_AddRefed<sbIRemoteMediaList>
    GetMediaListBySiteID(const nsAString& aSiteID);

  already_AddRefed<sbIMediaItem>
    FindMediaItemWithMatchingScope(const nsCOMArray<sbIMediaItem>& aMediaItems);

  PRBool mShouldScan;
  nsCOMPtr<sbILibrary> mLibrary;
  nsRefPtr<sbRemoteMediaListBase> mRemMediaList;

  // For holding the results of sbIMediaList enumeration methods..
  nsCOMArray<sbIMediaItem> mEnumerationArray;
  nsresult mEnumerationResult;

  // For holding onto the metadatajob so we can reuse it.
  nsCOMPtr<sbIMetadataJob> mMetadataJob;

  // SecurityCheckedComponent stuff
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;

  nsRefPtr<sbRemotePlayer> mRemotePlayer;
};

#endif // __SB_REMOTE_LIBRARYBASE_H__
