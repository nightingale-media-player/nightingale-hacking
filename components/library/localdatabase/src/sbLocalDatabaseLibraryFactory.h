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

#ifndef __SBLOCALDATABASELIBRARYFACTORY_H__
#define __SBLOCALDATABASELIBRARYFACTORY_H__

#include <sbILibraryFactory.h>
#include <sbILocalDatabaseLibraryFactory.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class nsIFile;
class nsILocalFile;

class sbLocalDatabaseLibraryFactory : public sbILocalDatabaseLibraryFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYFACTORY
  NS_DECL_SBILOCALDATABASELIBRARYFACTORY

  already_AddRefed<nsILocalFile> GetFileForGUID(const nsAString& aGUID);

private:
  nsresult InitalizeLibrary(nsIFile* aDatabaseFile);
};

#endif /* __SBLOCALDATABASELIBRARYFACTORY_H__ */

