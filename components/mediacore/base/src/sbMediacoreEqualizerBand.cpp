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

/**
* \file  sbMediacoreEqualizerBand.cpp
* \brief Nightingale Mediacore Equalizer Band Implementation.
*/
#include "sbMediacoreEqualizerBand.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreEqualizerBand,
                              sbIMediacoreEqualizerBand);

sbMediacoreEqualizerBand::sbMediacoreEqualizerBand()
: mLock(nsnull)
, mIndex(0)
, mFrequency(0)
, mGain(0)
{
}

sbMediacoreEqualizerBand::~sbMediacoreEqualizerBand()
{
  if(mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

NS_IMETHODIMP
sbMediacoreEqualizerBand::Init(PRUint32 aIndex, PRUint32 aFrequency, double aGain)
{
  NS_ENSURE_TRUE(!mLock, NS_ERROR_ALREADY_INITIALIZED);

  mLock = nsAutoLock::NewLock("sbMediacoreEqualizerBand::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);

  mIndex = aIndex;
  mFrequency = aFrequency;
  mGain = aGain;

  return NS_OK;
}

/* attribute unsigned long index; */
NS_IMETHODIMP
sbMediacoreEqualizerBand::GetIndex(PRUint32 *aIndex)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aIndex);

  nsAutoLock lock(mLock);
  *aIndex = mIndex;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEqualizerBand::SetIndex(PRUint32 aIndex)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mIndex = aIndex;

  return NS_OK;
}

/* attribute unsigned long frequency; */
NS_IMETHODIMP 
sbMediacoreEqualizerBand::GetFrequency(PRUint32 *aFrequency)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aFrequency);

  nsAutoLock lock(mLock);
  *aFrequency = mFrequency;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreEqualizerBand::SetFrequency(PRUint32 aFrequency)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mFrequency = aFrequency;

  return NS_OK;
}

/* attribute double gain; */
NS_IMETHODIMP 
sbMediacoreEqualizerBand::GetGain(double *aGain)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  *aGain = mGain;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreEqualizerBand::SetGain(double aGain)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);
  mGain = aGain;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEqualizerBand::GetValues(PRUint32 *aIndex, PRUint32 *aFrequency, double *aGain)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  NS_ENSURE_ARG_POINTER(aIndex);
  NS_ENSURE_ARG_POINTER(aFrequency);
  NS_ENSURE_ARG_POINTER(aGain);

  *aIndex = mIndex;
  *aFrequency = mFrequency;
  *aGain = mGain;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEqualizerBand::SetValues(PRUint32 aIndex, PRUint32 aFrequency, double aGain)
{
  NS_ENSURE_TRUE(mLock, NS_ERROR_NOT_INITIALIZED);

  nsAutoLock lock(mLock);

  mIndex = aIndex;
  mFrequency = aFrequency;
  mGain = aGain;

  return NS_OK;
}
