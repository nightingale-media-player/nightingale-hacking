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

#include "sbMockDeviceFirmwareHandler.h"

#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMNode.h>
#include <nsIDOMNodeList.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>
#include <nsISupportsUtils.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag2.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCIDInternal.h>

#include <sbIDevice.h>
#include <sbIDeviceProperties.h>

#define SB_MOCK_DEVICE_FIRMWARE_URL \
  "http://dingo.songbirdnest.com/~aus/firmware/firmware.xml"
#define SB_MOCK_DEVICE_RESET_URL \
  "http://dingo.songbirdnest.com/~aus/firmware/reset.html"
#define SB_MOCK_DEVICE_RELEASE_NOTES_URL \
  "http://dingo.songbirdnest.com/~aus/firmware/release_notes.html"

NS_IMPL_ISUPPORTS_INHERITED0(sbMockDeviceFirmwareHandler, 
                             sbBaseDeviceFirmwareHandler)

SB_DEVICE_FIRMWARE_HANLDER_REGISTERSELF_IMPL(sbMockDeviceFirmwareHandler,
                                             "Songbird Device Firmware Tester - Mock Device Firmware Handler")

sbMockDeviceFirmwareHandler::sbMockDeviceFirmwareHandler()
{
}

sbMockDeviceFirmwareHandler::~sbMockDeviceFirmwareHandler()
{
}

