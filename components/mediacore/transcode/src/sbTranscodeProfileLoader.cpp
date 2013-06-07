/* vim: set sw=2 : */
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

#include "sbTranscodeProfileLoader.h"

#include <nsINode.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMNode.h>
#include <nsIDOMParser.h>
#include <nsIFile.h>
#include <nsIFileStreams.h>
#include <nsIMutableArray.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>
#include <prdtoa.h>

#include <sbVariantUtils.h>

#include "sbTranscodeProfile.h"
#include "sbTranscodeProfileProperty.h"
#include "sbTranscodeProfileAttribute.h"

NS_IMPL_THREADSAFE_ISUPPORTS2(sbTranscodeProfileLoader,
                              sbITranscodeProfileLoader,
                              nsIRunnable);

sbTranscodeProfileLoader::sbTranscodeProfileLoader()
{
}

sbTranscodeProfileLoader::~sbTranscodeProfileLoader()
{
}

/* sbITranscodeProfile loadProfile (in nsIFile aFile); */
NS_IMETHODIMP
sbTranscodeProfileLoader::LoadProfile(nsIFile *aFile,
                                      sbITranscodeProfile **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  mFile = aFile;
  if (NS_IsMainThread()) {
    // this is on the main thread, call directly
    rv = LoadProfileInternal();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CallQueryInterface(mProfile.get(), _retval);
    NS_ENSURE_SUCCESS(rv, rv);

    mProfile = nsnull;
  }
  else {
    nsCOMPtr<nsIRunnable> runnable =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIRunnable*, this), &rv);
    NS_ASSERTION(runnable, "sbTranscodeProfileLoader should implement nsIRunnable");
    rv = NS_DispatchToMainThread(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = CallQueryInterface(mProfile.get(), _retval);
    NS_ENSURE_SUCCESS(rv, rv);

    mProfile = nsnull;

    // check the return value from LoadProfileInternal
    NS_ENSURE_SUCCESS(mResult, mResult);
  }

  mFile = nsnull;
  return NS_OK;
}

/** nsIRunnable */
NS_IMETHODIMP
sbTranscodeProfileLoader::Run()
{
  mResult = LoadProfileInternal();
  return NS_OK;
}

