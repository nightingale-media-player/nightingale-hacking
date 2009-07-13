/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

#ifndef __SB_LIB_HAL_H__
#define __SB_LIB_HAL_H__

/* *****************************************************************************
 *******************************************************************************
 *
 * HAL library services.
 *
 *******************************************************************************
 ******************************************************************************/

/** 
* \file  LibHal.h
* \brief Header for the HAL library services.
*/

/* *****************************************************************************
 *
 * HAL library imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsVoidArray.h>
#include <nsTArray.h>

/* HAL library API imports. */
#include <libhal.h>

/* Logging services imports. */
#include <sbIPDLog.h>


/* *****************************************************************************
 *
 * HAL library classes.
 *
 ******************************************************************************/

/*
 * sbLibHalCtx class.
 */

class sbLibHalCtx
{
    /* *************************************************************************
     *
     * Public interface.
     *
     **************************************************************************/

    public:

    /*
     * Public libhal context services.
     */

    sbLibHalCtx();
    virtual ~sbLibHalCtx();

    nsresult Initialize();


    /*
     * Public libhal API services.
     */

    nsresult SetUserData(
        void                        *aUserData);

    nsresult GetAllDevices(
        nsCStringArray              &aDeviceList);

    nsresult DevicePropertyExists(
        const nsACString            &aUDI,
        const char                  *aKey,
        PRBool                      *apExists);

    nsresult DeviceGetPropertyString(
        const nsACString            &aUDI,
        const char                  *aKey,
        nsCString                   &aProperty);

    nsresult DeviceGetPropertyInt(
        const nsACString            &aUDI,
        const char                  *aKey,
        PRUint32                    *aProperty);

    nsresult DeviceGetPropertyBool(
        const nsACString            &aUDI,
        const char                  *aKey,
        PRBool                      *aProperty);

    nsresult DeviceGetPropertyStringList(
        const nsACString            &aUDI,
        const char                  *aKey,
        nsTArray<nsCString>         &aProperty);

    nsresult SetDeviceAdded(
        LibHalDeviceAdded           aCallback);

    nsresult SetDeviceRemoved(
        LibHalDeviceRemoved         aCallback);

    nsresult SetDevicePropertyModified(
        LibHalDevicePropertyModified
                                    aCallback);

    nsresult DevicePropertyWatchAll();


    /*
     * Public libhal interface services.
     */

    nsresult DeviceHasInterface(
        const nsACString            &aUDI,
        const char                  *aInterface,
        PRBool                      *aHasInterface);


    /*
     * Public D-Bus message services.
     */

    nsresult DeviceVolumeEject(
        nsCString                   &aDeviceUDI);

    nsresult DeviceVolumeUnmount(
        nsCString                   &aDeviceUDI);


    /* *************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * mpLibHalCtx              HAL library API context.
     * mpDBusConnection         HAL library API context connection to D-Bus.
     */

    LibHalContext               *mpLibHalCtx;
    DBusConnection              *mpDBusConnection;


    /*
     * Private D-Bus message services.
     */

    nsresult DeviceCallMethod(
        nsCString                   &aDeviceUDI,
        const char                  *aInterface,
        const char                  *aMethod);

    nsresult DeviceGetMethodRetCode(
        DBusMessage                 *apDBusReply,
        int                         *apRetCode);
};


#endif /* __SB_LIB_HAL_H__ */
