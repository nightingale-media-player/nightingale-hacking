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

#ifndef __SBLOCALDATABASERESOURCEPROPERTY_H__
#define __SBLOCALDATABASERESOURCEPROPERTY_H__

#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabaseResourceProperty.h>
#include <sbILocalDatabasePropertyCache.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbLocalDatabaseResourceProperty : public sbILocalDatabaseResourceProperty
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYRESOURCE
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTY

  sbLocalDatabaseResourceProperty();
  sbLocalDatabaseResourceProperty(sbILocalDatabaseLibrary *aLibrary, 
                                  const nsAString& aGuid);

  virtual ~sbLocalDatabaseResourceProperty();

protected:
  PRBool mWriteThrough;
  PRBool mWritePending;

  PRLock* mPropertyCacheLock;
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  PRLock* mPropertyBagLock;
  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> mPropertyBag;

  PRLock*   mGuidLock;
  nsString  mGuid;
};

#endif /* __SBLOCALDATABASERESOURCEPROPERTY_H__ */
