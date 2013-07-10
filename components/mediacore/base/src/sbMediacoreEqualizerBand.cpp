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

/**
* \file  sbMediacoreEqualizerBand.cpp
* \brief Songbird Mediacore Equalizer Band Implementation.
*/
#include "sbMediacoreEqualizerBand.h"
#include <mozilla/Mutex.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediacoreEqualizerBand,
                              sbIMediacoreEqualizerBand);

sbMediacoreEqualizerBand::sbMediacoreEqualizerBand()
: mLock("sbMediacoreEqualizerBand::mLock")
, mIndex(0)
, mFrequency(0)
, mGain(0)
{
}

sbMediacoreEqualizerBand::~sbMediacoreEqualizerBand()
{

}

NS_IMETHODIMP
sbMediacoreEqualizerBand::Init(PRUint32 aIndex, PRUint32 aFrequency, double aGain)
{
  mozilla::MutexAutoLock lock(mLock);

  mIndex = aIndex;
  mFrequency = aFrequency;
  mGain = aGain;

  return NS_OK;
}

/* attribute unsigned long index; */
NS_IMETHODIMP
sbMediacoreEqualizerBand::GetIndex(PRUint32 *aIndex)
{
  NS_ENSURE_ARG_POINTER(aIndex);

  mozilla::MutexAutoLock lock(mLock);
  *aIndex = mIndex;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEqualizerBand::SetIndex(PRUint32 aIndex)
{
  mozilla::MutexAutoLock lock(mLock);
  mIndex = aIndex;

  return NS_OK;
}

/* attribute unsigned long frequency; */
NS_IMETHODIMP 
sbMediacoreEqualizerBand::GetFrequency(PRUint32 *aFrequency)
{
  NS_ENSURE_ARG_POINTER(aFrequency);

  mozilla::MutexAutoLock lock(mLock);
  *aFrequency = mFrequency;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreEqualizerBand::SetFrequency(PRUint32 aFrequency)
{
  mozilla::MutexAutoLock lock(mLock);
  mFrequency = aFrequency;

  return NS_OK;
}

/* attribute double gain; */
NS_IMETHODIMP 
sbMediacoreEqualizerBand::GetGain(double *aGain)
{
  mozilla::MutexAutoLock lock(mLock);
  *aGain = mGain;

  return NS_OK;
}

NS_IMETHODIMP 
sbMediacoreEqualizerBand::SetGain(double aGain)
{
  mozilla::MutexAutoLock lock(mLock);
  mGain = aGain;

  return NS_OK;
}

NS_IMETHODIMP
sbMediacoreEqualizerBand::GetValues(PRUint32 *aIndex, PRUint32 *aFrequency, double *aGain)
{
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
  mozilla::MutexAutoLock lock(mLock);

  mIndex = aIndex;
  mFrequency = aFrequency;
  mGain = aGain;

  return NS_OK;
}
