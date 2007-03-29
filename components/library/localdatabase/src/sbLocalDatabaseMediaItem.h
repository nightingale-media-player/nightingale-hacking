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

#ifndef __SBLOCALDATABASEMEDIAITEM_H__
#define __SBLOCALDATABASEMEDIAITEM_H__

#include <sbIMediaItem.h>

#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsStringGlue.h>
#include <nsWeakReference.h>
#include <sbILibraryResource.h>
#include <sbILocalDatabaseMediaItem.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbILocalDatabaseResourceProperty.h>
#include "sbLocalDatabaseResourceProperty.h"
#include <sbIMediaItem.h>

class sbILocalDatabaseLibrary;

class sbLocalDatabaseMediaItem : public sbLocalDatabaseResourceProperty,
                                 public sbIMediaItem,
                                 public sbILocalDatabaseMediaItem,
                                 public nsIClassInfo,
                                 public nsSupportsWeakReference
                                 
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  // When using inheritance, you must forward all interfaces implemented
  // by the base class, else you will get "pure virtual function was not
  // defined"style errors.
  NS_FORWARD_SBILOCALDATABASERESOURCEPROPERTY(sbLocalDatabaseResourceProperty::)
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseResourceProperty::)
  
  NS_DECL_SBIMEDIAITEM
  NS_DECL_SBILOCALDATABASEMEDIAITEM
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaItem(sbILocalDatabaseLibrary* aLibrary,
                           const nsAString& aGuid)
  : sbLocalDatabaseResourceProperty(aLibrary, aGuid),
    mMediaItemId(0),
    mLibrary(aLibrary)
  {
  }

protected:

  PRUint32 mMediaItemId;

  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;
};

#endif /* __SBLOCALDATABASEMEDIAITEM_H__ */

