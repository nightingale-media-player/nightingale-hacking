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
 * \file  sbBaseMediacoreMultibandEqualizer.h
 * \brief Songbird Mediacore Base Multiband Equalizer Definition.
 */

#ifndef __SB_BASEMEDIACOREMULTIBANDEQUALIZER_H__
#define __SB_BASEMEDIACOREMULTIBANDEQUALIZER_H__

#include <sbIMediacoreMultibandEqualizer.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>

double
SB_ClampDouble(double aGain, double aMin, double aMax);

void
SB_ConvertFloatEqGainToJSStringValue(double aGain, nsCString &aGainStr);

class sbBaseMediacoreMultibandEqualizer : public sbIMediacoreMultibandEqualizer
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREMULTIBANDEQUALIZER

  sbBaseMediacoreMultibandEqualizer();

  nsresult InitBaseMediacoreMultibandEqualizer();

  /**
   * \brief Suggested default band count for the equalizer.
   *        This value is defined as 10.
   */
  static const PRUint32 EQUALIZER_BAND_COUNT_DEFAULT;
  /**
   * \brief Suggested bands for a 10 band equalizer.
   */
  static const PRUint32 EQUALIZER_BANDS_10[10];
  
  /* override me, see cpp file for implementation notes */
  virtual nsresult OnInitBaseMediacoreMultibandEqualizer();
  /* override me, see cpp file for implementation notes */
  virtual nsresult OnSetEqEnabled(PRBool aEqEnabled);
  /* override me, see cpp file for implementation notes */
  virtual nsresult OnGetBandCount(PRUint32 *aBandCount);
  /* override me, see cpp file for implementation notes */
  virtual nsresult OnGetBand(PRUint32 aBandIndex, sbIMediacoreEqualizerBand *aBand);
  /* override me, see cpp file for implementation notes */
  virtual nsresult OnSetBand(sbIMediacoreEqualizerBand *aBand);

protected:
  virtual ~sbBaseMediacoreMultibandEqualizer();

  template<class T>
  static NS_HIDDEN_(PLDHashOperator)
    EnumerateIntoArrayUint32Key(const PRUint32 &aKey,
                                T* aData,
                                void* aArray);

  nsresult EnsureBandIsCached(sbIMediacoreEqualizerBand *aBand);

  PRMonitor*    mMonitor;
  PRPackedBool  mEqEnabled;

  typedef nsInterfaceHashtable<nsUint32HashKey, sbIMediacoreEqualizerBand> eqbands_t;
  eqbands_t mBands;
};

#endif /* __SB_BASEMEDIACOREMULTIBANDEQUALIZER_H__ */
