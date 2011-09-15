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
* \file  sbBaseMediacoreMultibandEqualizer.cpp
* \brief Songbird Mediacore Event Implementation.
*/
#include "sbBaseMediacoreMultibandEqualizer.h"

#include <nsIMutableArray.h>
#include <nsISimpleEnumerator.h>

#include <nsAutoPtr.h>
#include <nsComponentManagerUtils.h>

#include <prlog.h>
#include <prprf.h>

#include "sbMediacoreEqualizerBand.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediacoreMultibandEqualizer:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo * BaseMediacoreMultibandEqualizer() {
  static PRLogModuleInfo* gBaseMediacoreMultibandEqualizer = 
    PR_NewLogModule("sbBaseMediacoreMultibandEqualizer");
  return gBaseMediacoreMultibandEqualizer;
}

#define TRACE(args) PR_LOG(BaseMediacoreMultibandEqualizer() , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(BaseMediacoreMultibandEqualizer() , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

double
SB_ClampDouble(double aGain, double aMin, double aMax)
{
  double gain = aGain;

  if(aGain > aMax) {
    gain = aMax;
  }
  else if(aGain < aMin) {
    gain = aMin;
  }

  return gain;
}

void 
SB_ConvertFloatEqGainToJSStringValue(double aGain, nsCString &aGainStr)
{
  char gain[64] = {0};
  PR_snprintf(gain, 64, "%lg", SB_ClampDouble(aGain, -1.0, 1.0));

  // We have to replace the decimal point character with '.' so that
  // parseFloat in JS still understands that this number is a floating point
  // number. The JS Standard dictates that parseFloat _ONLY_ supports '.' as
  // it's decimal point character.
  gain[1] = '.';

  aGainStr.Assign(gain);

  return;
}

/*static*/ const PRUint32
sbBaseMediacoreMultibandEqualizer::EQUALIZER_BAND_COUNT_DEFAULT = 10;

/*static*/ const PRUint32 
sbBaseMediacoreMultibandEqualizer::EQUALIZER_BANDS_10[10] = 
  {32, 64, 125, 250, 500, 1000, 2000, 4000, 8000, 16000};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacoreMultibandEqualizer,
                              sbIMediacoreMultibandEqualizer)

sbBaseMediacoreMultibandEqualizer::sbBaseMediacoreMultibandEqualizer()
: mMonitor(nsnull)
, mEqEnabled(PR_FALSE)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - Created", this));
}

sbBaseMediacoreMultibandEqualizer::~sbBaseMediacoreMultibandEqualizer()
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - Destroyed", this));

  if(mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }

  if (mBands.IsInitialized()) {
    mBands.Clear();
  }
}

