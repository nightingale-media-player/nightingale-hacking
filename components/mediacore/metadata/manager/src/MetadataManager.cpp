/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
#include <xpcom/nsAutoPtr.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsIComponentRegistrar.h>
#include <xpcom/nsISupportsPrimitives.h>

#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <necko/nsIIOService.h>

#include <necko/nsNetUtil.h>

#include <nsStringGlue.h>

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
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsRefPtr<sbIMetadataHandler> pHandler;
  nsCOMPtr<nsIChannel> pChannel;
  nsCOMPtr<nsIIOService> pIOService = do_GetIOService(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ConvertUTF16toUTF8 cstrURL(strURL);

  nsCOMPtr<nsIURI> pURI;
  rv = pIOService->NewURI(cstrURL, nsnull, nsnull, getter_AddRefs(pURI));
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // Apparently, somewhere in here, it fails for local mp3 files on linux and mac.
  //

  nsCString cstrScheme;
  rv = pURI->GetScheme(cstrScheme);
  NS_ENSURE_SUCCESS(rv, rv);

  if(cstrScheme.Length() <= 1)
  {
    nsCString cstrFixedURL = NS_LITERAL_CSTRING("file://");
    cstrFixedURL += cstrURL;

    pIOService->NewURI(cstrFixedURL, nsnull, nsnull, getter_AddRefs(pURI));
    rv = pURI->GetScheme(cstrScheme);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = pIOService->NewChannelFromURI(pURI, getter_AddRefs(pChannel));
  NS_ENSURE_SUCCESS(rv, rv);

  // Local types to ease handling.
  handlerlist_t handlerlist; // hooray for autosorting (std::set)

  nsCString u8Url;
  rv = pURI->GetSpec( u8Url );
  NS_ENSURE_SUCCESS(rv, rv);

  nsString url = NS_ConvertUTF8toUTF16(u8Url);
  if (!m_ContractList.size())
    throw;

  // Go through the list of contract ids, and make them vote on the url
  for (contractlist_t::iterator i = m_ContractList.begin(); i != m_ContractList.end(); i++ )
  {
    nsCOMPtr<sbIMetadataHandler> handler = do_CreateInstance((*i).get(), &rv);
    if(NS_SUCCEEDED(rv) && handler.get())
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

  NS_ENSURE_TRUE(pHandler, NS_ERROR_UNEXPECTED);

  // So, if we have anything, set it up and send it back.
  rv = pHandler->SetChannel(pChannel);
  NS_ENSURE_SUCCESS(rv, rv);

  pHandler.swap(*_retval);

  return NS_OK;
}
