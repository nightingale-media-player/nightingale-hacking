/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef sbWatchFolderPrefMgr_h_
#define sbWatchFolderPrefMgr_h_

#include <nsIObserver.h>
#include <nsIPrefBranch2.h>
#include <nsAutoPtr.h>

class sbWatchFolderService;


//------------------------------------------------------------------------------
//
// @brief Utility class for abstracting all pref listening and delegation out
//        of the watch folder service.
//
//------------------------------------------------------------------------------
class sbWatchFolderPrefMgr : public nsIObserver
{
public:
  explicit sbWatchFolderPrefMgr();
  virtual ~sbWatchFolderPrefMgr();

  nsresult Init(sbWatchFolderService *aWFService);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsresult GetIsUnitTestsRunning(PRBool *aOutIsRunning);

protected:
  nsresult OnPrefChanged(const nsAString & aPrefName,
                         nsIPrefBranch2 *aPrefBranch);

private:
  nsRefPtr<sbWatchFolderService> mWatchFolderService;
};

#endif  // sbWatchFolderPrefMgr_h_

