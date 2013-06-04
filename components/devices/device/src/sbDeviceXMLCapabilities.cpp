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

#include "sbDeviceXMLCapabilities.h"

// Songbird includes
#include <sbIDeviceCapabilities.h>
#include <sbIDeviceProperties.h>
#include <sbMemoryUtils.h>
#include <sbStandardDeviceProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>
#include <sbVariantUtilsLib.h>

// Mozilla includes
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMNamedNodeMap.h>
#include <nsIDOMNode.h>
#include <nsIDOMNodeList.h>
#include <nsIMutableArray.h>
#include <nsIScriptSecurityManager.h>
#include <nsIPropertyBag2.h>
#include <nsIXMLHttpRequest.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsTArray.h>
#include <nsThreadUtils.h>

#define SB_DEVICE_CAPS_ELEMENT "devicecaps"
#define SB_DEVICE_CAPS_NS "http://songbirdnest.com/devicecaps/1.0"

sbDeviceXMLCapabilities::sbDeviceXMLCapabilities(nsIDOMElement* aRootElement,
                                                 sbIDevice*     aDevice) :
    mDevice(aDevice),
    mDeviceCaps(nsnull),
    mRootElement(aRootElement),
    mHasCapabilities(PR_FALSE)
{
  NS_ASSERTION(mRootElement, "no device capabilities element provided");
}

sbDeviceXMLCapabilities::~sbDeviceXMLCapabilities()
{
}

nsresult
sbDeviceXMLCapabilities::AddFunctionType(PRUint32 aFunctionType)
{
  return mDeviceCaps->SetFunctionTypes(&aFunctionType, 1);
}

nsresult
sbDeviceXMLCapabilities::AddContentType(PRUint32 aFunctionType,
                                        PRUint32 aContentType)
{
  return mDeviceCaps->AddContentTypes(aFunctionType, &aContentType, 1);
}

nsresult
sbDeviceXMLCapabilities::AddMimeType(PRUint32 aContentType,
                                     nsAString const & aMimeType)
{
  nsCString const & mimeTypeString = NS_LossyConvertUTF16toASCII(aMimeType);
  char const * mimeType = mimeTypeString.BeginReading();
  return mDeviceCaps->AddMimeTypes(aContentType, &mimeType, 1);
}

