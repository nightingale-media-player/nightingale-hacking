/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbDeviceFirmwareUpdate.h"

#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>

#include <mozilla/Monitor.h>
#include <nsMemory.h>


NS_IMPL_CLASSINFO(sbDeviceFirmwareUpdate, NULL,
                  nsIClassInfo::THREADSAFE, SB_DEVICEFIRMWAREUPDATE_CID);

NS_IMPL_ISUPPORTS2(sbDeviceFirmwareUpdate,
                   sbIDeviceFirmwareUpdate,
                   nsIClassInfo);

NS_IMPL_CI_INTERFACE_GETTER1(sbDeviceFirmwareUpdate,
                             sbIDeviceFirmwareUpdate);

NS_IMPL_THREADSAFE_CI(sbDeviceFirmwareUpdate);

sbDeviceFirmwareUpdate::sbDeviceFirmwareUpdate()
: mMonitor(nsnull)
, mFirmwareReadableVersion(NS_LITERAL_STRING("0"))
, mFirmwareVersion(0)
{

}

sbDeviceFirmwareUpdate::~sbDeviceFirmwareUpdate()
{

}

NS_IMETHODIMP 
sbDeviceFirmwareUpdate::GetFirmwareImageFile(nsIFile * *aFirmwareImageFile)
{
  NS_ENSURE_STATE(mFirmwareImageFile);

  mozilla::MonitorAutoLock mon(mMonitor);

  nsresult rv = mFirmwareImageFile->Clone(aFirmwareImageFile);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdate::GetFirmwareReadableVersion(nsAString & aFirmwareReadableVersion)
{
  mozilla::MonitorAutoLock mon(mMonitor);
  aFirmwareReadableVersion = mFirmwareReadableVersion;

  return NS_OK;
}

NS_IMETHODIMP 
sbDeviceFirmwareUpdate::GetFirmwareVersion(PRUint32 *aFirmwareVersion)
{
  NS_ENSURE_ARG_POINTER(aFirmwareVersion);

  mozilla::MonitorAutoLock mon(mMonitor);
  *aFirmwareVersion = mFirmwareVersion;

  return NS_OK;
}

NS_IMETHODIMP sbDeviceFirmwareUpdate::Init(nsIFile *aFirmwareImageFile, 
                                           const nsAString & aFirmwareReadableVersion, 
                                           PRUint32 aFirmwareVersion)
{
  NS_ENSURE_ARG_POINTER(aFirmwareImageFile);

  mFirmwareImageFile = aFirmwareImageFile;
  mFirmwareReadableVersion = aFirmwareReadableVersion;
  mFirmwareVersion = aFirmwareVersion;
  
  return NS_OK;
}
