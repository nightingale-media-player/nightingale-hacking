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
 * \file  sbMediacoreEqualizerBand.h
 * \brief Nightingale Mediacore Equalizer Band Definition.
 */

#ifndef __SB_MEDIACOREEQUALIZERBAND_H__
#define __SB_MEDIACOREEQUALIZERBAND_H__

#include <sbIMediacoreMultibandEqualizer.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>

class sbMediacoreEqualizerBand : public sbIMediacoreEqualizerBand
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREEQUALIZERBAND

  sbMediacoreEqualizerBand();

protected:
  virtual ~sbMediacoreEqualizerBand();

  PRLock* mLock;
  
  PRUint32  mIndex;
  PRUint32  mFrequency;
  PRFloat64 mGain;
};

#endif /* __SB_MEDIACOREEQUALIZERBAND_H__ */
