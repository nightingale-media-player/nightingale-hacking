/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbCDDeviceMarshall_h_
#define sbCDDeviceMarshall_h_

#include <sbBaseDeviceMarshall.h>
#include <sbIDeviceRegistrar.h>
#include <sbICDDeviceService.h>
#include <nsIClassInfo.h>
#include <nsStringAPI.h>
#include <nsIWritablePropertyBag.h>


class sbCDDeviceMarshall : public sbBaseDeviceMarshall,
                           public sbICDDeviceListener,
                           public nsIClassInfo
{
public:
  sbCDDeviceMarshall();
  virtual ~sbCDDeviceMarshall();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIDEVICEMARSHALL
  NS_DECL_SBICDDEVICELISTENER

  nsresult Init();

protected:
  nsresult AddCDDevice(const nsAString & aMarshallDeviceID,
                       nsIWritablePropertyBag *aParams);

  nsresult CreateAndDispatchDeviceManagerEvent(PRUint32 aType,
                                               nsIVariant *aData = nsnull,
                                               nsISupports *aOrigin = nsnull,
                                               PRBool aAsync = PR_FALSE);

private:
  nsCID    mMarshallCID;
  nsString mMarshallClassName;
};

#endif  // sbCDDeviceMarshall_h_

