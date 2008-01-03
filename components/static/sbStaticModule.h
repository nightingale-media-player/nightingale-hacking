/* vim: set sw=2 : */
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

/** nsIModule implementation for the static components */

#ifndef sbStaticModule_h__
#define sbStaticModule_h__

#include "nsIModule.h"
#include <nsCOMArray.h>

class sbStaticModule : public nsIModule
{
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMODULE
public:
  sbStaticModule();
  nsresult Initialize(nsIComponentManager *aCompMgr,
                      nsIFile* aLocation);
protected:
  nsCOMArray<nsIModule> mModules;
private:
  ~sbStaticModule();
};

#endif /* sbStaticModule_h__ */
