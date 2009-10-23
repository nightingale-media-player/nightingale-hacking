/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

// Mozilla includes
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsIDOMDocument.h>
#include <nsIDOMNamedNodeMap.h>
#include <nsIDOMNode.h>
#include <nsIDOMNodeList.h>
#include <nsIMutableArray.h>
#include <nsThreadUtils.h>

#define SB_DEVICE_CAPS_ELEMENT "devicecaps"
#define SB_DEVICE_CAPS_NS "http://songbirdnest.com/devicecaps/1.0"

sbDeviceXMLCapabilities::sbDeviceXMLCapabilities(
  nsIDOMDocument* aDocument) :
    mDocument(aDocument),
    mDeviceCaps(nsnull),
    mHasCapabilities(PR_FALSE)
{
  NS_ASSERTION(mDocument, "no XML document provided");
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
sbDeviceXMLCapabilities::AddFormat(PRUint32 aContentType,
                                   nsAString const & aFormat)
{
  nsCString const & formatString = NS_LossyConvertUTF16toASCII(aFormat);
  char const * format = formatString.BeginReading();
  return mDeviceCaps->AddFormats(aContentType, &format, 1);
}

nsresult
sbDeviceXMLCapabilities::Read(sbIDeviceCapabilities * aCapabilities) {
  // This function should only be called on the main thread.
  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  nsresult rv;

  mDeviceCaps = aCapabilities;

  rv = ProcessDocument(mDocument);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessDocument(nsIDOMDocument * aDocument)
{
  nsresult rv;

  nsCOMPtr<nsIDOMNodeList> domNodes;
  rv = aDocument->GetElementsByTagNameNS(NS_LITERAL_STRING(SB_DEVICE_CAPS_NS),
                                         NS_LITERAL_STRING(SB_DEVICE_CAPS_ELEMENT),
                                         getter_AddRefs(domNodes));

  NS_ENSURE_SUCCESS(rv, rv);

  if (domNodes) {
    PRUint32 nodeCount;
    rv = domNodes->GetLength(&nodeCount);
    NS_ENSURE_SUCCESS(rv, rv);
    bool capsFound = false;
    nsCOMPtr<nsIDOMNode> domNode;
    for (PRUint32 nodeIndex = 0; nodeIndex < nodeCount; ++nodeIndex) {
      rv = domNodes->Item(nodeIndex, getter_AddRefs(domNode));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString name;
      rv = domNode->GetNodeName(name);
      NS_ENSURE_SUCCESS(rv, rv);

      if (name.EqualsLiteral("devicecaps")) {
        capsFound = true;
        rv = ProcessDeviceCaps(domNode);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    if (capsFound) {
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

    rv = AddFormat(sbIDeviceCapabilities::CONTENT_AUDIO, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDeviceCaps->AddFormatType(mimeType, formatType);
    NS_ENSURE_SUCCESS(rv, rv);
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

    PRInt32 width;
    rv = attributes.GetValue(WIDTH, width);
    if (NS_FAILED(rv)) {
      NS_WARNING("Invalid width found in device settings file");
      continue;
    }

    PRInt32 height;
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

    rv = AddFormat(sbIDeviceCapabilities::CONTENT_IMAGE, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mDeviceCaps->AddFormatType(mimeType, imageFormatType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbDeviceXMLCapabilities::ProcessVideo(nsIDOMNode * aVideoNode)
{
  NS_ENSURE_ARG_POINTER(aVideoNode);

  // XXX TODO Implement once we do videos
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

    rv = AddFormat(sbIDeviceCapabilities::CONTENT_PLAYLIST, mimeType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