/*virtual*/ nsresult 
sbMockDeviceFirmwareHandler::OnInit()
{
  mContractId = 
    NS_LITERAL_STRING("@songbirdnest.com/Songbird/Device/Firmware/Handler/MockDevice;1");

  nsCOMPtr<nsIURI> uri;
  nsresult rv = CreateProxiedURI(nsDependentCString(SB_MOCK_DEVICE_RESET_URL),
                                 getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  uri.swap(mResetInstructionsLocation);

  rv = CreateProxiedURI(nsDependentCString(SB_MOCK_DEVICE_RELEASE_NOTES_URL),
                        getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  uri.swap(mReleaseNotesLocation);

  return NS_OK;
}

/*virtual*/ nsresult 
sbMockDeviceFirmwareHandler::OnCanUpdate(sbIDevice *aDevice, 
                                         PRBool *_retval)
{
  *_retval = PR_FALSE;

  nsCOMPtr<sbIDeviceProperties> properties;
  nsresult rv = aDevice->GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString vendorName;
  rv = properties->GetVendorName(vendorName);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: Other firmware handlers will probably want to be a 
  //         little bit more stringent.
  if(!vendorName.EqualsLiteral("ACME Inc.")) {
    return NS_OK;
  }

  // Yep, supported!
  *_retval = PR_TRUE;

  return NS_OK;
}

/*virtual*/ nsresult
sbMockDeviceFirmwareHandler::OnRefreshInfo(sbIDevice *aDevice, 
                                           sbIDeviceEventListener *aListener)
{
  nsresult rv = SendHttpRequest(NS_LITERAL_CSTRING("GET"), 
                                NS_LITERAL_CSTRING(SB_MOCK_DEVICE_FIRMWARE_URL));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetState(HANDLER_REFRESHING_INFO);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: Send firmware refresh info start

  return NS_OK;
}

/*virtual*/ nsresult
sbMockDeviceFirmwareHandler::OnUpdate(sbIDevice *aDevice, 
                                      sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                      sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you will want to actually perform the firmware update
   * on the device. The firmware update object will contain the local 
   * location for the firmware image. It also contains the version of the 
   * firmware image. 
   *
   * The implementation of this method must be asynchronous and not block
   * the main thread. The flow of expected events is as follows:
   * firmware update start, firmware write start, firmware write progress, 
   * firmware write end, firmware verify start, firmware verify progress, 
   * firmware verify end, firmware update end.
   *
   * See sbIDeviceEvent for more infomation about event payload.
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbMockDeviceFirmwareHandler::OnVerifyDevice(sbIDevice *aDevice, 
                                            sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you will want to verify the firmware on the device itself
   * to ensure that it is not corrupt. Whichever method you use will most likely
   * be device specific.
   * 
   * The implementation of this method must be asynchronous and not block
   * the main thread. The flow of expected events is as follows:
   * firmware verify start, firmware verify progress, firmware verify end.
   *
   * If any firmware verify error events are sent during the process
   * the firmware is considered corrupted.
   *
   * See sbIDeviceEvent for more infomation about event payload.
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbMockDeviceFirmwareHandler::OnVerifyUpdate(sbIDevice *aDevice, 
                                            sbIDeviceFirmwareUpdate *aFirmwareUpdate, 
                                            sbIDeviceEventListener *aListener)
{
  /**
   * Here is where you should provide a way to verify the firmware update
   * image itself to make sure that it is not corrupt in any way.
   *
   * The implementation of this method must be asynchronous and not block
   * the main thread. The flow of expected events is as follows:
   * firmware image verify start, firmware image verify progress, firmware 
   * image verify end.
   *
   * If any firmware image verify error events are sent during the process
   * the firmware image is considered corrupted.
   *
   * See sbIDeviceEvent for more infomation about event payload. 
   *
   * Events must be sent to both the device and the listener (if it is specified 
   * during the call).
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbMockDeviceFirmwareHandler::OnHttpRequestCompleted()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  handlerstate_t state = GetState();

  switch(state) {
    case HANDLER_REFRESHING_INFO: {
      rv = HandleRefreshInfoRequest();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    break;

    default:
      NS_WARNING("No code!");
  }

  return NS_OK;
}

nsresult 
sbMockDeviceFirmwareHandler::HandleRefreshInfoRequest()
{
  PRUint32 status = 0;
  
  nsresult rv = mXMLHttpRequest->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  printf("HTTP status code: %d\n", status);  

  nsCOMPtr<nsIDOMDocument> document;
  rv = mXMLHttpRequest->GetResponseXML(getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNodeList> rootNodeList;
  rv = document->GetElementsByTagName(NS_LITERAL_STRING("firmware"),
                                      getter_AddRefs(rootNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rootNodeListLength = 0;
  rv = rootNodeList->GetLength(&rootNodeListLength);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: Only one 'firmware' node is allowed.
  NS_ENSURE_TRUE(rootNodeListLength == 1, NS_ERROR_FAILURE);

  nsCOMPtr<nsIDOMNode> rootNode;
  rv = rootNodeList->Item(0, getter_AddRefs(rootNode));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNodeList> childNodes;
  rv = rootNode->GetChildNodes(getter_AddRefs(childNodes));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 childNodeListLength = 0;
  rv = childNodes->GetLength(&childNodeListLength);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 i = 0; i < childNodeListLength; ++i) {
    nsCOMPtr<nsIDOMNode> domNode;
    rv = childNodes->Item(i, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMElement> element = do_QueryInterface(domNode, &rv);
    if(NS_FAILED(rv)) {
      continue;
    }

    nsString tagName;
    rv = element->GetTagName(tagName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = element->GetAttribute(NS_LITERAL_STRING("value"), value);
    if(NS_FAILED(rv)) {
      continue;
    }

    // XXXAus: We only support 'version' and 'location' nodes
    //         and they must have a 'value' attribute.
    if(tagName.EqualsLiteral("version")) {
      nsAutoMonitor mon(mMonitor);
      mReadableFirmwareVersion = value;
      mFirmwareVersion = 0x01000001;
    }
    else if(tagName.EqualsLiteral("location")) {
      nsCOMPtr<nsIURI> uri;
      rv = CreateProxiedURI(NS_ConvertUTF16toUTF8(value), 
                            getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoMonitor mon(mMonitor);
      mFirmwareLocation = uri;
    }
  }

  // XXXAus: Send firmware refresh info end

  return NS_OK;
}
