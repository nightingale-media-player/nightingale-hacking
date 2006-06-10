/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/**
* \file MetadataManager.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include <nspr.h>
#include "MetadataManager.h"

#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsIComponentRegistrar.h>
#include <xpcom/nsSupportsPrimitives.h>

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>

#include <necko/nsNetUtil.h>

#include <string/nsStringAPI.h>
#include <string/nsString.h>

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS1(sbMetadataManager, sbIMetadataManager)

//-----------------------------------------------------------------------------
sbMetadataManager::sbMetadataManager()
{
}

//-----------------------------------------------------------------------------
sbMetadataManager::~sbMetadataManager()
{
}

//-----------------------------------------------------------------------------
/* sbIMetadataHandler GetHandlerForMediaURL (in wstring strURL); */
NS_IMETHODIMP sbMetadataManager::GetHandlerForMediaURL(const PRUnichar *strURL, sbIMetadataHandler **_retval)
{
  *_retval = nsnull;
  nsresult nRet = NS_ERROR_UNEXPECTED;
  if(!_retval) return NS_ERROR_NULL_POINTER;

  sbIMetadataHandler *pHandler = nsnull;
  nsCOMPtr<nsIChannel> pChannel;
  nsCOMPtr<nsIIOService> pIOService = do_GetIOService(&nRet);
  if(NS_FAILED(nRet)) return nRet;

  NS_ConvertUTF16toUTF8 cstrURL(strURL);

  nsCOMPtr<nsIURI> pURI;
  pIOService->NewURI(cstrURL, nsnull, nsnull, getter_AddRefs(pURI));
  if(!pURI) return nRet;

  nsCString cstrScheme;
  nRet = pURI->GetScheme(cstrScheme);
  if(NS_FAILED(nRet)) return nRet;
  if(cstrScheme.Length() <= 1 || cstrURL[0] == (PRUnichar)'/' );
  {
    nsCString cstrFixedURL = NS_LITERAL_CSTRING("file://");
    cstrFixedURL += cstrURL;

    pIOService->NewURI(cstrFixedURL, nsnull, nsnull, getter_AddRefs(pURI));
    nRet = pURI->GetScheme(cstrScheme);
  }

  PRInt32 nHandlerRet = 0;
  nRet = pIOService->NewChannelFromURI(pURI, getter_AddRefs(pChannel));
  if(NS_FAILED(nRet)) return nRet;

  // Local types to ease handling.
  handlerlist_t handlerlist; // hooray for autosorting

  // Find a useful handler for this object.
  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  if (rv != NS_OK)
    return rv;

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  if (rv != NS_OK)
    return rv;

  nsCString u8Url;
  pURI->GetSpec( u8Url );
  nsString url = NS_ConvertUTF8toUTF16(u8Url);
  PRBool moreAvailable = PR_FALSE;

  while(simpleEnumerator->HasMoreElements(&moreAvailable) == NS_OK && moreAvailable)
  {
    nsCOMPtr<nsISupportsCString> contractString;
    if (simpleEnumerator->GetNext(getter_AddRefs(contractString)) == NS_OK)
    {
      char* contractID = NULL;
      contractString->ToString(&contractID);
      if (strstr(contractID, "@songbird.org/Songbird/MetadataHandler/"))
      {
        nsCOMPtr<sbIMetadataHandler> handler(do_CreateInstance(contractID));
        if (handler.get())
        {
          PRInt32 vote;
          handler->Vote( url.get(), &vote );
          sbMetadataHandlerItem item;
          item.m_Handler = handler;
          item.m_Vote = vote;
          handlerlist.insert( item );
        }
      }
      PR_Free(contractID);
    }
  }

  if ( handlerlist.rbegin() != handlerlist.rend() )
  {
    handlerlist_t::reverse_iterator i = handlerlist.rbegin();
    pHandler = (*i).m_Handler.get();
  }

  if(!pHandler)
  {
    nRet = NS_ERROR_UNEXPECTED;
    return nRet;
  }

  nRet = pHandler->SetChannel(pChannel, &nHandlerRet);
  if(nRet != 0) return nRet;

  pHandler->AddRef();
  pHandler->AddRef();
  *_retval = pHandler;
  //(*_retval)->AddRef();

  nRet = NS_OK;

  return nRet;
}
