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

#include <nsCOMPtr.h>
#include <nsIConverterInputStream.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIURI.h>
#include <nsIURL.h> 
#include <nsNetUtil.h> 
#include <nsIDOMParser.h>
#include <nsIDOMDocument.h>
#include <nsIDOMElement.h>
#include <nsIDOMNode.h>
#include <nsIDOMNodeList.h>
#include <nsIDOMNamedNodeMap.h>
#include <nsUnicharUtils.h>
#include "sbArticlesData.h"

#define DATA_URL "chrome://songbird/content/intl/sbArticleRemoval.xml" 
#define DOCUMENT_DATA "sb-article-removal-document"
#define LANGUAGE_DATA "article-removal-data"
#define ARTICLE_DATA "article"

#define CONVERTER_BUFFER_SIZE 8192

sbArticlesData *gArticlesData;

NS_IMPL_THREADSAFE_ISUPPORTS1(sbArticlesDataObserver, 
                              nsIObserver)

NS_IMPL_ISUPPORTS0(sbArticlesData::sbArticle)
NS_IMPL_ISUPPORTS0(sbArticlesData::sbArticleLanguageData)

sbArticlesData::sbArticlesData()
: mArticlesLoaded(PR_FALSE)
{
}

sbArticlesData::~sbArticlesData()
{
  UnloadArticles();
}

nsresult sbArticlesData::Init() {
  return LoadArticles();
}

nsCOMPtr<sbArticlesDataObserver> gArticlesDataObserver;

