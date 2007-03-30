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

#ifndef __SBLOCALDATABASECASCADEFILTERSET_H__
#define __SBLOCALDATABASECASCADEFILTERSET_H__

#include <nsCOMPtr.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <sbICascadeFilterSet.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbIMediaList.h>

class sbLocalDatabaseCascadeFilterSet : public sbICascadeFilterSet
{
  typedef nsTArray<nsString> sbStringArray;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICASCADEFILTERSET

  nsresult Init(sbIMediaList* aMediaList,
                sbILocalDatabaseGUIDArray* aProtoArray);

private:
  nsresult ConfigureArray(PRUint32 aIndex);

  struct sbFilterSpec {
    PRBool isSearch;
    nsString property;
    sbStringArray propertyList;
    sbStringArray values;
    nsCOMPtr<sbILocalDatabaseGUIDArray> array;
  };

  nsCOMPtr<sbIMediaList> mMediaList;

  nsCOMPtr<sbILocalDatabaseGUIDArray> mProtoArray;

  nsTArray<sbFilterSpec> mFilters;
};


class sbGUIDArrayPrimraySortEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbGUIDArrayPrimraySortEnumerator(sbILocalDatabaseGUIDArray* aArray);

private:
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;
  PRUint32 mNextIndex;
};

#endif /* __SBLOCALDATABASECASCADEFILTERSET_H__ */

