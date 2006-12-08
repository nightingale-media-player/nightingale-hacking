/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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

/**
* \file MetadataManager.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include <nspr.h>
#include "MetadataManager.h"

#include <xpcom/nsAutoLock.h>
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

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#endif

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// GLOBALS ====================================================================
sbMetadataManager *gMetadataManager = nsnull;

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataManager, sbIMetadataManager)

//-----------------------------------------------------------------------------
sbMetadataManager::sbMetadataManager()
: m_pContractListLock(nsnull)
{
  m_pContractListLock = PR_NewLock();
  NS_ASSERTION(m_pContractListLock, "Failed to create sbMetadataManager::m_pContractListLock! Object *not* threadsafe!");

  // Find the list of handlers for this object.
  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  if (rv != NS_OK)
    return;

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  if (rv != NS_OK)
    return;

  PRBool moreAvailable = PR_FALSE;
  while(simpleEnumerator->HasMoreElements(&moreAvailable) == NS_OK && moreAvailable)
  {
    nsCOMPtr<nsISupportsCString> contractString;
    if (simpleEnumerator->GetNext(getter_AddRefs(contractString)) == NS_OK)
    {
      nsCString contractID;
      contractString->GetData(contractID);
      if (contractID.Find("@songbirdnest.com/Songbird/MetadataHandler/") != -1)
      {
        m_ContractList.push_back(contractID);
      }
    }
  }
}

//-----------------------------------------------------------------------------
sbMetadataManager::~sbMetadataManager()
{
  if(m_pContractListLock)
  {
    PR_DestroyLock(m_pContractListLock);
    m_pContractListLock = nsnull;
  }
}

//-----------------------------------------------------------------------------
sbMetadataManager *sbMetadataManager::GetSingleton()
{
  if (gMetadataManager) {
    NS_ADDREF(gMetadataManager);
    return gMetadataManager;
  }

  NS_NEWXPCOM(gMetadataManager, sbMetadataManager);
  if (!gMetadataManager)
    return nsnull;

  NS_ADDREF(gMetadataManager);
  NS_ADDREF(gMetadataManager);

  return gMetadataManager;
}

//-----------------------------------------------------------------------------
/* sbIMetadataHandler GetHandlerForMediaURL (in wstring strURL); */
NS_IMETHODIMP sbMetadataManager::GetHandlerForMediaURL(const nsAString &strURL, sbIMetadataHandler **_retval)
{
  nsAutoLock lock(m_pContractListLock);

  if(!_retval) return NS_ERROR_NULL_POINTER;
  *_retval = nsnull;
  nsresult nRet = NS_ERROR_UNEXPECTED;

  sbIMetadataHandler *pHandler = nsnull;
  nsCOMPtr<nsIChannel> pChannel;
  nsCOMPtr<nsIIOService> pIOService = do_GetIOService(&nRet);
  if(NS_FAILED(nRet)) return nRet;

  NS_ConvertUTF16toUTF8 cstrURL(strURL);

  nsCOMPtr<nsIURI> pURI;
  pIOService->NewURI(cstrURL, nsnull, nsnull, getter_AddRefs(pURI));
  if(!pURI) return nRet;

  //
  // Apparently, somewhere in here, it fails for local mp3 files on linux and mac.
  //

  nsCString cstrScheme;
  nRet = pURI->GetScheme(cstrScheme);
  if(NS_FAILED(nRet)) return nRet;
  if(cstrScheme.Length() <= 1)
  {
    nsCString cstrFixedURL = NS_LITERAL_CSTRING("file://");
    cstrFixedURL += cstrURL;

    pIOService->NewURI(cstrFixedURL, nsnull, nsnull, getter_AddRefs(pURI));
    nRet = pURI->GetScheme(cstrScheme);
  }

  nRet = pIOService->NewChannelFromURI(pURI, getter_AddRefs(pChannel));
  if(NS_FAILED(nRet)) return nRet;

  // Local types to ease handling.
  handlerlist_t handlerlist; // hooray for autosorting (std::set)

  nsCString u8Url;
  pURI->GetSpec( u8Url );
  nsString url = NS_ConvertUTF8toUTF16(u8Url);

  if (!m_ContractList.size())
    throw;

  // Go through the list of contract ids, and make them vote on the url
  for (contractlist_t::iterator i = m_ContractList.begin(); i != m_ContractList.end(); i++ )
  {
    nsCOMPtr<sbIMetadataHandler> handler(do_CreateInstance((*i).get()));
    if (handler.get())
    {
      PRInt32 vote;
      handler->Vote( url, &vote );
      if (vote >= 0) // If everyone returns -1, give up.
      {
        sbMetadataHandlerItem item;
        item.m_Handler = handler;
        item.m_Vote = vote;
        handlerlist.insert( item ); // Sorted list (std::set)
      }
    }
  }

  // If there's anything in the list
  if ( handlerlist.rbegin() != handlerlist.rend() )
  {
    // The end of the list had the highest vote
    handlerlist_t::reverse_iterator i = handlerlist.rbegin();
    pHandler = (*i).m_Handler.get();
  }

  if(!pHandler)
  {
    nRet = NS_ERROR_UNEXPECTED;
    return nRet;
  }

  // So, if we have anything, set it up and send it back.
  nRet = pHandler->SetChannel(pChannel);
  if(nRet != 0) return nRet;

  pHandler->AddRef();
  *_retval = pHandler;

  nRet = NS_OK;

  return nRet;
}
