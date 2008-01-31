#ifndef SBBASEDEVICEMARSHALL_H_
#define SBBASEDEVICEMARSHALL_H_

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

#include <sbIDeviceMarshall.h>
#include <nsCOMPtr.h>
#include <sbIDeviceCompatibility.h>
#include <nsID.h>
#include <nsIArray.h>

class nsISimpleEnumerator;
class sbIDeviceControllerRegistrar;
class nsIPropertyBag;
class sbIDeviceController;

/**
 * This class provides a set of common tools for use by device marshall
 * implementations.
 * Currently this class provides three main purposes.
 * - Monitoring shutdown via the ClearMonitoringFlag this ensure proper
 *   shutdown.
 * - Controller instantiation. It will instantiate all the controllers
 *   for a marshaller and return them via the GetControllers or will
 *   directly register them with RegisterControllers
 * - Compatibility search via the FindCompatibleControllers methods.
 */
class sbBaseDeviceMarshall : public sbIDeviceMarshall
{
public:
  /**
   * Class to compare controllers. This allows 
   */
  class CompatibilityComparer
  {
  public:
    /**
     * Initializes the comparer's values
     */
    CompatibilityComparer() :
      mBestMatch(nsnull)
    {
    }
    /**
     * Determines if the controller is the best.
     * Returns true  
     */
    virtual PRBool Compare(sbIDeviceController* controller,
                         nsIPropertyBag* deviceParams);
    /**
     * Returns the controller that thus far is the best match
     */
    sbIDeviceController *GetBestMatch() const
    {
      return mBestMatch;
    }
  protected:
    /**
     * Sets the device controller as the best match
     */
    void SetBestMatch(sbIDeviceController* deviceController, sbIDeviceCompatibility * compatibility)
    {
      mBestMatch = deviceController;
      mCompatibility = compatibility;
    }
  private:
    /**
     * We don't own this, and we'll never outlive the owner
     */
    sbIDeviceController* mBestMatch;
    nsCOMPtr<sbIDeviceCompatibility> mCompatibility;
  };
  /**
   * Locates the most compatible controller based on the ControllerComparer.
   */
  sbIDeviceController* FindCompatibleControllers(nsIPropertyBag* deviceParams)
  {
    CompatibilityComparer comparer;
    return FindCompatibleControllers(deviceParams, comparer);
  }
  /**
   * Locates the most compatible controller based on deviceComparer.
   */
  sbIDeviceController* FindCompatibleControllers(nsIPropertyBag* deviceParams,
                                                 CompatibilityComparer & deviceComparer);
protected:
  /**
   * Verifies that shutdown has been called.
   */
  virtual ~sbBaseDeviceMarshall();
  /**
   * Initializes the shutdown flag.
   */
  sbBaseDeviceMarshall(nsACString const & categoryName);
  void RegisterControllers(sbIDeviceControllerRegistrar * registrar);
  /**
   * Retrieves the controllers for the category and loads them into the
   * controllers array
   */
  nsIArray* GetControllers() const
  {
    if (!mControllers) {
      // Logically const
      if (!const_cast<sbBaseDeviceMarshall*>(this)->RefreshControllers()) {
        NS_ASSERTION(PR_FALSE, "Something bad happened while refreshing the controllers");
        return nsnull;
      }
    }
    return mControllers.get();
  }
  /**
   * Refreshes the list of controllers returning an array interface to them
   */
  nsIArray* RefreshControllers();
  /**
   * Derived classes call this at the start of their stopMonitoring implementation.
   * An assert will fire on destruction if this is not called. Monitoring would
   * normally be turned off during shutdown.
   */
  void ClearMonitoringFlag()
  {
    mIsMonitoring = PR_FALSE;
  }
  /**
   * Derived classes can use this to determine whether or not monitoring is active
   */
  PRBool IsMonitoring() const
  {
    return mIsMonitoring;
  }
private:
  nsCOMPtr<nsIArray> mControllers;
  nsCString mCategoryName;
  PRBool mIsMonitoring;
  /**
   * Retrieves the enumerator for our category with the name mCategoryName
   */
  nsresult GetCategoryManagerEnumerator(nsCOMPtr<nsISimpleEnumerator> & enumerator);
};

#endif
