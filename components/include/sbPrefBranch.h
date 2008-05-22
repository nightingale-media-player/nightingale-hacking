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

#ifndef SBPREFBRANCH_H_
#define SBPREFBRANCH_H_

#include <nsCOMPtr.h>
#include <nsThreadUtils.h>
#include <sbProxyUtils.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>

/**
 * Helper class for string bundles to make the syntax a bit easier on the eyes
 * This can be used on non-main threads but each instance must only be used
 * by the thread that created it.
 */
class sbPrefBranch
{
public:
  /**
   * Initialize the pref branch with the specified branch. 
   * A null @aRoot means the root.
   */
  sbPrefBranch(const char* aRoot, nsresult* aResult) : mCreatingThread(PR_GetCurrentThread()) {
    NS_ASSERTION(aResult != nsnull, "null nsresult* passed into sbPrefBranch constructor");
    *aResult = NS_OK;

    // special bonus macro for passing our 
#define __ENSURE_SUCCESS(rv) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
      *aResult = rv; \
      NS_ENSURE_SUCCESS(rv, /*void */); \
    } \
    PR_END_MACRO

    nsresult rv;

    // get the prefs service
    mPrefService = 
      do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    __ENSURE_SUCCESS(rv);

    // If we're not on the main thread proxy the service
    PRBool const isMainThread = NS_IsMainThread();
    if (!isMainThread) {
      nsCOMPtr<nsIPrefService> proxy;
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(nsIPrefService),
                                mPrefService,
                                nsIProxyObjectManager::INVOKE_SYNC |
                                nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                getter_AddRefs(proxy));
      __ENSURE_SUCCESS(rv);
      mPrefService.swap(proxy);
    }

    if (aRoot) {
      rv = mPrefService->GetBranch(aRoot, getter_AddRefs(mPrefBranch));
      __ENSURE_SUCCESS(rv);
    } else {
      mPrefBranch = do_QueryInterface(mPrefService, &rv);
      __ENSURE_SUCCESS(rv);
    }
#undef __ENSURE_SUCCESS
  }

  PRBool GetBoolPref(const char* aKey, PRBool aDefault) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbPrefBranch used on multiple threads");
    NS_ASSERTION(mPrefBranch, "mPrefBranch is null in sbPrefBranch");

    PRBool result;
    nsresult rv = mPrefBranch->GetBoolPref(aKey, &result);
    if (NS_FAILED(rv)) {
      return aDefault;
    }
    return result;
  }

  nsresult SetBoolPref(const char* aKey, PRBool aValue) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbPrefBranch used on multiple threads");
    NS_ASSERTION(mPrefBranch, "mPrefBranch is null in sbPrefBranch");

    return mPrefBranch->SetBoolPref(aKey, aValue);
  }

  PRInt32 GetIntPref(const char* aKey, const PRInt32 aDefault) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbPrefBranch used on multiple threads");
    NS_ASSERTION(mPrefBranch, "mPrefBranch is null in sbPrefBranch");

    PRInt32 result;
    nsresult rv = mPrefBranch->GetIntPref(aKey, &result);
    if (NS_FAILED(rv)) {
      return aDefault;
    }
    return result;
  }

  nsresult SetIntPref(const char* aKey, const PRInt32 aValue) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbPrefBranch used on multiple threads");
    NS_ASSERTION(mPrefBranch, "mPrefBranch is null in sbPrefBranch");

    return mPrefBranch->SetIntPref(aKey, aValue);
  }


  nsCString GetCharPref(const char* aKey, const nsCString& aDefault) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbPrefBranch used on multiple threads");
    NS_ASSERTION(mPrefBranch, "mPrefBranch is null in sbPrefBranch");

    char* result;
    nsresult rv = mPrefBranch->GetCharPref(aKey, &result);
    if (NS_FAILED(rv) || result == nsnull) {
      return aDefault;
    }
    nsCString res;
    res.Adopt(result);
    return res;
  }

  nsresult SetCharPref(const char* aKey, const nsCString& aValue) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbPrefBranch used on multiple threads");
    NS_ASSERTION(mPrefBranch, "mPrefBranch is null in sbPrefBranch");

    return mPrefBranch->SetCharPref(aKey, aValue.get());
  }

private:
  nsCOMPtr<nsIPrefService> mPrefService;
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  PRThread * mCreatingThread;
};

#endif /*SBPREFBRANCH_H_*/
