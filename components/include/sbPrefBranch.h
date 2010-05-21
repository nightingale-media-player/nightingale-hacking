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
#include <nsComponentManagerUtils.h>
#include <sbProxyUtils.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsIVariant.h>

/**
 * Helper class for preferences to make the syntax a bit easier on the eyes
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

    // special bonus macro for passing our errors outwards
#define __ENSURE_SUCCESS(rv) \
    PR_BEGIN_MACRO \
    if (NS_FAILED(rv)) { \
      *aResult = rv; \
      NS_ENSURE_SUCCESS(rv, /*void */); \
    } \
    PR_END_MACRO

    nsresult rv;

    // get the prefs service
    nsCOMPtr<nsIPrefService> prefService;
    prefService = do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
    __ENSURE_SUCCESS(rv);

    // If we're not on the main thread proxy the service
    PRBool const isMainThread = NS_IsMainThread();
    if (!isMainThread) {
      nsCOMPtr<nsIPrefService> proxy;
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(nsIPrefService),
                                prefService,
                                nsIProxyObjectManager::INVOKE_SYNC,
                                getter_AddRefs(proxy));
      __ENSURE_SUCCESS(rv);
      prefService.swap(proxy);
    }

    if (aRoot) {
      rv = prefService->GetBranch(aRoot, getter_AddRefs(mPrefBranch));
      __ENSURE_SUCCESS(rv);
    } else {
      mPrefBranch = do_QueryInterface(prefService, &rv);
      __ENSURE_SUCCESS(rv);
    }

    // If we're not on the main thread, and we're not using the root
    // then we need a proxy to the prefBranch too
    if (!isMainThread && aRoot) {
      nsCOMPtr<nsIPrefBranch> proxy;
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(nsIPrefBranch),
                                mPrefBranch,
                                nsIProxyObjectManager::INVOKE_SYNC,
                                getter_AddRefs(proxy));
      __ENSURE_SUCCESS(rv);
      mPrefBranch.swap(proxy);
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

  /* Get a preference as an nsIVariant */
  nsresult GetPreference(const nsAString & aPrefName, nsIVariant **_retval)
  {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread,
                 "sbPrefBranch used on multiple threads");
    NS_ENSURE_STATE(mPrefBranch);
    NS_ENSURE_ARG_POINTER(_retval);
    NS_ENSURE_FALSE(aPrefName.IsEmpty(), NS_ERROR_INVALID_ARG);
    nsresult rv;

    NS_LossyConvertUTF16toASCII prefNameASCII(aPrefName);

    // get tht type of the pref
    PRInt32 prefType;
    rv = mPrefBranch->GetPrefType(prefNameASCII.get(), &prefType);
    NS_ENSURE_SUCCESS(rv, rv);

    // create a writable variant
    nsCOMPtr<nsIWritableVariant> writableVariant =
      do_CreateInstance("@songbirdnest.com/Songbird/Variant;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // get the value of our pref
    switch (prefType) {
      case nsIPrefBranch::PREF_INVALID: {
        rv = writableVariant->SetAsEmpty();
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
      case nsIPrefBranch::PREF_STRING: {
        char* _value = NULL;
        rv = mPrefBranch->GetCharPref(prefNameASCII.get(), &_value);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCString value;
        value.Adopt(_value);

        // set the value of the variant to the value of the pref
        rv = writableVariant->SetAsACString(value);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
      case nsIPrefBranch::PREF_INT: {
        PRInt32 value;
        rv = mPrefBranch->GetIntPref(prefNameASCII.get(), &value);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = writableVariant->SetAsInt32(value);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
      case nsIPrefBranch::PREF_BOOL: {
        PRBool value;
        rv = mPrefBranch->GetBoolPref(prefNameASCII.get(), &value);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = writableVariant->SetAsBool(value);
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
      default: {
        NS_NOTREACHED("Impossible pref type");
        // And for release builds, do something sane.
        rv = writableVariant->SetAsEmpty();
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
    }
    return CallQueryInterface(writableVariant, _retval);
  }

private:
  nsCOMPtr<nsIPrefBranch> mPrefBranch;
  PRThread * mCreatingThread;
};

#endif /*SBPREFBRANCH_H_*/