nsresult
sbDeviceXMLCapabilities::Read(sbIDeviceCapabilities * aCapabilities) {
  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  nsresult rv;

  mDeviceCaps = aCapabilities;

  rv = ProcessCapabilities(mRootElement);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

/* static */ nsresult
sbDeviceXMLCapabilities::GetCapabilities
                           (sbIDeviceCapabilities** aCapabilities,
                            nsIDOMDocument*         aDocument,
                            sbIDevice*              aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ENSURE_ARG_POINTER(aDocument);

  // Function variables.
  nsresult rv;

  // No capabilities are available yet.
  *aCapabilities = nsnull;

  // Get the document element.
  nsCOMPtr<nsIDOMElement> documentElem;
  rv = aDocument->GetDocumentElement(getter_AddRefs(documentElem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the capabilities.
  rv = GetCapabilities(aCapabilities, documentElem, aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ nsresult
sbDeviceXMLCapabilities::GetCapabilities
                           (sbIDeviceCapabilities** aCapabilities,
                            nsIDOMNode*             aDeviceCapsRootNode,
                            sbIDevice*              aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ENSURE_ARG_POINTER(aDeviceCapsRootNode);

  // Function variables.
  nsresult rv;

  // No capabilities are available yet.
  *aCapabilities = nsnull;

  // Get the device capabilities root element.  There are no capabilities if
  // root node is not an element.
  nsCOMPtr<nsIDOMElement>
    deviceCapsRootElem = do_QueryInterface(aDeviceCapsRootNode, &rv);
  if (NS_FAILED(rv))
    return NS_OK;

  // Read the device capabilities for the device.
  nsCOMPtr<sbIDeviceCapabilities> deviceCapabilities =
    do_CreateInstance(SONGBIRD_DEVICECAPABILITIES_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceCapabilities->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  sbDeviceXMLCapabilities xmlCapabilities(deviceCapsRootElem, aDevice);
  rv = xmlCapabilities.Read(deviceCapabilities);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceCapabilities->ConfigureDone();
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  if (xmlCapabilities.HasCapabilities())
    deviceCapabilities.forget(aCapabilities);

  return NS_OK;
}

/* static */ nsresult
sbDeviceXMLCapabilities::AddCapabilities
                           (sbIDeviceCapabilities* aCapabilities,
                            const char*            aXMLCapabilitiesSpec,
                            PRBool*                aAddedCapabilities,
                            sbIDevice*             aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ENSURE_ARG_POINTER(aXMLCapabilitiesSpec);

  // Function variables.
  nsresult rv;

  // No capabilities have yet been added.
  if (aAddedCapabilities)
    *aAddedCapabilities = PR_FALSE;

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

  // Read the device capabilities file.
  rv = xmlHttpRequest->Open(NS_LITERAL_CSTRING("GET"),
                           nsCString(aXMLCapabilitiesSpec),
                           PR_FALSE,                  // async
                           SBVoidString(),            // user
                           SBVoidString());           // password
  NS_ENSURE_SUCCESS(rv, rv);
  rv = xmlHttpRequest->Send(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device capabilities root element.
  nsCOMPtr<nsIDOMElement>  deviceCapsElem;
  nsCOMPtr<nsIDOMDocument> deviceCapabilitiesDocument;
  rv = xmlHttpRequest->GetResponseXML
                         (getter_AddRefs(deviceCapabilitiesDocument));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deviceCapabilitiesDocument->GetDocumentElement
                                     (getter_AddRefs(deviceCapsElem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device capabilities.
  rv = AddCapabilities(aCapabilities,
                       deviceCapsElem,
                       aAddedCapabilities,
                       aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ nsresult
sbDeviceXMLCapabilities::AddCapabilities
                           (sbIDeviceCapabilities* aCapabilities,
                            nsIDOMNode*            aDeviceCapsRootNode,
                            PRBool*                aAddedCapabilities,
                            sbIDevice*             aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aCapabilities);
  NS_ENSURE_ARG_POINTER(aDeviceCapsRootNode);

  // Function variables.
  nsresult rv;

  // No capabilities have yet been added.
  if (aAddedCapabilities)
    *aAddedCapabilities = PR_FALSE;

  // Get the device capabilities.
  nsCOMPtr<sbIDeviceCapabilities> deviceCapabilities;
  rv = GetCapabilities(getter_AddRefs(deviceCapabilities),
                       aDeviceCapsRootNode,
                       aDevice);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add any device capabilities.
  if (deviceCapabilities) {
    rv = aCapabilities->AddCapabilities(deviceCapabilities);
    NS_ENSURE_SUCCESS(rv, rv);
    if (aAddedCapabilities)
      *aAddedCapabilities = PR_TRUE;
  }

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessCapabilities(nsIDOMNode* aRootNode)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aRootNode);

  // Function variables.
  nsresult rv;

  // Get the list of all device capabilities nodes.  If none exist, there are no
  // capabilities to process.
  nsCOMPtr<nsIDOMElement> rootElement = do_QueryInterface(aRootNode, &rv);
  if (NS_FAILED(rv))
    return NS_OK;
  nsCOMPtr<nsIDOMNodeList> deviceCapsNodeList;
  rv = rootElement->GetElementsByTagNameNS
                      (NS_LITERAL_STRING(SB_DEVICE_CAPS_NS),
                       NS_LITERAL_STRING(SB_DEVICE_CAPS_ELEMENT),
                       getter_AddRefs(deviceCapsNodeList));
  if (NS_FAILED(rv) || !deviceCapsNodeList)
    return NS_OK;

  // Check each device capabilities node for a matching item.
  PRUint32 nodeCount;
  rv = deviceCapsNodeList->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 i = 0; i < nodeCount; i++) {
    // Get the next device capabilities node.
    nsCOMPtr<nsIDOMNode> deviceCapsNode;
    rv = deviceCapsNodeList->Item(i, getter_AddRefs(deviceCapsNode));
    NS_ENSURE_SUCCESS(rv, rv);

    // Process device capabilities if they match device.
    PRBool deviceMatches;
    rv = DeviceMatchesCapabilitiesNode(deviceCapsNode, &deviceMatches);
    NS_ENSURE_SUCCESS(rv, rv);
    if (deviceMatches) {
      rv = ProcessDeviceCaps(deviceCapsNode);
      NS_ENSURE_SUCCESS(rv, rv);
      mHasCapabilities = PR_TRUE;
    }
  }

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessDeviceCaps(nsIDOMNode * aDevCapNode)
{
  nsCOMPtr<nsIDOMNodeList> nodes;
  nsresult rv = aDevCapNode->GetChildNodes(getter_AddRefs(nodes));
  if (nodes) {
    PRUint32 nodeCount;
    rv = nodes->GetLength(&nodeCount);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIDOMNode> domNode;
    for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
      rv = nodes->Item(nodeIndex, getter_AddRefs(domNode));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString name;
      rv = domNode->GetNodeName(name);
      NS_ENSURE_SUCCESS(rv, rv);

      if (name.Equals(NS_LITERAL_STRING("audio"))) {
        rv = ProcessAudio(domNode);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (name.Equals(NS_LITERAL_STRING("image"))) {
        rv = ProcessImage(domNode);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (name.Equals(NS_LITERAL_STRING("video"))) {
        rv = ProcessVideo(domNode);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (name.Equals(NS_LITERAL_STRING("playlist"))) {
        rv = ProcessPlaylist(domNode);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  return NS_OK;
}

/**
 * Helper class that is used to retrieve attribute values from DOM node
 */
class sbDOMNodeAttributes
{
public:
  /**
   * Initialize the object with a DOM node
   */
  sbDOMNodeAttributes(nsIDOMNode * aNode)
  {
    if (aNode) {
      aNode->GetAttributes(getter_AddRefs(mAttributes));
    }
  }
  /**
   * Retrieves a string attribute value
   * \param aName The name of the attribute
   * \param aValue The string to receive the value
   */
  nsresult
  GetValue(nsAString const & aName,
           nsAString & aValue)
  {
    NS_ENSURE_TRUE(mAttributes, NS_ERROR_FAILURE);
    nsCOMPtr<nsIDOMNode> node;
    nsresult rv = mAttributes->GetNamedItem(aName, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    if (node) {
      rv = node->GetNodeValue(aValue);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }

    return NS_ERROR_NOT_AVAILABLE;
  }

  /**
   * Retrieve an integer attribute value
   * \param aName The name of the attribute
   * \param aValue The integer to receive the value
   */
  nsresult
  GetValue(nsAString const & aName,
           PRInt32 & aValue)
  {
    nsString value;
    nsresult rv = GetValue(aName, value);
    // If not found then just return the error, no need to log
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      return rv;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    aValue = value.ToInteger(&rv, 10);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }
private:
  nsCOMPtr<nsIDOMNamedNodeMap> mAttributes;
};

/**
 * Gets the content value of the node
 * \param aValueNode The DOM node to get the value
 * \param aValue Receives the value from the node
 */
static nsresult
GetNodeValue(nsIDOMNode * aValueNode, nsAString & aValue)
{
  NS_ENSURE_ARG_POINTER(aValueNode);

  nsCOMPtr<nsIDOMNodeList> nodes;
  nsresult rv = aValueNode->GetChildNodes(getter_AddRefs(nodes));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 nodeCount;
  rv = nodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeCount > 0) {
    nsCOMPtr<nsIDOMNode> node;
    rv = nodes->Item(0, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = node->GetNodeValue(aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

/**
 * Builds a range object from a range node
 * \param aRangeNode The range node to use to build the range object
 * \param aRange The returned range
 */
static nsresult
BuildRange(nsIDOMNode * aRangeNode, sbIDevCapRange ** aRange)
{
  NS_ENSURE_ARG_POINTER(aRangeNode);
  NS_ENSURE_ARG_POINTER(aRange);

  nsresult rv;
  nsCOMPtr<sbIDevCapRange> range =
    do_CreateInstance(SB_IDEVCAPRANGE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNodeList> nodes;
  rv = aRangeNode->GetChildNodes(getter_AddRefs(nodes));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 nodeCount;
  rv = nodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    nsCOMPtr<nsIDOMNode> node;
    rv = nodes->Item(nodeIndex, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = node->GetNodeName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    if (name.EqualsLiteral("value")) {
      nsString bitRate;
      rv = GetNodeValue(node, bitRate);
      NS_ENSURE_SUCCESS(rv, rv);
      PRInt32 bitRateValue = bitRate.ToInteger(&rv, 10);
      if (NS_SUCCEEDED(rv)) {
        rv = range->AddValue(bitRateValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else if (name.EqualsLiteral("range")) {
      sbDOMNodeAttributes attributes(node);
      PRInt32 min = 0;
      rv = attributes.GetValue(NS_LITERAL_STRING("min"), min);
      if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave at 0
         NS_ENSURE_SUCCESS(rv, rv);
      }

      PRInt32 max = 0;
      rv = attributes.GetValue(NS_LITERAL_STRING("max"), max);
      if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave at 0
        NS_ENSURE_SUCCESS(rv, rv);
      }

      PRInt32 step = 0;
      rv = attributes.GetValue(NS_LITERAL_STRING("step"), step);
      if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave at 0
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = range->Initialize(min, max, step);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  range.forget(aRange);

  return NS_OK;
}

/**
 * This method attempts to split a fraction string in the form of
 * "numerator/denominator" into two out-param unsigned integers.
 */
static
nsresult
GetStringFractionValues(const nsAString & aString,
                        PRUint32 *aOutNumerator,
                        PRUint32 *aOutDenominator)
{
  NS_ENSURE_ARG_POINTER(aOutNumerator);
  NS_ENSURE_ARG_POINTER(aOutDenominator);

  nsresult rv;
  nsTArray<nsString> splitResultArray;
  nsString_Split(aString, NS_LITERAL_STRING("/"), splitResultArray);
  NS_ENSURE_TRUE(splitResultArray.Length() > 0, NS_ERROR_UNEXPECTED);

  *aOutNumerator = splitResultArray[0].ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (splitResultArray.Length() == 2) {
    *aOutDenominator = splitResultArray[1].ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    *aOutDenominator = 1;
  }

  return NS_OK;
}

/**
 * This method will retrieve a set of values or a min/max value depending on
 * which value type is specified. If the value is a range (i.e. has a min/max
 * value) |aOutIsRange| will be true and the first and second element in
 * |aOutCapRangeArray| will be the min and max values respectively.
 */
static
nsresult
GetFractionRangeValues(nsIDOMNode * aDOMNode,
                       nsIArray **aOutCapRangeArray,
                       PRBool *aOutIsRange)
{
  NS_ENSURE_ARG_POINTER(aDOMNode);
  NS_ENSURE_ARG_POINTER(aOutCapRangeArray);
  NS_ENSURE_ARG_POINTER(aOutIsRange);

  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aDOMNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeCount == 0) {
    return NS_OK;
  }

  nsCOMPtr<nsIMutableArray> fractionArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString minValue, maxValue;
  nsTArray<nsString> strings;
  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = domNode->GetNodeName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = GetNodeValue(domNode, value);
    if (NS_FAILED(rv)) {
      continue;
    }

    if (name.EqualsLiteral("value")) {
      NS_ENSURE_TRUE(strings.AppendElement(value), NS_ERROR_OUT_OF_MEMORY);
    }
    else if (name.EqualsLiteral("min")) {
      minValue.Assign(value);
    }
    else if (name.EqualsLiteral("max")) {
      maxValue.Assign(value);
    }
  }

  // If the values had at least a min or a max value, use the min/max
  // attributes on the device cap range. If not, fallback to a set list of
  // values.
  PRUint32 numerator, denominator;
  if (!minValue.Equals(EmptyString()) || !maxValue.Equals(EmptyString())) {
    nsCOMPtr<sbIDevCapFraction> minCapFraction =
      do_CreateInstance("@songbirdnest.com/Songbird/Device/sbfraction;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetStringFractionValues(minValue, &numerator, &denominator);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = minCapFraction->Initialize(numerator, denominator);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDevCapFraction> maxCapFraction =
      do_CreateInstance("@songbirdnest.com/Songbird/Device/sbfraction;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetStringFractionValues(maxValue, &numerator, &denominator);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = maxCapFraction->Initialize(numerator, denominator);
    NS_ENSURE_SUCCESS(rv, rv);

    // Per the out-param comment, the first value in the array will be the
    // minimum fraction range.
    rv = fractionArray->AppendElement(minCapFraction, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    // And the second param will be the max value.
    rv = fractionArray->AppendElement(maxCapFraction, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    // Mark these values as a range.
    *aOutIsRange = PR_TRUE;
  }
  else {
    // Mark the list of values as not a range.
    *aOutIsRange = PR_FALSE;
    // Loop through all of the values and convert them to cap fractions.
    for (PRUint32 i = 0; i < strings.Length(); i++) {
      rv = GetStringFractionValues(strings[i], &numerator, &denominator);
      if (NS_FAILED(rv)) {
        continue;
      }

      nsCOMPtr<sbIDevCapFraction> curFraction =
        do_CreateInstance("@songbirdnest.com/Songbird/Device/sbfraction;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = curFraction->Initialize(numerator, denominator);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = fractionArray->AppendElement(curFraction, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return CallQueryInterface(fractionArray.get(), aOutCapRangeArray);
}

nsresult
sbDeviceXMLCapabilities::ProcessAudio(nsIDOMNode * aAudioNode)
{
  NS_ENSURE_ARG_POINTER(aAudioNode);

  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aAudioNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeCount == 0) {
    return NS_OK;
  }
  rv = AddFunctionType(sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddContentType(sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK,
                      sbIDeviceCapabilities::CONTENT_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = domNode->GetNodeName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!name.EqualsLiteral("format")) {
      continue;
    }

    sbDOMNodeAttributes attributes(domNode);

    nsString mimeType;
    rv = attributes.GetValue(NS_LITERAL_STRING("mime"),
                             mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString container;
    rv = attributes.GetValue(NS_LITERAL_STRING("container"),
                             container);
    if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave blank
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsString codec;
    rv = attributes.GetValue(NS_LITERAL_STRING("codec"),
                             codec);
    if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave blank
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsString isPreferredString;
    rv = attributes.GetValue(NS_LITERAL_STRING("preferred"),
                             isPreferredString);
    if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave blank
      NS_ENSURE_SUCCESS(rv, rv);
    }
    PRBool isPreferred = isPreferredString.EqualsLiteral("true");

    nsCOMPtr<nsIDOMNodeList> ranges;
    rv = domNode->GetChildNodes(getter_AddRefs(ranges));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 rangeCount;
    rv = ranges->GetLength(&rangeCount);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDevCapRange> bitRates;
    nsCOMPtr<sbIDevCapRange> sampleRates;
    nsCOMPtr<sbIDevCapRange> channels;
    for (PRUint32 rangeIndex = 0; rangeIndex < rangeCount; ++rangeIndex) {
      nsCOMPtr<nsIDOMNode> range;
      rv = ranges->Item(rangeIndex, getter_AddRefs(range));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString rangeName;
      rv = range->GetNodeName(rangeName);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rangeName.EqualsLiteral("bitrates")) {
        rv = BuildRange(range, getter_AddRefs(bitRates));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (rangeName.EqualsLiteral("samplerates")) {
        rv = BuildRange(range, getter_AddRefs(sampleRates));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (rangeName.EqualsLiteral("channels")) {
        rv = BuildRange(range, getter_AddRefs(channels));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    nsCOMPtr<sbIAudioFormatType> formatType =
      do_CreateInstance(SB_IAUDIOFORMATTYPE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = formatType->Initialize(NS_ConvertUTF16toUTF8(container),
                                NS_ConvertUTF16toUTF8(codec),
                                bitRates,
                                sampleRates,
                                channels,
                                nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddMimeType(sbIDeviceCapabilities::CONTENT_AUDIO, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isPreferred) {
      rv = mDeviceCaps->AddPreferredFormatType(
              sbIDeviceCapabilities::CONTENT_AUDIO,
              mimeType,
              formatType);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = mDeviceCaps->AddFormatType(sbIDeviceCapabilities::CONTENT_AUDIO,
                                      mimeType,
                                      formatType);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessImageSizes(
                                         nsIDOMNode * aImageSizeNode,
                                         nsIMutableArray * aImageSizes)
{
  NS_ENSURE_ARG_POINTER(aImageSizeNode);
  NS_ENSURE_ARG_POINTER(aImageSizes);

  nsresult rv;

  nsCOMPtr<nsIDOMNodeList> nodes;
  rv = aImageSizeNode->GetChildNodes(getter_AddRefs(nodes));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 nodeCount;
  rv = nodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(WIDTH, "width");
  NS_NAMED_LITERAL_STRING(HEIGHT, "height");

  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    nsCOMPtr<nsIDOMNode> node;
    rv = nodes->Item(nodeIndex, getter_AddRefs(node));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = node->GetNodeName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!name.EqualsLiteral("size")) {
      continue;
    }
    sbDOMNodeAttributes attributes(node);

    nsCOMPtr<sbIImageSize> imageSize =
      do_CreateInstance(SB_IMAGESIZE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 width = 0;
    rv = attributes.GetValue(WIDTH, width);
    if (NS_FAILED(rv)) {
      NS_WARNING("Invalid width found in device settings file");
      continue;
    }

    PRInt32 height = 0;
    rv = attributes.GetValue(HEIGHT, height);
    if (NS_FAILED(rv)) {
      continue;
    }
    rv = imageSize->Initialize(width, height);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aImageSizes->AppendElement(imageSize, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessImage(nsIDOMNode * aImageNode)
{
  NS_ENSURE_ARG_POINTER(aImageNode);

  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aImageNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeCount == 0) {
    return NS_OK;
  }
  rv = AddFunctionType(sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddContentType(sbIDeviceCapabilities::FUNCTION_IMAGE_DISPLAY,
                      sbIDeviceCapabilities::CONTENT_IMAGE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = domNode->GetNodeName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!name.EqualsLiteral("format")) {
      continue;
    }

    sbDOMNodeAttributes attributes(domNode);

    nsString mimeType;
    rv = attributes.GetValue(NS_LITERAL_STRING("mime"),
                             mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMutableArray> imageSizes =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1",
                        &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDevCapRange> widths;
    nsCOMPtr<sbIDevCapRange> heights;

    nsCOMPtr<nsIDOMNodeList> sections;
    rv = domNode->GetChildNodes(getter_AddRefs(sections));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 sectionCount;
    rv = sections->GetLength(&sectionCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 sectionIndex = 0;
         sectionIndex < sectionCount;
         ++sectionIndex) {
      nsCOMPtr<nsIDOMNode> section;
      rv = sections->Item(sectionIndex, getter_AddRefs(section));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString sectionName;
      rv = section->GetNodeName(sectionName);
      NS_ENSURE_SUCCESS(rv, rv);

      if (sectionName.EqualsLiteral("explicit-sizes")) {
        rv = ProcessImageSizes(section, imageSizes);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (sectionName.EqualsLiteral("widths")) {
        rv = BuildRange(section, getter_AddRefs(widths));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (sectionName.EqualsLiteral("heights")) {
        rv = BuildRange(section, getter_AddRefs(heights));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    nsCOMPtr<sbIImageFormatType> imageFormatType =
      do_CreateInstance(SB_IIMAGEFORMATTYPE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = imageFormatType->Initialize(NS_ConvertUTF16toUTF8(mimeType),
                                     imageSizes,
                                     widths,
                                     heights);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddMimeType(sbIDeviceCapabilities::CONTENT_IMAGE, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDeviceCaps->AddFormatType(sbIDeviceCapabilities::CONTENT_IMAGE,
                                    mimeType,
                                    imageFormatType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


typedef sbAutoFreeXPCOMArrayByRef<char**> sbAutoStringArray;

nsresult
sbDeviceXMLCapabilities::ProcessVideoStream(nsIDOMNode* aVideoStreamNode,
                                            sbIDevCapVideoStream** aVideoStream)
{
  // Retrieve the child nodes for the video node
  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aVideoStreamNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  // See if there are any child nodes, leave if not
  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (nodeCount == 0) {
    return NS_OK;
  }

  nsString type;
  sbDOMNodeAttributes attributes(aVideoStreamNode);

  // Failure is an option, leaves type blank
  attributes.GetValue(NS_LITERAL_STRING("type"), type);

  nsCOMPtr<nsIMutableArray> sizes =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIDevCapRange> widths;
  nsCOMPtr<sbIDevCapRange> heights;
  nsCOMPtr<sbIDevCapRange> bitRates;

  PRBool parValuesAreRange = PR_TRUE;
  nsCOMPtr<nsIArray> parValueArray;

  PRBool frameRateValuesAreRange = PR_TRUE;
  nsCOMPtr<nsIArray> frameRateValuesArray;

  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);
    nsString name;
    rv = domNode->GetNodeName(name);
    if (NS_FAILED(rv)) {
      continue;
    }
    if (name.Equals(NS_LITERAL_STRING("explicit-sizes"))) {
      rv = ProcessImageSizes(domNode, sizes);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("widths"))) {
      rv = BuildRange(domNode, getter_AddRefs(widths));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("heights"))) {
      rv = BuildRange(domNode, getter_AddRefs(heights));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("videoPARs")) ||
             name.Equals(NS_LITERAL_STRING("videoPAR")))
    {
      rv = GetFractionRangeValues(domNode,
                                  getter_AddRefs(parValueArray),
                                  &parValuesAreRange);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("frame-rates"))) {
      rv = GetFractionRangeValues(domNode,
                                  getter_AddRefs(frameRateValuesArray),
                                  &frameRateValuesAreRange);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("bit-rates"))) {
      rv = BuildRange(domNode, getter_AddRefs(bitRates));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<sbIDevCapVideoStream> videoStream =
    do_CreateInstance(SB_IDEVCAPVIDEOSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = videoStream->Initialize(NS_ConvertUTF16toUTF8(type),
                               sizes,
                               widths,
                               heights,
                               parValueArray,
                               parValuesAreRange,
                               frameRateValuesArray,
                               frameRateValuesAreRange,
                               bitRates);
  NS_ENSURE_SUCCESS(rv, rv);

  videoStream.forget(aVideoStream);

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessAudioStream(nsIDOMNode* aAudioStreamNode,
                                            sbIDevCapAudioStream** aAudioStream)
{
  // Retrieve the child nodes for the video node
  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aAudioStreamNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  // See if there are any child nodes, leave if not
  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (nodeCount == 0) {
    return NS_OK;
  }

  nsString type;
  sbDOMNodeAttributes attributes(aAudioStreamNode);

  // Failure is an option, leaves type blank
  attributes.GetValue(NS_LITERAL_STRING("type"), type);

  nsCOMPtr<sbIDevCapRange> bitRates;
  nsCOMPtr<sbIDevCapRange> sampleRates;
  nsCOMPtr<sbIDevCapRange> channels;

  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);
    nsString name;
    rv = domNode->GetNodeName(name);
    if (NS_FAILED(rv)) {
      continue;
    }
    if (name.Equals(NS_LITERAL_STRING("bit-rates"))) {
      rv = BuildRange(domNode, getter_AddRefs(bitRates));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("sample-rates"))) {
      rv = BuildRange(domNode, getter_AddRefs(sampleRates));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (name.Equals(NS_LITERAL_STRING("channels"))) {
      rv = BuildRange(domNode, getter_AddRefs(channels));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<sbIDevCapAudioStream> audioStream =
    do_CreateInstance(SB_IDEVCAPAUDIOSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = audioStream->Initialize(NS_ConvertUTF16toUTF8(type),
                               bitRates,
                               sampleRates,
                               channels);
  NS_ENSURE_SUCCESS(rv, rv);

  audioStream.forget(aAudioStream);

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessVideoFormat(nsIDOMNode* aVideoFormatNode)
{
  nsresult rv;

  sbDOMNodeAttributes attributes(aVideoFormatNode);

  nsString containerType;
  rv = attributes.GetValue(NS_LITERAL_STRING("container-type"),
                           containerType);
  if (NS_FAILED(rv)) {
    return NS_OK;
  }

  nsString isPreferredString;
  rv = attributes.GetValue(NS_LITERAL_STRING("preferred"),
                           isPreferredString);
  if (rv != NS_ERROR_NOT_AVAILABLE) { // not found error is ok, leave blank
    NS_ENSURE_SUCCESS(rv, rv);
  }
  PRBool isPreferred = isPreferredString.EqualsLiteral("true");

  // Retrieve the child nodes for the video node
  nsCOMPtr<nsIDOMNodeList> domNodes;
  rv = aVideoFormatNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  // See if there are any child nodes, leave if not
  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDevCapVideoStream> videoStream;
  nsCOMPtr<sbIDevCapAudioStream> audioStream;
  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = domNode->GetNodeName(name);
    if (NS_FAILED(rv)) {
      continue;
    }
    if (name.Equals(NS_LITERAL_STRING("video-stream"))) {
      ProcessVideoStream(domNode, getter_AddRefs(videoStream));
    }
    else if (name.Equals(NS_LITERAL_STRING("audio-stream"))) {
      ProcessAudioStream(domNode, getter_AddRefs(audioStream));
    }
  }

  nsCOMPtr<sbIVideoFormatType> videoFormat =
    do_CreateInstance(SB_IVIDEOFORMATTYPE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = videoFormat->Initialize(NS_ConvertUTF16toUTF8(containerType),
                               videoStream,
                               audioStream);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddMimeType(sbIDeviceCapabilities::CONTENT_VIDEO, containerType);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isPreferred) {
    rv = mDeviceCaps->AddPreferredFormatType(
            sbIDeviceCapabilities::CONTENT_VIDEO,
            containerType,
            videoFormat);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mDeviceCaps->AddFormatType(sbIDeviceCapabilities::CONTENT_VIDEO,
                                    containerType,
                                    videoFormat);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessVideo(nsIDOMNode * aVideoNode)
{
  NS_ENSURE_ARG_POINTER(aVideoNode);

  // Retrieve the child nodes for the video node
  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aVideoNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  // See if there are any child nodes, leave if not
  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  if (nodeCount == 0) {
    return NS_OK;
  }

  rv = AddFunctionType(sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddContentType(sbIDeviceCapabilities::FUNCTION_VIDEO_PLAYBACK,
                      sbIDeviceCapabilities::CONTENT_VIDEO);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = domNode->GetNodeName(name);
    if (NS_FAILED(rv)) {
      continue;
    }

    // Ignore things we don't know about
    if (!name.EqualsLiteral("format")) {
      continue;
    }

    ProcessVideoFormat(domNode);
  }
  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessPlaylist(nsIDOMNode * aPlaylistNode)
{
  NS_ENSURE_ARG_POINTER(aPlaylistNode);

  nsCOMPtr<nsIDOMNodeList> domNodes;
  nsresult rv = aPlaylistNode->GetChildNodes(getter_AddRefs(domNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!domNodes) {
    return NS_OK;
  }

  PRUint32 nodeCount;
  rv = domNodes->GetLength(&nodeCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (nodeCount == 0) {
    return NS_OK;
  }
  //XXXeps it shouldn't be specific to audio. MEDIA_PLAYBACK???
  rv = AddFunctionType(sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddContentType(sbIDeviceCapabilities::FUNCTION_AUDIO_PLAYBACK,
                      sbIDeviceCapabilities::CONTENT_PLAYLIST);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> domNode;
  for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
    rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString name;
    rv = domNode->GetNodeName(name);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!name.EqualsLiteral("format")) {
      continue;
    }

    sbDOMNodeAttributes attributes(domNode);

    nsString mimeType;
    rv = attributes.GetValue(NS_LITERAL_STRING("mime"),
                             mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString pathSeparator;
    rv = attributes.GetValue(NS_LITERAL_STRING("pathSeparator"), pathSeparator);
    // If not found, use a void string  
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      pathSeparator.SetIsVoid(PR_TRUE);
    } else {
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = AddMimeType(sbIDeviceCapabilities::CONTENT_PLAYLIST, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPlaylistFormatType> formatType =
      do_CreateInstance(SB_IPLAYLISTFORMATTYPE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = formatType->Initialize(NS_ConvertUTF16toUTF8(pathSeparator));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDeviceCaps->AddFormatType(sbIDeviceCapabilities::CONTENT_PLAYLIST,
                                    mimeType,
                                    formatType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::DeviceMatchesCapabilitiesNode
                           (nsIDOMNode * aCapabilitiesNode,
                            PRBool * aDeviceMatches)
{
  NS_ENSURE_ARG_POINTER(aCapabilitiesNode);
  NS_ENSURE_ARG_POINTER(aDeviceMatches);

  PRUint32 nodeCount;
  nsresult rv;

  // Get the devices node.  Device matches by default if no devices node is
  // specified.
  nsCOMPtr<nsIDOMNode> devicesNode;
  rv = GetFirstChildByTagName(aCapabilitiesNode,
                              "devices",
                              getter_AddRefs(devicesNode));
  NS_ENSURE_SUCCESS(rv, rv);
  if (!devicesNode) {
    *aDeviceMatches = PR_TRUE;
    return NS_OK;
  }

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

nsresult
sbDeviceXMLCapabilities::DeviceMatchesDeviceNode
                           (nsIDOMNode * aDeviceNode,
                            nsIPropertyBag2 * aDeviceProperties,
                            PRBool * aDeviceMatches)
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

nsresult
sbDeviceXMLCapabilities::GetFirstChildByTagName(nsIDOMNode*  aNode,
                                                const char*        aTagName,
                                                nsIDOMNode** aChildNode)
{
  NS_ENSURE_ARG_POINTER(aTagName);
  NS_ENSURE_ARG_POINTER(aChildNode);

  nsresult rv;

  // Get the list of child nodes.
  nsCOMPtr<nsIDOMNodeList> childNodeList;
  rv = aNode->GetChildNodes(getter_AddRefs(childNodeList));
  NS_ENSURE_SUCCESS(rv, rv);

  // Search the children for a matching tag name.
  nsAutoString tagName;
  tagName.AssignLiteral(aTagName);
  PRUint32 childNodeCount;
  rv = childNodeList->GetLength(&childNodeCount);
  NS_ENSURE_SUCCESS(rv, rv);
  for (PRUint32 nodeIndex = 0; nodeIndex < childNodeCount; ++nodeIndex) {
    // Get the next child node.
    nsCOMPtr<nsIDOMNode> childNode;
    rv = childNodeList->Item(nodeIndex, getter_AddRefs(childNode));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the child node name.
    nsString nodeName;
    rv = childNode->GetNodeName(nodeName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Return child node if the tag name matches.
    if (nodeName.Equals(tagName)) {
      childNode.forget(aChildNode);
      return NS_OK;
    }
  }

  // No child found.
  *aChildNode = nsnull;

  return NS_OK;
}