nsresult 
sbBaseMediacoreMultibandEqualizer::EnsureBandIsCached(sbIMediacoreEqualizerBand *aBand)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - EnsureBandIsCached", this));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mBands.IsInitialized(), NS_ERROR_NOT_INITIALIZED);

  PRUint32 bandIndex = 0;
  nsresult rv = aBand->GetIndex(&bandIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mBands.Get(bandIndex, nsnull);
  if(!success) {
    success = mBands.Put(bandIndex, aBand);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    PRUint32 bandFrequency = 0;
    rv = aBand->GetFrequency(&bandFrequency);
    NS_ENSURE_SUCCESS(rv, rv);

    PRFloat64 bandGain = 0;
    rv = aBand->GetGain(&bandGain);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediacoreEqualizerBand> band;
    mBands.Get(bandIndex, getter_AddRefs(band));
    
    rv = band->SetFrequency(bandFrequency);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = band->SetGain(bandGain);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbBaseMediacoreMultibandEqualizer::InitBaseMediacoreMultibandEqualizer()
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - InitBaseMediacoreMultibandEqualizer", this));

  mMonitor = nsAutoMonitor::NewMonitor("sbBaseMediacoreMultibandEqualizer::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  PRBool success = mBands.Init(10);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = OnInitBaseMediacoreMultibandEqualizer();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult 
sbBaseMediacoreMultibandEqualizer::OnInitBaseMediacoreMultibandEqualizer()
{
  /*
   * Here is where you would initialize any elements
   * that are required for your multiband equalizer.
   */
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseMediacoreMultibandEqualizer::OnSetEqEnabled(PRBool aEqEnabled)
{
  /*
   * Here is where you would enable / disable the equalizer
   * based on the value of aEqEnabled.
   */ 

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbBaseMediacoreMultibandEqualizer::OnGetBandCount(PRUint32 *aBandCount)
{
  /*
   * Here is where you would return the number of bands supported
   * by your equalizer.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacoreMultibandEqualizer::OnGetBand(PRUint32 aBandIndex, 
                                             sbIMediacoreEqualizerBand *aBand)
{
  /*
   * Here is where you would fill out the current values for a band.
   * The aBand object just needs to be initialized in this case
   * by calling Init() with the correct frequency band and gain values.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacoreMultibandEqualizer::OnSetBand(sbIMediacoreEqualizerBand *aBand)
{
  /*
   * Here is where you would apply new values for a band.
   * The aBand object will contain the frequency band and gain value
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}

//
// sbIMediacoreMultibandEqualizer
//
/* attribute boolean eqEnabled; */
NS_IMETHODIMP
sbBaseMediacoreMultibandEqualizer::GetEqEnabled(PRBool *aEqEnabled)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - GetEqEnabled", this));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aEqEnabled);

  nsAutoMonitor mon(mMonitor);
  *aEqEnabled = mEqEnabled;
  
  return NS_OK;
}

NS_IMETHODIMP
sbBaseMediacoreMultibandEqualizer::SetEqEnabled(PRBool aEqEnabled)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - SetEqEnabled", this));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);

  nsAutoMonitor mon(mMonitor);
  
  nsresult rv = OnSetEqEnabled(aEqEnabled);
  NS_ENSURE_SUCCESS(rv, rv);

  mEqEnabled = aEqEnabled;

  return NS_OK;
}

template<class T>
PLDHashOperator AppendElementToArray(T* aData, void* aArray)
{
  nsIMutableArray *array = (nsIMutableArray*)aArray;
  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aData, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = array->AppendElement(aData, false);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

template<class T>
PLDHashOperator 
sbBaseMediacoreMultibandEqualizer::EnumerateIntoArrayUint32Key(
                                      const PRUint32 &aKey,
                                      T* aData,
                                      void* aArray)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - EnumerateIntoArray (PRUint32)"));
  return AppendElementToArray(aData, aArray);
}

/* attribute nsISimpleEnumerator bands; */
NS_IMETHODIMP 
sbBaseMediacoreMultibandEqualizer::GetBands(nsISimpleEnumerator * *aBands)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - GetBands", this));

  NS_ENSURE_TRUE(mBands.IsInitialized(), NS_ERROR_NOT_INITIALIZED);

  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);
  
  nsCOMPtr<nsIMutableArray> mutableArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mBands.EnumerateRead(
    sbBaseMediacoreMultibandEqualizer::EnumerateIntoArrayUint32Key,
    mutableArray.get());

  mon.Exit();
  
  rv = mutableArray->Enumerate(aBands);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbBaseMediacoreMultibandEqualizer::SetBands(nsISimpleEnumerator *aBands)
{
  NS_ENSURE_ARG_POINTER(aBands);

  nsresult rv = NS_ERROR_UNEXPECTED;
  PRBool hasMore = PR_FALSE;

  nsCOMPtr<nsISupports> element;
  while(NS_SUCCEEDED(aBands->HasMoreElements(&hasMore)) &&
        hasMore &&
        NS_SUCCEEDED(aBands->GetNext(getter_AddRefs(element)))) {
    
    nsCOMPtr<sbIMediacoreEqualizerBand> band = 
      do_QueryInterface(element, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetBand(band);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* readonly attribute unsigned long bandCount; */
NS_IMETHODIMP 
sbBaseMediacoreMultibandEqualizer::GetBandCount(PRUint32 *aBandCount)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - GetBandCount", this));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aBandCount);

  nsAutoMonitor mon(mMonitor);

  nsresult rv = OnGetBandCount(aBandCount);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* sbIMediacoreEqualizerBand getBand (in unsigned long aBandIndex); */
NS_IMETHODIMP 
sbBaseMediacoreMultibandEqualizer::GetBand(PRUint32 aBandIndex, sbIMediacoreEqualizerBand **_retval)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - GetBand", this));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mBands.IsInitialized(), NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIMediacoreEqualizerBand> band;

  nsAutoMonitor mon(mMonitor);
    
  if(!mBands.Get(aBandIndex, getter_AddRefs(band))) {
    nsRefPtr<sbMediacoreEqualizerBand> newBand;
    NS_NEWXPCOM(newBand, sbMediacoreEqualizerBand);
    NS_ENSURE_TRUE(newBand, NS_ERROR_OUT_OF_MEMORY);

    rv = OnGetBand(aBandIndex, newBand);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = EnsureBandIsCached(newBand);
    NS_ENSURE_SUCCESS(rv, rv);
    
    NS_ADDREF(*_retval = newBand);
    
    return NS_OK;
  }

  band.forget(_retval);

  return NS_OK;
}

/* void setBand (in sbIMediacoreEqualizerBand aBand); */
NS_IMETHODIMP 
sbBaseMediacoreMultibandEqualizer::SetBand(sbIMediacoreEqualizerBand *aBand)
{
  TRACE(("sbBaseMediacoreMultibandEqualizer[0x%x] - SetBand", this));

  NS_ENSURE_TRUE(mMonitor, NS_ERROR_NOT_INITIALIZED);
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsAutoMonitor mon(mMonitor);

  if(mEqEnabled) {
    rv = OnSetBand(aBand);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = EnsureBandIsCached(aBand);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
