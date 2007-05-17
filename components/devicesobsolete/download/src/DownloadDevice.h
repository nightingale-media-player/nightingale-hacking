/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __DOWNLOAD_DEVICE_H__
#define __DOWNLOAD_DEVICE_H__

/* *****************************************************************************
 *******************************************************************************
 *
 * Download device.
 *
 *******************************************************************************
 ******************************************************************************/

/** 
* \file  DownloadDevice.h
* \brief Songbird DownloadDevice Component Definition.
*/

/* *****************************************************************************
 *
 * Download device configuration.
 *
 ******************************************************************************/

/*
 * Download device XPCOM component definitions.
 */

#define SONGBIRD_DownloadDevice_CONTRACTID                                     \
                            "@songbirdnest.com/Songbird/Device/DownloadDevice;1"
#define SONGBIRD_DownloadDevice_CLASSNAME "Songbird Download Device"
#define SONGBIRD_DownloadDevice_CID                                            \
{                                                                              \
    0x961DA3F4,                                                                \
    0x5EF1,                                                                    \
    0x4AD0,                                                                    \
    { 0x81, 0x8d, 0x62, 0x2C, 0x7B, 0xD1, 0x74, 0x47}                          \
}


/* *****************************************************************************
 *
 * Download device imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include <DeviceBase.h>
#include <sbIDownloadDevice.h>

/* Mozilla imports. */
#include <nsCOMPtr.h>

/* Songbird imports. */
#include <sbILibrary.h>


/* *****************************************************************************
 *
 * Download device classes.
 *
 ******************************************************************************/

/*
 * sbDownloadDevice class.
 */

class sbDownloadDevice :  public sbIDownloadDevice, public sbDeviceBase
{

    /* *************************************************************************
     *
     * Public interface.
     *
     **************************************************************************/

    public:

    /*
     * Inherited interfaces.
     */

    NS_DECL_ISUPPORTS
    NS_DECL_SBIDEVICEBASE
    NS_DECL_SBIDOWNLOADDEVICE


    /* *************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * mpDownloadLibrary        Download device library.
     */

    nsCOMPtr<sbILibrary>        mpDownloadLibrary;
};

#endif // __DOWNLOAD_DEVICE_H__
