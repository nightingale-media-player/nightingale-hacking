/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbMediacoreShuffleSequenceGenerator.h"

#include <nsIURI.h>
#include <nsIURL.h>

#include <nsAutoLock.h>
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

#include <sbTArrayStringEnumerator.h>

#include <ctime>
#include <vector>
#include <algorithm>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreShuffleSequenceGenerator, 
                              sbIMediacoreSequenceGenerator)

nsresult 
sbMediacoreShuffleSequenceGenerator::Init()
{
  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreShuffleSequenceGenerator::OnGenerateSequence(sbIMediaListView *aView, 
                                                        PRUint32 *aSequenceLength, 
                                                        PRUint32 **aSequence)
{
  NS_ENSURE_ARG_POINTER(aView);
  NS_ENSURE_ARG_POINTER(aSequenceLength);
  NS_ENSURE_ARG_POINTER(aSequence);

  *aSequenceLength = 0;
  *aSequence = nsnull;

  PRUint32 length = 0;
  nsresult rv = aView->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  // Reserve space for return array
  *aSequence = (PRUint32*)NS_Alloc(sizeof(PRUint32) * length);
  *aSequenceLength = length;

  // Reserve space for pool and sequence.
  std::vector<PRUint32> pool;
  pool.reserve(length);

  // Generate pool.
  for(PRUint32 current = 0; current < length; ++current) {
    pool.push_back(current);
  }

  // Seed.
  std::srand(std::clock());

  // Randomly sample the pool to populate the sequence.
  random_shuffle(pool.begin(), pool.end());

  // Copy into the return array
  copy(pool.begin(), pool.end(), *aSequence);
  
  return NS_OK;
}