nsresult
sbTranscodeProfileLoader::LoadProfileInternal()
{
  const PRInt32 IOFLAGS_DEFAULT = -1;
  const PRInt32 PERMISSIONS_DEFAULT = -1;
  const PRInt32 FLAGS_DEFAULT = 0;

  nsresult rv;

  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_UNEXPECTED);
  NS_ENSURE_ARG_POINTER(mFile);

  nsCOMPtr<nsIDOMParser> domParser =
    do_CreateInstance("@mozilla.org/xmlextras/domparser;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileInputStream> fileStream =
    do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileStream->Init(mFile,
                        IOFLAGS_DEFAULT,
                        PERMISSIONS_DEFAULT,
                        FLAGS_DEFAULT);

  PRUint32 fileSize;
  rv = fileStream->Available(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> doc;
  rv = domParser->ParseFromStream(fileStream,
                                  nsnull,
                                  fileSize,
                                  "text/xml",
                                  getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = fileStream->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  mProfile = new sbTranscodeProfile();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMElement> element;
  rv = doc->GetDocumentElement(getter_AddRefs(element));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> childNode;
  rv = element->GetFirstChild(getter_AddRefs(childNode));
  NS_ENSURE_SUCCESS(rv, rv);

  while (childNode) {
    nsCOMPtr<nsIDOMElement> childElement = do_QueryInterface(childNode, &rv);

    if (NS_SUCCEEDED(rv)) {
      nsString localName;
      rv = childElement->GetLocalName(localName);
      NS_ENSURE_SUCCESS(rv, rv);

      if (localName.EqualsLiteral("type")) {
        PRUint32 type;
        rv = GetType(childNode, &type);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mProfile->SetType(type);
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (localName.EqualsLiteral("description")) {
        nsCOMPtr<nsINode> node = do_QueryInterface(childNode);
        if (node) {
          nsString textContent;
          node->GetTextContent(textContent);
          rv = mProfile->SetDescription(textContent);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      } else if (localName.EqualsLiteral("priority")) {
        nsCOMPtr<nsINode> node = do_QueryInterface(childNode);
        if (node) {
          nsString textContent;
          node->GetTextContent(textContent);
          PRInt32 priority = textContent.ToInteger(&rv);
          NS_ENSURE_SUCCESS(rv, rv);

          PRBool hasQuality = PR_FALSE;
          rv = childElement->HasAttribute(NS_LITERAL_STRING("quality"),
                                          &hasQuality);
          NS_ENSURE_SUCCESS(rv, rv);
          if (hasQuality) {
            nsString qualityString;
            rv = childElement->GetAttribute(NS_LITERAL_STRING("quality"),
                                            qualityString);
            NS_ENSURE_SUCCESS(rv, rv);
            NS_LossyConvertUTF16toASCII qualityCString(qualityString);
            PRFloat64 quality = PR_strtod(qualityCString.get(), nsnull);
            rv = mProfile->AddPriority((double)quality, priority);
            NS_ENSURE_SUCCESS(rv, rv);
          } else {
            rv = mProfile->SetPriority(priority);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      } else if (localName.EqualsLiteral("id")) {
        nsCOMPtr<nsINode> node = do_QueryInterface(childNode);
        if (node) {
          nsString textContent;
          node->GetTextContent(textContent);
          rv = mProfile->SetId(textContent);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      } else if (localName.EqualsLiteral("extension")) {
        nsCOMPtr<nsINode> node = do_QueryInterface(childNode);
        if (node) {
          nsString textContent;
          node->GetTextContent(textContent);
          rv = mProfile->SetFileExtension(NS_ConvertUTF16toUTF8(textContent));
          NS_ENSURE_SUCCESS(rv, rv);
        }
      } else if (localName.EqualsLiteral("container")) {
        rv = ProcessContainer(mProfile, CONTAINER_GENERIC, childElement);
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (localName.EqualsLiteral("audio")) {
        rv = ProcessContainer(mProfile, CONTAINER_AUDIO, childElement);
        NS_ENSURE_SUCCESS(rv, rv);
      } else if (localName.EqualsLiteral("video")) {
        rv = ProcessContainer(mProfile, CONTAINER_VIDEO, childElement);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    rv = childNode->GetNextSibling(getter_AddRefs(childNode));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbTranscodeProfileLoader::GetType(nsIDOMNode* aTypeNode, PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aTypeNode);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<nsINode> node = do_QueryInterface(aTypeNode, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString type;
  node->GetTextContent(type);

  if (type.EqualsLiteral("audio")) {
    *_retval = sbITranscodeProfile::TRANSCODE_TYPE_AUDIO;
  } else if (type.EqualsLiteral("video")) {
    *_retval = sbITranscodeProfile::TRANSCODE_TYPE_AUDIO_VIDEO;
  } else if (type.EqualsLiteral("image")) {
    *_retval = sbITranscodeProfile::TRANSCODE_TYPE_IMAGE;
  } else {
    return NS_ERROR_INVALID_ARG;
  }
  return NS_OK;
}

nsresult
sbTranscodeProfileLoader::ProcessAttribute(nsIDOMElement* aAttributeElement,
                                           sbITranscodeProfileAttribute** _retval)
{
  NS_ENSURE_ARG_POINTER(aAttributeElement);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsRefPtr<sbTranscodeProfileAttribute> attr =
    new sbTranscodeProfileAttribute();
  NS_ENSURE_TRUE(attr, NS_ERROR_OUT_OF_MEMORY);

  nsString name;
  rv = aAttributeElement->GetAttribute(NS_LITERAL_STRING("name"),
                                       name);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = attr->SetName(name);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString type;
  rv = aAttributeElement->GetAttribute(NS_LITERAL_STRING("type"),
                                       type);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString valueString;
  rv = aAttributeElement->GetAttribute(NS_LITERAL_STRING("value"),
                                       valueString);
  NS_ENSURE_SUCCESS(rv, rv);

  if (type.EqualsLiteral("int")) {
    PRInt32 value;

    value = valueString.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = attr->SetValue(sbNewVariant(value));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (type.EqualsLiteral("string")) {
    rv = attr->SetValue(sbNewVariant(valueString));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  rv = CallQueryInterface(attr.get(), _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbTranscodeProfileLoader::ProcessProperty(nsIDOMElement* aPropertyElement,
                                          sbITranscodeProfileProperty** _retval)
{
  NS_ENSURE_ARG_POINTER(aPropertyElement);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsRefPtr<sbTranscodeProfileProperty> property =
    new sbTranscodeProfileProperty();
  NS_ENSURE_TRUE(property, NS_ERROR_OUT_OF_MEMORY);

  nsString attributeValue;
  rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("name"),
                                      attributeValue);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = property->SetPropertyName(attributeValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("hidden"),
                                      attributeValue);
  NS_ENSURE_SUCCESS(rv, rv);
  if (attributeValue.IsEmpty() ||
      attributeValue.EqualsLiteral("0") ||
      attributeValue.LowerCaseEqualsLiteral("false"))
  {
    rv = property->SetHidden(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = property->SetHidden(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("mapping"),
                                      attributeValue);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!attributeValue.IsEmpty()) {
    rv = property->SetMapping(NS_ConvertUTF16toUTF8(attributeValue));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("scale"),
                                      attributeValue);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!attributeValue.IsEmpty()) {
    rv = property->SetScale(NS_ConvertUTF16toUTF8(attributeValue));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("type"),
                                      attributeValue);
  NS_ENSURE_SUCCESS(rv, rv);

  if (attributeValue.EqualsLiteral("int")) {
    PRInt32 value;

    rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("min"),
                                        attributeValue);
    NS_ENSURE_SUCCESS(rv, rv);

    value = attributeValue.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->SetValueMin(sbNewVariant(value));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("max"),
                                        attributeValue);
    NS_ENSURE_SUCCESS(rv, rv);

    value = attributeValue.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->SetValueMax(sbNewVariant(value));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aPropertyElement->GetAttribute(NS_LITERAL_STRING("default"),
                                        attributeValue);
    NS_ENSURE_SUCCESS(rv, rv);

    value = attributeValue.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = property->SetValue(sbNewVariant(value));
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  rv = CallQueryInterface(property.get(), _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbTranscodeProfileLoader::ProcessContainer(sbTranscodeProfile* aProfile,
                                           ContainerType_t aContainerType,
                                           nsIDOMElement* aContainer)
{
  NS_ENSURE_ARG_POINTER(aProfile);
  NS_ENSURE_ARG_POINTER(aContainer);

  nsresult rv;

  nsCOMPtr<nsIMutableArray> properties =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> attributes =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> childNode;
  rv = aContainer->GetFirstChild(getter_AddRefs(childNode));
  NS_ENSURE_SUCCESS(rv, rv);

  while (childNode) {
    nsCOMPtr<nsIDOMElement> childElement = do_QueryInterface(childNode);
    if (childElement) {
      nsString localName;
      rv = childElement->GetLocalName(localName);
      NS_ENSURE_SUCCESS(rv, rv);

      if (localName.EqualsLiteral("type")) {
        nsCOMPtr<nsINode> node = do_QueryInterface(childNode);
        NS_ENSURE_SUCCESS(rv, rv);

        nsString textContent;
        node->GetTextContent(textContent);

        switch (aContainerType) {
          case CONTAINER_GENERIC:
            rv = aProfile->SetContainerFormat(textContent);
            break;
          case CONTAINER_AUDIO:
            rv = aProfile->SetAudioCodec(textContent);
            break;
          case CONTAINER_VIDEO:
            rv = aProfile->SetVideoCodec(textContent);
            break;
          default:
            rv = NS_ERROR_UNEXPECTED;
        }
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (localName.EqualsLiteral("attribute")) {
        nsCOMPtr<sbITranscodeProfileAttribute> attr;
        rv = ProcessAttribute(childElement, getter_AddRefs(attr));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = attributes->AppendElement(attr, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (localName.EqualsLiteral("property")) {
        nsCOMPtr<sbITranscodeProfileProperty> property;
        rv = ProcessProperty(childElement, getter_AddRefs(property));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = properties->AppendElement(property, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (localName.EqualsLiteral("quality-property")) {
        nsString attrVal;
        rv = childElement->GetAttribute(NS_LITERAL_STRING("quality"), attrVal);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCString attrCVal = NS_LossyConvertUTF16toASCII(attrVal);
        PRFloat64 quality = PR_strtod(attrCVal.get(), nsnull);

        rv = childElement->GetAttribute(NS_LITERAL_STRING("value"), attrVal);
        NS_ENSURE_SUCCESS(rv, rv);
        attrCVal = NS_LossyConvertUTF16toASCII(attrVal);
        PRFloat64 value = PR_strtod(attrCVal.get(), nsnull);

        rv = childElement->GetAttribute(NS_LITERAL_STRING("name"), attrVal);
        NS_ENSURE_SUCCESS(rv, rv);

        if (attrVal.EqualsLiteral("bitrate")) {
          NS_ENSURE_STATE(aContainerType == CONTAINER_AUDIO);
          rv = aProfile->AddAudioBitrate((double)quality, (double)value);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else if (attrVal.EqualsLiteral("bpp")) {
          NS_ENSURE_STATE(aContainerType == CONTAINER_VIDEO);
          rv = aProfile->AddVideoBPP((double)quality, (double)value);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else {
          NS_ENSURE_TRUE(false, NS_ERROR_UNEXPECTED);
        }
      }
    }
    rv = childNode->GetNextSibling(getter_AddRefs(childNode));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  switch (aContainerType) {
    case CONTAINER_GENERIC:
      rv = aProfile->SetContainerProperties(properties);
      rv = aProfile->SetContainerAttributes(attributes);
      break;
    case CONTAINER_AUDIO:
      rv = aProfile->SetAudioProperties(properties);
      rv = aProfile->SetAudioAttributes(attributes);
      break;
    case CONTAINER_VIDEO:
      rv = aProfile->SetVideoProperties(properties);
      rv = aProfile->SetVideoAttributes(attributes);
      break;
    default:
      rv = NS_ERROR_UNEXPECTED;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
