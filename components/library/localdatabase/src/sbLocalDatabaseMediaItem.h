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
#include <sbILibraryResource.h>
#include <sbILibrary.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsIClassInfo.h>

class sbLocalDatabaseMediaItem : public sbIMediaItem,
                                 public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYRESOURCE
  NS_DECL_SBIMEDIAITEM
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaItem(sbILibrary* aLibrary, const nsAString& aGuid);

private:
  ~sbLocalDatabaseMediaItem();

  nsCOMPtr<sbILibrary> mLibrary;
  nsString mGuid;
};

#endif /* __SBLOCALDATABASEMEDIAITEM_H__ */

