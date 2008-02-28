/* vim: set sw=2 :miv */
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

#include "sbBaseDevice.h"

class sbMockDevice : public sbBaseDevice
{
public:
  sbMockDevice();
  NS_DECL_ISUPPORTS

  /* sbIDevice excluding GetState */
  NS_IMETHOD GetName(nsAString & aName);
  NS_IMETHOD GetControllerId(nsID * *aControllerId);
  NS_IMETHOD GetId(nsID * *aId);
  NS_IMETHOD Connect(void);
  NS_IMETHOD Disconnect(void);
  NS_IMETHOD GetConnected(PRBool *aConnected);
  NS_IMETHOD GetThreaded(PRBool *aThreaded);
  NS_IMETHOD GetPreference(const nsAString & aPrefName, nsIVariant **_retval);
  NS_IMETHOD SetPreference(const nsAString & aPrefName, nsIVariant *aPrefValue);
  NS_IMETHOD GetCapabilities(sbIDeviceCapabilities * *aCapabilities);
  NS_IMETHOD GetContent(sbIDeviceContent * *aContent);
  NS_IMETHOD GetParameters(nsIPropertyBag2 * *aParameters);
  NS_IMETHOD GetProperties(sbIDeviceProperties * *theProperties);
  NS_IMETHOD SubmitRequest(PRUint32 aRequest, nsIPropertyBag2 *aRequestParameters);
  NS_IMETHOD CancelRequests();

public:
  nsresult ProcessRequest();

protected:
  PRBool mIsConnected;
  
private:
  ~sbMockDevice();
};
