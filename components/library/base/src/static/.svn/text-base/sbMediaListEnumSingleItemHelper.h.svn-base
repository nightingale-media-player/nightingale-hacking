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

#ifndef __SBMEDIALISTENUMSINGLEITEMHELPER_H__
#define __SBMEDIALISTENUMSINGLEITEMHELPER_H__

class sbIMediaItem;
class sbIMediaList;

#include <nsCOMPtr.h>

#include <sbIMediaListListener.h>

/**
 * Helper class to fetch the first item (if any) of a list enumeration
 */
class sbMediaListEnumSingleItemHelper : public sbIMediaListEnumerationListener
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

public:
  static sbMediaListEnumSingleItemHelper* New();

public:
  already_AddRefed<sbIMediaItem> GetItem();
  already_AddRefed<sbIMediaList> GetList();

private:
  sbMediaListEnumSingleItemHelper();
  ~sbMediaListEnumSingleItemHelper();

protected:
  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIMediaList> mList;
};

#endif /* __SBMEDIALISTENUMSINGLEITEMHELPER_H__ */
