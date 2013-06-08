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
* \file  sbBaseMediacoreVolumeControl.cpp
* \brief Songbird Base Mediacore Volume Control Implementation.
*/
#include "sbBaseMediacoreVolumeControl.h"

#include <prprf.h>
#include <mozilla/Monitor.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediacoreVolumeControl:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseMediacoreVolumeControl = nsnull;
#define TRACE(args) PR_LOG(gBaseMediacoreVolumeControl , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseMediacoreVolumeControl , PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

void SB_ConvertFloatVolToJSStringValue(PRFloat64 aVol, 
                                       nsACString &aStrVol)
{
  char volume[64] = {0};
  PR_snprintf(volume, 64, "%lg", aVol);

  // We have to replace the decimal point character with '.' so that
  // parseFloat in JS still understands that this number is a floating point
  // number. The JS Standard dictates that parseFloat _ONLY_ supports '.' as
  // it's decimal point character.
  volume[1] = '.';

  aStrVol.Assign(volume);

  return;
}

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBaseMediacoreVolumeControl, 
                              sbIMediacoreVolumeControl)

sbBaseMediacoreVolumeControl::sbBaseMediacoreVolumeControl()
: mMonitor(nsnull)
, mMute(PR_FALSE)
, mVolume(0)
{
  MOZ_COUNT_CTOR(sbBaseMediacoreVolumeControl);

#ifdef PR_LOGGING
  if (!gBaseMediacoreVolumeControl)
    gBaseMediacoreVolumeControl = PR_NewLogModule("sbBaseMediacoreVolumeControl");
#endif

  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - Created", this));
}

sbBaseMediacoreVolumeControl::~sbBaseMediacoreVolumeControl()
{
  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - Destroyed", this));

  MOZ_COUNT_DTOR(sbBaseMediacoreVolumeControl);
}

nsresult 
sbBaseMediacoreVolumeControl::InitBaseMediacoreVolumeControl()
{
  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - InitBaseMediacoreVolumeControl", this));

  return OnInitBaseMediacoreVolumeControl();
}

NS_IMETHODIMP 
sbBaseMediacoreVolumeControl::GetMute(PRBool *aMute)
{
  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - GetMute", this));
  NS_ENSURE_ARG_POINTER(aMute);

  mozilla::MonitorAutoLock mon(mMonitor);
  *aMute = mMute;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacoreVolumeControl::SetMute(PRBool aMute)
{
  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - SetMute", this));

  nsresult rv = OnSetMute(aMute);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MonitorAutoLock mon(mMonitor);
  mMute = aMute;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacoreVolumeControl::GetVolume(double *aVolume)
{
  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - GetVolume", this));
  NS_ENSURE_ARG_POINTER(aVolume);

  mozilla::MonitorAutoLock mon(mMonitor);
  *aVolume = mVolume;

  return NS_OK;
}

NS_IMETHODIMP 
sbBaseMediacoreVolumeControl::SetVolume(double aVolume)
{
  TRACE(("sbBaseMediacoreVolumeControl[0x%x] - SetVolume", this));

  nsresult rv = OnSetVolume(aVolume);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MonitorAutoLock mon(mMonitor);
  mVolume = aVolume;

  return NS_OK;
}

/*virtual*/ nsresult 
sbBaseMediacoreVolumeControl::OnInitBaseMediacoreVolumeControl()
{
  /**
   *  This is where you may want to set what the current volume is
   *  so that the value is initialized properly.
   */  

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult 
sbBaseMediacoreVolumeControl::OnSetMute(PRBool aMute)
{
  /**
   *  This is where you would set the actual mute flag on the mediacore 
   *  implementation. 
   *
   *  The value is always cached for you if this method succeeds. If it fails
   *  the current value remains.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}
  
/*virtual*/ nsresult 
sbBaseMediacoreVolumeControl::OnSetVolume(double aVolume)
{
  /**
   *  This is where you would set the actual volume on the mediacore 
   *  implementation. 
   *
   *  The value is always cached for you if this method succeeds. If it fails
   *  the current value remains.
   */

  return NS_ERROR_NOT_IMPLEMENTED;
}
