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

#include <nsIFile.h>
#include <nsISecurityCheckedComponent.h>
#include <sbILibrary.h>
#include <sbILibraryResource.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>
#include <sbIRemoteLibrary.h>
#include <sbIRemoteMediaList.h>
#include <sbIRemotePlayer.h>
#include <sbISecurityMixin.h>
#include <sbISecurityAggregator.h>
#include <sbIWrappedMediaList.h>

#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class nsIStringEnumerator;
class sbIMediaItem;

// PURE VIRTUAL: Inherits but doesn't impl nsIClassInfo
//               derived classes must impl.
class sbRemoteLibraryBase : public nsIClassInfo,
                            public nsISecurityCheckedComponent,
                            public sbISecurityAggregator,
                            public sbIRemoteLibrary,
                            public sbIRemoteMediaList,
                            public sbIWrappedMediaList,
                            public sbIMediaList,
                            public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISECURITYAGGREGATOR
  NS_DECL_SBIREMOTELIBRARY
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  NS_FORWARD_SAFE_SBIREMOTEMEDIALIST(mRemMediaList)
  NS_FORWARD_SAFE_SBIMEDIALIST(mRemMediaList)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mRemMediaList)
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mRemMediaList)
  NS_FORWARD_SAFE_NSISECURITYCHECKEDCOMPONENT(mSecurityMixin)

  sbRemoteLibraryBase();

  // sbIWrappedMediaList
  NS_IMETHOD_(already_AddRefed<sbIMediaItem>) GetMediaItem();
  NS_IMETHOD_(already_AddRefed<sbIMediaList>) GetMediaList();

  // Gets the GUID for the built in libraries: main, web, download
  static nsresult GetLibraryGUID( const nsAString &aLibraryID,
                                  nsAString &aLibraryGUID );
protected:
  virtual ~sbRemoteLibraryBase();

  // PURE VIRTUAL: Needs to be implemented by derived class
  // on connection to a library, set the internal remote medialist
  virtual nsresult InitInternalMediaList() = 0;

  nsresult GetListEnumForProperty( const nsAString& aProperty,
                                   nsIStringEnumerator** _retval );

  PRBool mShouldScan;
  nsCOMPtr<sbILibrary> mLibrary;
  nsRefPtr<sbRemoteMediaListBase> mRemMediaList;

  // For holding the results of sbIMediaList enumeration methods..
  nsCOMArray<sbIMediaItem> mEnumerationArray;
  nsresult mEnumerationResult;

  // SecurityCheckedComponent stuff
  nsCOMPtr<nsISecurityCheckedComponent> mSecurityMixin;
};

#endif // __SB_REMOTE_LIBRARYBASE_H__
