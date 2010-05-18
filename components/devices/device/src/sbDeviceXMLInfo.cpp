/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird device XML info.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceXMLInfo.cpp
 * \brief Songbird Device XML Info Source.
 */

//------------------------------------------------------------------------------
//
// Songbird device XML info imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDeviceXMLInfo.h"

// Songbird imports.
#include <sbIDeviceProperties.h>
#include <sbStandardDeviceProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>
#include <sbVariantUtilsLib.h>

// Mozilla imports.
#include <nsIDOMNamedNodeMap.h>
#include <nsIDOMNodeList.h>
#include <nsIMutableArray.h>
#include <nsIPropertyBag2.h>
#include <nsIScriptSecurityManager.h>
#include <nsIWritablePropertyBag.h>
#include <nsIXMLHttpRequest.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>


//------------------------------------------------------------------------------
//
// Public Songbird device XML info services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Read
//

nsresult sbDeviceXMLInfo::Read(const char* aDeviceXMLInfoSpec)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceXMLInfoSpec);

  // Function variables.
  nsresult rv;

  // Create an XMLHttpRequest object.
  nsCOMPtr<nsIXMLHttpRequest>
    xmlHttpRequest = do_CreateInstance(NS_XMLHTTPREQUEST_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIScriptSecurityManager> ssm =
    do_GetService(NS_SCRIPTSECURITYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrincipal> principal;
  rv = ssm->GetSystemPrincipal(getter_AddRefs(principal));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = xmlHttpRequest->Init(principal, nsnull, nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the device info file.
  rv = xmlHttpRequest->OpenRequest(NS_LITERAL_CSTRING("GET"),
                                   nsDependentCString(aDeviceXMLInfoSpec),
                                   PR_FALSE,                  // async
                                   SBVoidString(),            // user
                                   SBVoidString());           // password
  NS_ENSURE_SUCCESS(rv, rv);
  rv = xmlHttpRequest->Send(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device info document.
  nsCOMPtr<nsIDOMDocument> deviceInfoDocument;
  rv = xmlHttpRequest->GetResponseXML(getter_AddRefs(deviceInfoDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the device info.
  rv = Read(deviceInfoDocument);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// Read
//

nsresult sbDeviceXMLInfo::Read(nsIDOMDocument* aDeviceXMLInfoDocument)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceXMLInfoDocument);

  // Function variables.
  nsresult rv;

  // Get the list of all device info elements.
  nsCOMPtr<nsIDOMNodeList> nodeList;
  rv = aDeviceXMLInfoDocument->GetElementsByTagNameNS
                                 (NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
                                  NS_LITERAL_STRING("deviceinfo"),
                                  getter_AddRefs(nodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Search all device info elements for one that matches target device.
  PRUint32 nodeCount;
  rv = nodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < nodeCount; i++) {
    // Get the next device info element.
    nsCOMPtr<nsIDOMNode> node;
    rv = nodeList->Item(i, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    // Use device info node if it matches target device.
    PRBool deviceMatches;
    rv = DeviceMatchesDeviceInfoNode(node, &deviceMatches);
    NS_ENSURE_SUCCESS(rv, rv);
    if (deviceMatches) {
      mDeviceInfoElement = do_QueryInterface(node, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      break;
    }
  }

  return NS_OK;
}


//-------------------------------------
//
// GetDeviceInfoPresent
//

nsresult
sbDeviceXMLInfo::GetDeviceInfoPresent(PRBool* aDeviceInfoPresent)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceInfoPresent);

  // Device info is present if the device info element is available.
  if (mDeviceInfoElement)
    *aDeviceInfoPresent = PR_TRUE;
  else
    *aDeviceInfoPresent = PR_FALSE;

  return NS_OK;
}


//-------------------------------------
//
// GetDeviceInfoElement
//

nsresult
sbDeviceXMLInfo::GetDeviceInfoElement(nsIDOMElement** aDeviceInfoElement)
{
  NS_ENSURE_ARG_POINTER(aDeviceInfoElement);
  NS_IF_ADDREF(*aDeviceInfoElement = mDeviceInfoElement);
  return NS_OK;
}


//-------------------------------------
//
// GetDeviceFolder
//

nsresult
sbDeviceXMLInfo::GetDeviceFolder(const nsAString& aFolderType,
                                 nsAString&       aFolderURL)
{
  nsresult rv;

  // Default to no folder.
  aFolderURL.SetIsVoid(PR_TRUE);

  // Do nothing more if no device info element.
  if (!mDeviceInfoElement)
    return NS_OK;

  // Get the list of folder nodes.
  nsCOMPtr<nsIDOMNodeList> folderNodeList;
  rv = mDeviceInfoElement->GetElementsByTagNameNS
                             (NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
                              NS_LITERAL_STRING("folder"),
                              getter_AddRefs(folderNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Search for a matching folder element.
  PRUint32 nodeCount;
  rv = folderNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < nodeCount; i++) {
    // Get the next folder element.
    nsCOMPtr<nsIDOMElement> folderElement;
    nsCOMPtr<nsIDOMNode>    folderNode;
    rv = folderNodeList->Item(i, getter_AddRefs(folderNode));
    NS_ENSURE_SUCCESS(rv, rv);
    folderElement = do_QueryInterface(folderNode, &rv);
    if (NS_FAILED(rv))
      continue;

    // Return folder URL if the folder element is of the specified type.
    nsAutoString folderType;
    rv = folderElement->GetAttribute(NS_LITERAL_STRING("type"), folderType);
    if (NS_FAILED(rv))
      continue;
    if (folderType.Equals(aFolderType)) {
      rv = folderElement->GetAttribute(NS_LITERAL_STRING("url"), aFolderURL);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  return NS_OK;
}


//-------------------------------------
//
// GetDeviceFolder
//

nsresult
sbDeviceXMLInfo::GetDeviceFolder(PRUint32   aContentType,
                                 nsAString& aFolderURL)
{
  nsresult rv;

  // Map from content type to device XML info folder element type.
  static const char* folderContentTypeMap[] = {
    "",
    "",
    "",
    "music",
    "photo",
    "video",
    "playlist",
    "album"
  };

  // Default to no folder.
  aFolderURL.Truncate();

  // Validate content type.
  if (aContentType >= NS_ARRAY_LENGTH(folderContentTypeMap))
    return NS_OK;

  // Get the folder type.
  nsAutoString folderType;
  folderType.AssignLiteral(folderContentTypeMap[aContentType]);
  if (folderType.IsEmpty())
    return NS_OK;

  // Get the device folder URL.
  rv = GetDeviceFolder(folderType, aFolderURL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-------------------------------------
//
// GetExcludedFolders
//
nsresult
sbDeviceXMLInfo::GetExcludedFolders(nsAString & aExcludedFolders)
{
  nsresult rv;

  aExcludedFolders.Truncate();

  // Do nothing more if no device info element.
  if (!mDeviceInfoElement)
    return NS_OK;

  // Get the list of exclude folder nodes.
  nsCOMPtr<nsIDOMNodeList> excludeNodeList;
  rv = mDeviceInfoElement->GetElementsByTagNameNS
                             (NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
                              NS_LITERAL_STRING("excludefolder"),
                              getter_AddRefs(excludeNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get all excluded folders.
  PRUint32 nodeCount;
  rv = excludeNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < nodeCount; i++) {
    // Get the next exclude folder element.
    nsCOMPtr<nsIDOMElement> excludeElement;
    nsCOMPtr<nsIDOMNode>    excludeNode;
    rv = excludeNodeList->Item(i, getter_AddRefs(excludeNode));
    NS_ENSURE_SUCCESS(rv, rv);
    excludeElement = do_QueryInterface(excludeNode, &rv);
    if (NS_SUCCEEDED(rv)) {
      nsString excludeURL;
      rv = excludeElement->GetAttribute(NS_LITERAL_STRING("url"), excludeURL);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!aExcludedFolders.IsEmpty()) {
        aExcludedFolders.AppendLiteral(",");
      }
      aExcludedFolders.Append(excludeURL);
    }
  }
  return NS_OK;
}


//-------------------------------------
//
// GetMountTimeout
//

nsresult
sbDeviceXMLInfo::GetMountTimeout(PRUint32* aMountTimeout)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMountTimeout);

  // Function variables.
  nsresult rv;

  // Check if a device info element is available.
  if (!mDeviceInfoElement)
    return NS_ERROR_NOT_AVAILABLE;

  // Get the list of mount timeout nodes.
  nsCOMPtr<nsIDOMNodeList> mountTimeoutNodeList;
  rv = mDeviceInfoElement->GetElementsByTagNameNS
                             (NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
                              NS_LITERAL_STRING("mounttimeout"),
                              getter_AddRefs(mountTimeoutNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if any mount timeout nodes are available.
  PRUint32 nodeCount;
  rv = mountTimeoutNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!nodeCount)
    return NS_ERROR_NOT_AVAILABLE;

  // Get the first mount timeout element.
  nsCOMPtr<nsIDOMElement> mountTimeoutElement;
  nsCOMPtr<nsIDOMNode>    mountTimeoutNode;
  rv = mountTimeoutNodeList->Item(0, getter_AddRefs(mountTimeoutNode));
  NS_ENSURE_SUCCESS(rv, rv);
  mountTimeoutElement = do_QueryInterface(mountTimeoutNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the mount timeout value.
  nsAutoString mountTimeoutString;
  rv = mountTimeoutElement->GetAttribute(NS_LITERAL_STRING("value"),
                                         mountTimeoutString);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  PRUint32 mountTimeout;
  mountTimeout = mountTimeoutString.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);
  *aMountTimeout = mountTimeout;

  return NS_OK;
}


//-------------------------------------
//
// GetDoesDeviceSupportReformat
//

nsresult
sbDeviceXMLInfo::GetDoesDeviceSupportReformat(PRBool *aOutSupportsReformat)
{
  NS_ENSURE_ARG_POINTER(aOutSupportsReformat);
  *aOutSupportsReformat = PR_TRUE;

  // Check if a device info element is available.
  NS_ENSURE_TRUE(mDeviceInfoElement, NS_ERROR_NOT_AVAILABLE);

  nsresult rv;
  nsCOMPtr<nsIDOMNodeList> supportsFormatNodeList;
  rv = mDeviceInfoElement->GetElementsByTagNameNS(
      NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
      NS_LITERAL_STRING("supportsreformat"),
      getter_AddRefs(supportsFormatNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // See if there is at least one node.
  PRUint32 nodeCount;
  rv = supportsFormatNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeCount > 0) {
    // Only process the first node value.
    nsCOMPtr<nsIDOMNode> supportsFormatNode;
    rv = supportsFormatNodeList->Item(0, getter_AddRefs(supportsFormatNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMElement> supportsFormatElement =
      do_QueryInterface(supportsFormatNode, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Read the value
    nsString supportsFormatValue;
    rv = supportsFormatElement->GetAttribute(NS_LITERAL_STRING("value"),
                                             supportsFormatValue);
    NS_ENSURE_SUCCESS(rv, rv);

    if (supportsFormatValue.Equals(NS_LITERAL_STRING("false"),
          (nsAString::ComparatorFunc)CaseInsensitiveCompare)) {
      *aOutSupportsReformat = PR_FALSE;
    }
  }


  return NS_OK;
}


//-------------------------------------
//
// GetStorageDeviceInfoList
//

nsresult
sbDeviceXMLInfo::GetStorageDeviceInfoList(nsIArray** aStorageDeviceInfoList)
{
  // Validate arguments and ensure this is called on the main thread.
  NS_ENSURE_ARG_POINTER(aStorageDeviceInfoList);
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Function variables.
  nsresult rv;

  // Check if a device info element is available.  There doesn't have to be, so,
  // if not, just return an error without any warnings.
  if (!mDeviceInfoElement)
    return NS_ERROR_NOT_AVAILABLE;

  // Get the list of storage nodes.
  nsCOMPtr<nsIDOMNodeList> storageNodeList;
  rv = mDeviceInfoElement->GetElementsByTagNameNS
                             (NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
                              NS_LITERAL_STRING("storage"),
                              getter_AddRefs(storageNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if any storage nodes are available.
  PRUint32 nodeCount;
  rv = storageNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the storage device info list.
  nsCOMPtr<nsIMutableArray> storageDeviceInfoList =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the storage device info.
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    // Get the storage device node.
    nsCOMPtr<nsIDOMNode> storageDeviceNode;
    rv = storageNodeList->Item(nodeIndex, getter_AddRefs(storageDeviceNode));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the storage device attributes.
    nsCOMPtr<nsIDOMNamedNodeMap> attributes;
    PRUint32                     attributeCount;
    rv = storageDeviceNode->GetAttributes(getter_AddRefs(attributes));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = attributes->GetLength(&attributeCount);
    NS_ENSURE_SUCCESS(rv, rv);

    // Create the storage device info property bag.
    nsCOMPtr<nsIWritablePropertyBag> storageDeviceInfo =
      do_CreateInstance("@mozilla.org/hash-property-bag;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the storage device info.
    for (PRUint32 attributeIndex = 0;
         attributeIndex < attributeCount;
         ++attributeIndex) {
      // Get the next attribute.
      nsCOMPtr<nsIDOMNode> attribute;
      rv = attributes->Item(attributeIndex, getter_AddRefs(attribute));
      NS_ENSURE_SUCCESS(rv, rv);

      // Get the attribute name.
      nsAutoString attributeName;
      rv = attribute->GetNodeName(attributeName);
      NS_ENSURE_SUCCESS(rv, rv);

      // Get the attribute value.
      nsAutoString attributeValue;
      rv = attribute->GetNodeValue(attributeValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // Set the storage device info.
      storageDeviceInfo->SetProperty(attributeName,
                                     sbNewVariant(attributeValue));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Add the storage device info.
    rv = storageDeviceInfoList->AppendElement(storageDeviceInfo, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  rv = CallQueryInterface(storageDeviceInfoList, aStorageDeviceInfoList);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// sbDeviceXMLInfo
//

sbDeviceXMLInfo::sbDeviceXMLInfo(sbIDevice* aDevice) :
  mDevice(aDevice)
{
}


//-------------------------------------
//
// ~sbDeviceXMLInfo
//

sbDeviceXMLInfo::~sbDeviceXMLInfo()
{
}


//------------------------------------------------------------------------------
//
// Private Songbird device XML info services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// DeviceMatchesDeviceInfoNode
//

nsresult
sbDeviceXMLInfo::DeviceMatchesDeviceInfoNode(nsIDOMNode* aDeviceInfoNode,
                                             PRBool*     aDeviceMatches)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceInfoNode);
  NS_ENSURE_ARG_POINTER(aDeviceMatches);

  // Function variables.
  PRUint32 nodeCount;
  nsresult rv;

  // Get the devices node.  Device matches by default if no devices node is
  // specified.
  nsCOMPtr<nsIDOMNode>     devicesNode;
  nsCOMPtr<nsIDOMNodeList> devicesNodeList;
  nsCOMPtr<nsIDOMElement>
    deviceInfoElement = do_QueryInterface(aDeviceInfoNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceInfoElement->GetElementsByTagNameNS
                            (NS_LITERAL_STRING(SB_DEVICE_INFO_NS),
                             NS_LITERAL_STRING("devices"),
                             getter_AddRefs(devicesNodeList));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = devicesNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!nodeCount) {
    *aDeviceMatches = PR_TRUE;
    return NS_OK;
  }
  rv = devicesNodeList->Item(0, getter_AddRefs(devicesNode));
  NS_ENSURE_SUCCESS(rv, rv);

  // If no device was specified, the device doesn't match.
  if (!mDevice) {
    *aDeviceMatches = PR_FALSE;
    return NS_OK;
  }

  // Get the device properties.
  nsCOMPtr<sbIDeviceProperties> deviceProperties;
  rv = mDevice->GetProperties(getter_AddRefs(deviceProperties));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPropertyBag2> properties;
  rv = deviceProperties->GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the devices node child list.  Device doesn't match if list is empty.
  nsCOMPtr<nsIDOMNodeList> childNodeList;
  rv = devicesNode->GetChildNodes(getter_AddRefs(childNodeList));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!childNodeList) {
    *aDeviceMatches = PR_FALSE;
    return NS_OK;
  }

  // Check each child node for a matching device node.
  rv = childNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    // Get the next child node.
    nsCOMPtr<nsIDOMNode> childNode;
    rv = childNodeList->Item(nodeIndex, getter_AddRefs(childNode));
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip all but device nodes.
    nsString nodeName;
    rv = childNode->GetNodeName(nodeName);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!nodeName.EqualsLiteral("device")) {
      continue;
    }

    // Check if the device matches the device node.
    PRBool matches;
    rv = DeviceMatchesDeviceNode(childNode, properties, &matches);
    NS_ENSURE_SUCCESS(rv, rv);
    if (matches) {
      *aDeviceMatches = PR_TRUE;
      return NS_OK;
    }
  }

  // No match found.
  *aDeviceMatches = PR_FALSE;

  return NS_OK;
}


//-------------------------------------
//
// DeviceMatchesDeviceNode
//

nsresult
sbDeviceXMLInfo::DeviceMatchesDeviceNode(nsIDOMNode*      aDeviceNode,
                                         nsIPropertyBag2* aDeviceProperties,
                                         PRBool*          aDeviceMatches)
{
  NS_ENSURE_ARG_POINTER(aDeviceNode);
  NS_ENSURE_ARG_POINTER(aDeviceProperties);
  NS_ENSURE_ARG_POINTER(aDeviceMatches);

  nsresult rv;

  // Get the device node attributes.
  nsCOMPtr<nsIDOMNamedNodeMap> attributes;
  rv = aDeviceNode->GetAttributes(getter_AddRefs(attributes));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if each device node attribute matches the device.
  PRBool matches = PR_TRUE;
  PRUint32 attributeCount;
  rv = attributes->GetLength(&attributeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 attributeIndex = 0;
       attributeIndex < attributeCount;
       ++attributeIndex) {
    // Get the next attribute.
    nsCOMPtr<nsIDOMNode> attribute;
    rv = attributes->Item(attributeIndex, getter_AddRefs(attribute));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the attribute name.
    nsAutoString attributeName;
    rv = attribute->GetNodeName(attributeName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the attribute value.
    nsAutoString attributeValue;
    rv = attribute->GetNodeValue(attributeValue);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the corresponding device property key.
    nsAutoString deviceKey(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_BASE));
    deviceKey.Append(attributeName);

    // If the device property key does not exist, the device does not match.
    PRBool hasKey;
    rv = aDeviceProperties->HasKey(deviceKey, &hasKey);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasKey) {
      matches = PR_FALSE;
      break;
    }

    // Get the device property value.
    nsCOMPtr<nsIVariant> deviceValue;
    rv = aDeviceProperties->Get(deviceKey, getter_AddRefs(deviceValue));
    NS_ENSURE_SUCCESS(rv, rv);

    // If the device property value and the attribute value are not equal, the
    // device does not match.
    PRBool equal;
    rv = sbVariantsEqual(deviceValue, sbNewVariant(attributeValue), &equal);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!equal) {
      matches = PR_FALSE;
      break;
    }
  }

  // Return results.
  *aDeviceMatches = matches;

  return NS_OK;
}


