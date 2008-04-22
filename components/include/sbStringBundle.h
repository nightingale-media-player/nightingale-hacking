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

#ifndef SBSTRINGBUNDLE_H_
#define SBSTRINGBUNDLE_H_

#include <nsCOMPtr.h>
#include <nsThreadUtils.h>
#include <sbProxyUtils.h>
#include <nsIStringBundle.h>

/**
 * Helper class for string bundles to make the syntax a bit easier on the eyes
 * This can be used on non-main threads but each instance must only be used
 * by the thread that created it.
 */
class sbStringBundle
{
public:
  /**
   * Initializes the string bundle from the URI
   */
  sbStringBundle(char const * aURI) : mCreatingThread(PR_GetCurrentThread()) {
    NS_ASSERTION(aURI, "sbStringBundle must have a non null aURI");
    /* Get the string bundle service */
    nsCOMPtr<nsIStringBundleService> service = 
      do_GetService(NS_STRINGBUNDLE_CONTRACTID,
                    &mRV);
    if (NS_FAILED(mRV))
      return;
    // If we're not on the main thread proxy the service
    PRBool const isMainThread = NS_IsMainThread();
    if (!isMainThread) {
      nsCOMPtr<nsIStringBundleService> proxy;
      mRV = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                 NS_GET_IID(nsIStringBundleService),
                                 service,
                                 nsIProxyObjectManager::INVOKE_SYNC |
                                 nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                 getter_AddRefs(proxy));
      if (NS_FAILED(mRV))
        return;
      service.swap(proxy);
    }
    // Get the string bundle from the service
    mRV = service->CreateBundle(aURI, getter_AddRefs(mBundle));
    if (NS_FAILED(mRV))
      return;
    
    // If this isn't the main thread then proxy the bundle
    if (!isMainThread) {
      nsCOMPtr<nsIStringBundle> temp;
      mRV = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                 NS_GET_IID(nsIStringBundle),
                                 mBundle,
                                 nsIProxyObjectManager::INVOKE_SYNC |
                                 nsIProxyObjectManager::FORCE_PROXY_CREATION,
                                 getter_AddRefs(temp));
      mBundle.swap(temp);
    }
  }
  /**
   * Retrieves a localized string given a name
   * aName char const * was chosen to avoid callers from having to use NS_LITERAL_STRING
   */
  nsString Get(char const * aName) {
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbStringBundle used on multiple threads");
    NS_ASSERTION(mBundle, "mBundle is null in sbStringBundle");

    nsString result;
    PRUnichar * buffer;
    mRV = mBundle->GetStringFromName(NS_ConvertASCIItoUTF16(aName).BeginReading(), 
                                     &buffer);
    if (NS_SUCCEEDED(mRV)) {
      result.Adopt(buffer);
    }
    else {
      NS_WARNING("Name not found in string bundle");
    }
    return result;
  }
  /**
   * Retrieves the localizes string with parameter substitution
   * aName char const * was chosen to avoid callers from having to use NS_LITERAL_STRING
   */
  nsString Format(char const * aName,
                  PRUnichar const ** aParams,
                  int aParamCount) {
    
   return Format(NS_ConvertASCIItoUTF16(aName),
                 aParams,
                 aParamCount);
  }
  /**
   * Retrieves the localizes string with parameter substitution
   * aName char const * was chosen to avoid callers from having to use NS_LITERAL_STRING
   */
  nsString Format(nsAString const & aName,
                  PRUnichar const ** aParams,
                  int aParamCount) {
    
    NS_ASSERTION(PR_GetCurrentThread() == mCreatingThread, "sbStringBundle used on multiple threads");
    NS_ASSERTION(mBundle, "mBundle is null in sbStringBundle");

    nsString result;
    PRUnichar * buffer;
    mRV = mBundle->FormatStringFromName(aName.BeginReading(), 
                                        aParams,
                                        aParamCount,
                                        &buffer);
    if (NS_SUCCEEDED(mRV)) {
      result.Adopt(buffer);
    }
    else {
      NS_WARNING("Name not found in string bundle");
    }
    return result;
  }
  
  /**
   * Returns the result of the last operation
   */
  nsresult Result() const {
    return mRV;
  }
private:
  nsCOMPtr<nsIStringBundle> mBundle;
  PRThread * mCreatingThread;
  nsresult mRV;
};

#endif /*SBSTRINGBUNDLE_H_*/
