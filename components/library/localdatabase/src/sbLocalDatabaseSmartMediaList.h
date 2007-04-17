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

#ifndef __SBLOCALDATABASESMARTMEDIALIST_H__
#define __SBLOCALDATABASESMARTMEDIALIST_H__

#include <sbILocalDatabaseSmartMediaList.h>

#include <nsWeakReference.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prlock.h>

#include <sbIMediaItem.h>
#include <sbIMediaList.h>

class sbLocalDatabaseSmartMediaListCondition : public sbILocalDatabaseSmartMediaListCondition
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASESMARTMEDIALISTCONDITION

  sbLocalDatabaseSmartMediaListCondition();
  ~sbLocalDatabaseSmartMediaListCondition();

protected:

};

class sbLocalDatabaseSmartMediaList : public sbILocalDatabaseSmartMediaList,
                                      public nsSupportsWeakReference
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASESMARTMEDIALIST

  NS_FORWARD_SAFE_SBIMEDIALIST(mList)
  NS_FORWARD_SAFE_SBIMEDIAITEM(mItem)
  NS_FORWARD_SAFE_SBILIBRARYRESOURCE(mItem)

  sbLocalDatabaseSmartMediaList();
  ~sbLocalDatabaseSmartMediaList();

protected:
  PRLock* mInnerLock;

  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<sbIMediaList> mList;

};

#endif /* __SBLOCALDATABASESMARTMEDIALIST_H__ */
