/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbTranscodeManager.h"

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsAutoPtr.h>

#include <nsServiceManagerUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIComponentRegistrar.h>
#include <nsISupportsPrimitives.h>

#include <nsStringGlue.h>
#include <nsISimpleEnumerator.h>
#include <nsArrayUtils.h>

#include <sbITranscodingConfigurator.h>
#include <sbITranscodeVideoJob.h>

/* Global transcode manager (singleton) */
sbTranscodeManager *gTranscodeManager = nsnull;

static const char TranscodeContractIDPrefix[] =
    "@getnightingale.com/Nightingale/Mediacore/Transcode/";

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTranscodeManager, sbITranscodeManager)

sbTranscodeManager::sbTranscodeManager()
{
  m_pContractListLock = PR_NewLock();
  NS_ASSERTION(m_pContractListLock,
          "Failed to create sbTranscodeManager::m_pContractListLock! "
          "Object *not* threadsafe!");

  // Find the list of handlers for this object.
  nsresult rv;
  nsCOMPtr<nsIComponentRegistrar> registrar;
  rv = NS_GetComponentRegistrar(getter_AddRefs(registrar));
  NS_ENSURE_SUCCESS (rv, /* void */);

  nsCOMPtr<nsISimpleEnumerator> simpleEnumerator;
  rv = registrar->EnumerateContractIDs(getter_AddRefs(simpleEnumerator));
  NS_ENSURE_SUCCESS (rv, /* void */);

  PRBool moreAvailable = PR_FALSE;
  while(simpleEnumerator->HasMoreElements(&moreAvailable) == NS_OK &&
        moreAvailable)
  {
    nsCOMPtr<nsISupportsCString> contractString;
    rv = simpleEnumerator->GetNext(getter_AddRefs(contractString));
    if (NS_SUCCEEDED (rv))
    {
      nsCString contractID;
      contractString->GetData(contractID);
      if (contractID.Find(TranscodeContractIDPrefix) != -1)
      {
        m_ContractList.push_back(contractID);
      }
    }
  }
}

//-----------------------------------------------------------------------------
sbTranscodeManager::~sbTranscodeManager()
{
  if(m_pContractListLock)
  {
    PR_DestroyLock(m_pContractListLock);
    m_pContractListLock = nsnull;
  }
}

//-----------------------------------------------------------------------------
sbTranscodeManager *sbTranscodeManager::GetSingleton()
{
  if (gTranscodeManager) {
    NS_ADDREF(gTranscodeManager);
    return gTranscodeManager;
  }

  NS_NEWXPCOM(gTranscodeManager, sbTranscodeManager);
  if (!gTranscodeManager)
    return nsnull;

  NS_ADDREF(gTranscodeManager);
  NS_ADDREF(gTranscodeManager);

  return gTranscodeManager;
}

//-----------------------------------------------------------------------------
void sbTranscodeManager::DestroySingleton()
{
  nsRefPtr<sbTranscodeManager> dummy;
  dummy.swap(gTranscodeManager);
}

NS_IMETHODIMP
sbTranscodeManager::GetTranscoderForMediaItem(sbIMediaItem *aMediaItem,
                                              nsISupports **_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  int bestVote = -1;
  nsCOMPtr<nsISupports> bestHandler;

  for (contractlist_t::iterator contractid = m_ContractList.begin();
       contractid != m_ContractList.end();
       ++contractid )
  {
    nsCOMPtr<nsISupports> supports =
      do_CreateInstance((*contractid).get(), &rv);
    if (NS_FAILED(rv) || !supports)
    {
      continue;
    }

    nsCOMPtr<sbITranscodeVideoJob> videoJob = do_QueryInterface(supports, &rv);
    if (NS_SUCCEEDED(rv) && videoJob)
    {
      // new, video-style transcoder
      PRInt32 vote;
      rv = videoJob->Vote(aMediaItem, &vote);
      if (NS_FAILED(rv)) {
        continue;
      }

      /* Negative values are not ever permissible */
      if (vote >= bestVote) {
        bestVote = vote;
        bestHandler = supports;
      }
    }
  }

  if (bestHandler) {
    NS_ADDREF(*_retval = bestHandler);
    return NS_OK;
  }
  else
    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbTranscodeManager::GetTranscodeProfiles(PRUint32 aType, nsIArray **_retval)
{
  nsresult rv;
  nsCOMPtr<nsIMutableArray> array =
      do_CreateInstance("@getnightingale.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (contractlist_t::iterator contractid = m_ContractList.begin();
       contractid != m_ContractList.end();
       ++contractid )
  {
    nsCOMPtr<sbITranscodingConfigurator> handler = do_CreateInstance(
            (*contractid).get(), &rv);

    if(NS_SUCCEEDED(rv) && handler)
    {
      nsCOMPtr<nsIArray> profiles;
      PRUint32 length;

      rv = handler->GetAvailableProfiles(getter_AddRefs(profiles));
      NS_ENSURE_SUCCESS (rv, rv);

      rv = profiles->GetLength(&length);
      NS_ENSURE_SUCCESS (rv, rv);

      for (PRUint32 i = 0; i < length; i++) {
        nsCOMPtr<sbITranscodeProfile> profile =
            do_QueryElementAt(profiles, i, &rv);
        NS_ENSURE_SUCCESS (rv, rv);

        PRUint32 profileType;
        rv = profile->GetType(&profileType);
        NS_ENSURE_SUCCESS (rv, rv);

        if (profileType == aType) {
          rv = array->AppendElement(profile, PR_FALSE);
          NS_ENSURE_SUCCESS (rv, rv);
        }
      }
    }
  }

  nsCOMPtr<nsIArray> arr = do_QueryInterface(array, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  arr.forget(_retval);

  return NS_OK;
}

