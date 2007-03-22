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

#ifndef __SBLOCALDATABASELIBRARY_H__
#define __SBLOCALDATABASELIBRARY_H__

#include <sbLocalDatabaseResourceProperty.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>

#include <nsCOMPtr.h>
#include <nsIURI.h>
#include <nsStringGlue.h>
#include <sbILocalDatabasePropertyCache.h>

class sbIDatabaseQuery;

class sbLocalDatabaseLibrary : public sbLocalDatabaseResourceProperty,
                               public sbILibrary,
                               public sbILocalDatabaseLibrary
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  // When using inheritence, you must forward all interfaces implemented
  // by the base class, else you will get "pure virtual function was not
  // defined" style errors.
  NS_FORWARD_SBILOCALDATABASERESOURCEPROPERTY(sbLocalDatabaseResourceProperty::)
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseResourceProperty::)

  NS_DECL_SBILIBRARY
  NS_DECL_SBILOCALDATABASELIBRARY

  // This constructor assumes the database file lives in the 'ProfD/db'
  // directory.
  sbLocalDatabaseLibrary(const nsAString& aDatabaseGuid)
  : mDatabaseGuid(aDatabaseGuid)
  {
    NS_ASSERTION(!mDatabaseGuid.IsEmpty(), "No GUID!");
  }

  // Use this constructor to specify a location for the database file.
  sbLocalDatabaseLibrary(nsIURI* aDatabaseLocation,
                         const nsAString& aDatabaseGuid)
  : mDatabaseLocation(aDatabaseLocation),
    mDatabaseGuid(aDatabaseGuid)
  {
    NS_ASSERTION(mDatabaseLocation, "Null pointer!");
    NS_ASSERTION(!mDatabaseGuid.IsEmpty(), "No GUID!");
  }

private:
  nsresult CreateQueries();

  inline NS_METHOD MakeStandardQuery(sbIDatabaseQuery** _retval);

  nsString mDatabaseGuid;
  nsCOMPtr<nsIURI> mDatabaseLocation;

  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  nsString mGetContractIdForGuidQuery;
  nsString mGetMediaItemIdForGuidQuery;
  nsString mInsertMediaItemQuery;

  // This library's resource guid
  nsString mGuid;
};

#endif /* __SBLOCALDATABASELIBRARY_H__ */