nsresult sbArticlesData::AddObserver() {
  if (gArticlesDataObserver)
    return NS_OK;

  nsresult rv;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbArticlesDataObserver> observer = new sbArticlesDataObserver();
  gArticlesDataObserver = do_QueryInterface(observer, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // "xpcom-shutdown" is called right before the app will terminate
  rv = observerService->AddObserver(gArticlesDataObserver, 
                                    NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                    PR_FALSE);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to add shutdown observer");
  
  return NS_OK;
}

nsresult sbArticlesData::RemoveObserver() {
  if (!gArticlesDataObserver)
    return NS_OK;
    
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->RemoveObserver(gArticlesDataObserver, 
                                       NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                   "Failed to remove shutdown observer");

  gArticlesDataObserver = NULL;
  
  return NS_OK;
}

nsresult sbArticlesData::LoadArticles() {
  if (mArticlesLoaded) 
    return NS_OK;
    
  nsresult rv = AddObserver();
  NS_ENSURE_SUCCESS(rv, rv);
    
  // load articles data from chrome url

  nsCOMPtr<nsIURI> dataURI;
  rv = NS_NewURI(getter_AddRefs(dataURI), NS_LITERAL_CSTRING(DATA_URL));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> input;
  rv = NS_OpenURI(getter_AddRefs(input), dataURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConverterInputStream> converterStream =
    do_CreateInstance("@mozilla.org/intl/converter-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converterStream->Init(input,
                             "UTF-8",
                             CONVERTER_BUFFER_SIZE,
                             nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicharInputStream> unichar =
    do_QueryInterface(converterStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 read = 0;
  nsString response;
  rv = unichar->ReadString(PR_UINT32_MAX, response, &read);
  NS_ENSURE_SUCCESS(rv, rv);

  // parse document
  
  nsCOMPtr<nsIDOMParser> parser =
    do_CreateInstance(NS_DOMPARSER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocument> doc;
  rv = parser->ParseFromString(response.get(), "text/xml", getter_AddRefs(doc));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // get top element
  
  nsCOMPtr<nsIDOMElement> top;
  rv = doc->GetDocumentElement(getter_AddRefs(top));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMNode> topNode;
  topNode = do_QueryInterface(top, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // check that we support it
  nsString topLocalName;
  rv = topNode->GetLocalName(topLocalName);
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!topLocalName.EqualsLiteral(DOCUMENT_DATA)) {
    // wrong document element localname, fail.
    return NS_ERROR_FAILURE;
  }
  
  // for each valid child element, load language data

  nsCOMPtr<nsIDOMNodeList> childNodes;
  rv = topNode->GetChildNodes(getter_AddRefs(childNodes));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint32 nchildren;
  rv = childNodes->GetLength(&nchildren);
  NS_ENSURE_SUCCESS(rv, rv);
  
  for (PRUint32 i=0;i<nchildren; i++) {

    // get child node

    nsCOMPtr<nsIDOMNode> childNode;
    rv = childNodes->Item(i, getter_AddRefs(childNode));
    NS_ENSURE_SUCCESS(rv, rv);
    
    PRUint16 nodeType;
    rv = childNode->GetNodeType(&nodeType);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // check that it's a language node
    if (nodeType == nsIDOMNode::ELEMENT_NODE) {
      
      nsString localName;
      rv = childNode->GetLocalName(localName);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (localName.EqualsLiteral(LANGUAGE_DATA)) {
        
        // Load language data
        nsString language;
        nsTArray<nsRefPtr<sbArticle> > data;

        // get the node attributes
        nsCOMPtr<nsIDOMNamedNodeMap> attrs;
        rv = childNode->GetAttributes(getter_AddRefs(attrs));
        NS_ENSURE_SUCCESS(rv, rv);
        
        // get "lang" attribute
        nsCOMPtr<nsIDOMNode> lang;
        rv = attrs->GetNamedItem(NS_LITERAL_STRING("lang"), getter_AddRefs(lang));
        NS_ENSURE_SUCCESS(rv, rv);
        
        // get its value
        rv = lang->GetNodeValue(language);
        NS_ENSURE_SUCCESS(rv, rv);
        
        // for every children, read article entry

        nsCOMPtr<nsIDOMNodeList> articleNodes;
        rv = childNode->GetChildNodes(getter_AddRefs(articleNodes));
        NS_ENSURE_SUCCESS(rv, rv);
        
        PRUint32 narticles;
        rv = articleNodes->GetLength(&narticles);
        NS_ENSURE_SUCCESS(rv, rv);
        
        for (PRUint32 j=0; j<narticles; j++) {

          // get article child node

          nsCOMPtr<nsIDOMNode> articleNode;
          rv = articleNodes->Item(j, getter_AddRefs(articleNode));
          NS_ENSURE_SUCCESS(rv, rv);
          
          PRUint16 articleNodeType;
          rv = articleNode->GetNodeType(&articleNodeType);
          NS_ENSURE_SUCCESS(rv, rv);
          
          // check that it's an article node
          if (nodeType == nsIDOMNode::ELEMENT_NODE) {
            nsString pattern;

            nsString articleLocalName;
            rv = articleNode->GetLocalName(articleLocalName);
            NS_ENSURE_SUCCESS(rv, rv);
            
            if (articleLocalName.EqualsLiteral(ARTICLE_DATA)) {
              nsString prefix;
              nsString suffix;
              
              // get attributes list
              nsCOMPtr<nsIDOMNamedNodeMap> artAttrs;
              rv = articleNode->GetAttributes(getter_AddRefs(artAttrs));
              NS_ENSURE_SUCCESS(rv, rv);
              
              // get "pattern" attribute
              nsCOMPtr<nsIDOMNode> patt;
              rv = artAttrs->GetNamedItem(NS_LITERAL_STRING("pattern"), getter_AddRefs(patt));
              NS_ENSURE_SUCCESS(rv, rv);
              
              // get its value
              rv = patt->GetNodeValue(pattern);
              NS_ENSURE_SUCCESS(rv, rv);
              
              // parse pattern
              rv = ParsePattern(pattern, prefix, suffix);
              NS_ENSURE_SUCCESS(rv, rv);
              
              // store it
              nsRefPtr<sbArticle> art =
                new sbArticle(prefix, suffix);
              NS_ENSURE_TRUE(art, NS_ERROR_OUT_OF_MEMORY);
              
              data.AppendElement(art);
            }
          }
        }

        nsRefPtr<sbArticleLanguageData> ldata =
          new sbArticleLanguageData(language, data);
        NS_ENSURE_TRUE(ldata, NS_ERROR_OUT_OF_MEMORY);

        mLanguages.AppendElement(ldata);
      }
      
    }
    
  }
  
  mArticlesLoaded = PR_TRUE;

  return NS_OK;
}

nsresult sbArticlesData::UnloadArticles() {
  if (!mArticlesLoaded)
    return NS_OK;
    
  nsresult rv = RemoveObserver();
  NS_ENSURE_SUCCESS(rv, rv);
    
  // free articles data
  mLanguages.Clear();
  
  mArticlesLoaded = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbArticlesDataObserver::Observe(nsISupports* aSubject,
                         const char* aTopic,
                         const PRUnichar* aData) {

  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID) == 0) {
    if (gArticlesData) {
      gArticlesData->UnloadArticles();
      delete gArticlesData;
      gArticlesData = NULL;
    }
  }

  return NS_OK;
}

nsresult sbArticlesData::EnsureLoaded() {
  if (gArticlesData) 
    return NS_OK;
  
  gArticlesData = new sbArticlesData();
  
  if (!gArticlesData)
    return NS_ERROR_OUT_OF_MEMORY;
    
  return gArticlesData->Init();
}

nsresult sbArticlesData::RemoveArticles(const nsAString & aInput,
                                        const nsAString & aLanguage,
                                        nsAString &_retval) {
  nsresult rv = EnsureLoaded();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return gArticlesData->_RemoveArticles(aInput, aLanguage, _retval);
}

nsresult sbArticlesData::ParsePattern(const nsAString &aPattern,
                                      nsAString &aPrefix,
                                      nsAString &aSuffix) {

  
  PRInt32 pos = aPattern.FindChar('*');
  if (pos == -1) 
    return NS_ERROR_ILLEGAL_VALUE;
    
  const nsAString& left = Substring(aPattern, 0, pos);
  const nsAString& right = Substring(aPattern, pos+1);
  
  aPrefix = left;
  aSuffix = right;

  return NS_OK;
}

nsresult sbArticlesData::_RemoveArticles(const nsAString & aInput,
                                         const nsAString & aLanguage,
                                         nsAString &_retval) {

  nsString val;
  val = aInput;
  PRUint32 nlang = mLanguages.Length();
  bool abort = PR_FALSE;
  for (PRUint32 i=0;i<nlang && !abort;i++) {
    sbArticleLanguageData *lang = mLanguages[i];
    if (aLanguage.IsEmpty() ||
        aLanguage.Equals(lang->mLanguage)) {
      PRUint32 narticles = lang->mArticles.Length();
      for (PRUint32 j=0;j<narticles;j++) {
        nsString newVal;
        
        nsresult rv = RemoveArticle(val, lang->mArticles[j], newVal);
        NS_ENSURE_SUCCESS(rv, rv);
        
        if (newVal.IsEmpty()) {
          abort = PR_TRUE;
          break;
        }
        val = newVal;
      }
    }
  }
  
  _retval = val;
  
  return NS_OK;
}

nsresult sbArticlesData::RemoveArticle(const nsAString & aInput,
                                       const sbArticle * aArticle,
                                       nsAString &_retval) {
  nsString val;
  val = aInput;
  
  if (!aArticle->mPrefix.IsEmpty()) {
    const nsAString &left = Substring(val, 0, aArticle->mPrefix.Length());
    if (left.Equals(aArticle->mPrefix, CaseInsensitiveCompare)) {
      val = Substring(val, left.Length());
    }
  }
  if (!aArticle->mSuffix.IsEmpty()) {
    const nsAString &right = Substring(val, val.Length() - aArticle->mSuffix.Length());
    if (right.Equals(aArticle->mSuffix, CaseInsensitiveCompare)) {
      val = Substring(val, 0, val.Length()-right.Length());
    }
  }
  
  _retval = val;
  
  return NS_OK;
}
