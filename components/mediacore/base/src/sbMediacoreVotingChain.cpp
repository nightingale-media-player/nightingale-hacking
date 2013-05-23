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

/** 
* \file  sbMediacoreVotingChain.cpp
* \brief Songbird Mediacore Voting Chain Implementation.
*/
#include "sbMediacoreVotingChain.h"

#include <nsIMutableArray.h>
#include <mozilla/Mutex.h>

#include <nsComponentManagerUtils.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediacoreVotingChain:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediacoreVotingChain = nsnull;
#define TRACE(args) PR_LOG(gMediacoreVotingChain, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediacoreVotingChain, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreVotingChain, 
                              sbIMediacoreVotingChain)

sbMediacoreVotingChain::sbMediacoreVotingChain()
: mLock(nsnull)
{
  MOZ_COUNT_CTOR(sbMediacoreVotingChain);

#ifdef PR_LOGGING
  if (!gMediacoreVotingChain)
    gMediacoreVotingChain= PR_NewLogModule("sbMediacoreVotingChain");
#endif

  TRACE(("sbMediacoreVotingChain[0x%x] - Created", this));
}

sbMediacoreVotingChain::~sbMediacoreVotingChain()
{
  TRACE(("sbMediacoreVotingChain[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbMediacoreVotingChain);

  mResults.clear();
}

nsresult 
sbMediacoreVotingChain::Init()
{
  TRACE(("sbMediacoreVotingChain[0x%x] - Init", this));

  return NS_OK;
}

nsresult 
sbMediacoreVotingChain::AddVoteResult(PRUint32 aVoteResult, 
                                      sbIMediacore *aMediacore)
{
  TRACE(("sbMediacoreVotingChain[0x%x] - AddVoteResult", this));
  NS_ENSURE_ARG_POINTER(aMediacore);

  mozilla::MutexAutoLock lock(mLock);
  mResults[aVoteResult] = aMediacore;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreVotingChain::GetValid(PRBool *aValid) 
{
  TRACE(("sbMediacoreVotingChain[0x%x] - GetValid", this));
  NS_ENSURE_ARG_POINTER(aValid);

  mozilla::MutexAutoLock lock(mLock);
  *aValid = !mResults.empty();

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreVotingChain::GetMediacoreChain(nsIArray * *aMediacoreChain)
{
  TRACE(("sbMediacoreVotingChain[0x%x] - GetMediacoreChain", this));
  NS_ENSURE_ARG_POINTER(aMediacoreChain);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> mutableArray = 
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MutexAutoLock lock(mLock);

  votingmap_t::const_reverse_iterator cit = mResults.rbegin();
  votingmap_t::const_reverse_iterator endCit = mResults.rend();

  for(; cit != endCit; ++cit) {
    rv = mutableArray->AppendElement((*cit).second, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = CallQueryInterface(mutableArray, aMediacoreChain);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreVotingChain::GetVote(sbIMediacore *aMediacore, 
                                PRUint32 *_retval)
{
  TRACE(("sbMediacoreVotingChain[0x%x] - GetVote", this));
  NS_ENSURE_ARG_POINTER(aMediacore);
  NS_ENSURE_ARG_POINTER(_retval);

  mozilla::MutexAutoLock lock(mLock);

  votingmap_t::const_reverse_iterator cit = mResults.rbegin();
  votingmap_t::const_reverse_iterator endCit = mResults.rend();
  for(; cit != endCit; ++cit) {
    if((*cit).second == aMediacore) {
      *_retval = (*cit).first;
      return NS_OK;
    }
  }

  return NS_ERROR_NOT_AVAILABLE;
}
