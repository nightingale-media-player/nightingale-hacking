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

/* *****************************************************************************
 *******************************************************************************
 *
 * HAL library services.
 *
 *******************************************************************************
 ******************************************************************************/

/** 
* \file  LibHal.cpp
* \brief Source for the HAL library services.
*/

/* *****************************************************************************
 *
 * HAL library imported services.
 *
 ******************************************************************************/

/* Self imports. */
#include <sbLibHal.h>

#include <sbDebugUtils.h>

/* D-Bus imports. */
#include <dbus/dbus-glib-lowlevel.h>


/* *****************************************************************************
 *
 * HAL library services.
 *
 ******************************************************************************/

/*
 * sbLibHalCtx
 *
 *   This method is the constructor for the HAL library context services class.
 */

sbLibHalCtx::sbLibHalCtx()
:
    mpLibHalCtx(NULL),
    mpDBusConnection(NULL)
{
  SB_PRLOG_SETUP(sbLibHalCtx);
}


/*
 * ~sbLibHalCtx
 *
 *   This method is the destructor for the HAL library context services class.
 */

sbLibHalCtx::~sbLibHalCtx()
{
    /* Dispose of the HAL library context. */
    if (mpLibHalCtx)
    {
        libhal_ctx_shutdown(mpLibHalCtx, NULL);
        libhal_ctx_free(mpLibHalCtx);
    }

    /* Dispose of the D-Bus connection. */
    if (mpDBusConnection)
        dbus_connection_unref(mpDBusConnection);
}


/*
 * Initialize
 *
 *   This method initializes the HAL library context services.
 */

nsresult sbLibHalCtx::Initialize()
{
    dbus_bool_t                 success;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Create a HAL library context. */
    mpLibHalCtx = libhal_ctx_new();
    if (!mpLibHalCtx)
        rv = NS_ERROR_OUT_OF_MEMORY;

    /* Set up a D-Bus connection with the glib main event loop. */
    if (NS_SUCCEEDED(rv))
    {
        mpDBusConnection = dbus_bus_get(DBUS_BUS_SYSTEM, &dBusError);
        if (!mpDBusConnection)
            rv = NS_ERROR_OUT_OF_MEMORY;
    }
    if (NS_SUCCEEDED(rv))
        dbus_connection_setup_with_g_main(mpDBusConnection, NULL);

    /* Connect the HAL library context to D-Bus. */
    if (NS_SUCCEEDED(rv))
    {
        success = libhal_ctx_set_dbus_connection(mpLibHalCtx, mpDBusConnection);
        if (!success)
            rv = NS_ERROR_FAILURE;
    }

    /* Initialize the HAL library context. */
    if (NS_SUCCEEDED(rv))
        success = libhal_ctx_init(mpLibHalCtx, &dBusError);

    /* Log any errors. */
    if (dbus_error_is_set(&dBusError))
    {
        LOG("sbLibHalCtx::Initialize error %s: %s\n",
             dBusError.name,
             dBusError.message);
        dbus_error_init(&dBusError);
    }

    /* Clean up. */
    dbus_error_free(&dBusError);

    return rv;
}


/* *****************************************************************************
 *
 * HAL library API services.
 *
 ******************************************************************************/

/*
 * SetUserData
 *
 *   --> aUserData              User data.
 *
 *   This function associates the user data specified by aUserData with the
 * HAL library context.
 */

nsresult sbLibHalCtx::SetUserData(
    void                        *aUserData)
{
    dbus_bool_t                 success;

    /* Set the user data. */
    success = libhal_ctx_set_user_data(mpLibHalCtx, aUserData);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    return NS_OK;
}


/*
 * GetAllDevices
 *
 *   <-- aDeviceList            List of devices.
 *
 *   This function returns in aDeviceList, the list of all devices.  Each entry
 * in the list contains the device UDI.
 */

nsresult sbLibHalCtx::GetAllDevices(
    nsCStringArray              &aDeviceList)
{
    char                        **deviceList = NULL;
    int                         deviceCnt;
    int                         i;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Get the list of devices. */
    deviceList = libhal_get_all_devices(mpLibHalCtx, &deviceCnt, &dBusError);
    if (!deviceList)
        rv = NS_ERROR_UNEXPECTED;

    /* Return the list of devices. */
    if (NS_SUCCEEDED(rv))
    {
        aDeviceList.Clear();
        for (i = 0; i < deviceCnt; i++)
        {
            aDeviceList.AppendCString(nsCString(deviceList[i]));
        }
    }

    /* Log any errors. */
    if (dbus_error_is_set(&dBusError))
    {
        LOG("sbLibHalCtx::GetAllDevices error %s: %s\n",
             dBusError.name,
             dBusError.message);
        dbus_error_init(&dBusError);
    }

    /* Clean up. */
    dbus_error_free(&dBusError);
    if (deviceList)
        libhal_free_string_array(deviceList);

    return rv;
}


/*
 * DevicePropertyExists
 *
 *   --> aUDI                   Device for which to check property.
 *   --> aKey                   Key for property to check.
 *   <-- apExists               True if property exists.
 *
 *   This function checks if the property with the key specified by aKey exists
 * on the device specified by aUDI.  If it does, this function returns true in
 * apExists; otherwise, it returns false.
 */

nsresult sbLibHalCtx::DevicePropertyExists(
    const nsACString            &aUDI,
    const char                  *aKey,
    bool                      *apExists)
{
    dbus_bool_t                 exists;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(aKey, "aKey is null");
    NS_ASSERTION(apExists, "apExists is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Check if the property exists. */
    exists = libhal_device_property_exists(mpLibHalCtx,
                                           aUDI.BeginReading(),
                                           aKey,
                                           &dBusError);

    /* Log any errors. */
    if (dbus_error_is_set(&dBusError))
    {
        LOG("sbLibHalCtx::DevicePropertyExists error %s: %s\n",
             dBusError.name,
             dBusError.message);
        dbus_error_init(&dBusError);
        rv = NS_ERROR_UNEXPECTED;
    }

    /* Clean up. */
    dbus_error_free(&dBusError);

    /* Return results. */
    if (NS_SUCCEEDED(rv))
    {
        if (exists)
            *apExists = PR_TRUE;
        else
            *apExists = PR_FALSE;
    }

    return rv;
}


/*
 * DeviceGetPropertyString
 *
 *   --> aUDI                   Device for which to get property.
 *   --> aKey                   Key for property to get.
 *   <-- aProperty              Property value.
 *
 *   This function returns in aProperty the string value for the property with
 * the key specified by aKey of the device specified by aUDI.
 */

nsresult sbLibHalCtx::DeviceGetPropertyString(
    const nsACString            &aUDI,
    const char                  *aKey,
    nsCString                   &aProperty)
{
    char                        *property = NULL;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(aKey, "aKey is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Get the property string. */
    property = libhal_device_get_property_string(mpLibHalCtx,
                                                 aUDI.BeginReading(),
                                                 aKey,
                                                 &dBusError);
    if (property)
        aProperty.AssignLiteral(property);
    else
        rv = NS_ERROR_FAILURE;

    /* Don't log errors. */

    /* Clean up. */
    dbus_error_free(&dBusError);
    if (property)
        libhal_free_string(property);

    return rv;
}


/*
 * DeviceGetPropertyInt
 *
 *   --> aUDI                   Device for which to get property.
 *   --> aKey                   Key for property to get.
 *   <-- aProperty              Property value.
 *
 *   This function returns in aProperty the integer value for the property with
 * the key specified by aKey of the device specified by aUDI.
 */

nsresult sbLibHalCtx::DeviceGetPropertyInt(
    const nsACString            &aUDI,
    const char                  *aKey,
    PRUint32                    *aProperty)
{
    int                         property;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(aKey, "aKey is null");
    NS_ASSERTION(aProperty, "aProperty is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Get the property int. */
    property = libhal_device_get_property_int(mpLibHalCtx,
                                              aUDI.BeginReading(),
                                              aKey,
                                              &dBusError);
    if (!dbus_error_is_set(&dBusError))
        *aProperty = property;
    else
        rv = NS_ERROR_FAILURE;

    /* Don't log errors. */

    /* Clean up. */
    dbus_error_free(&dBusError);

    return rv;
}


/*
 * DeviceGetPropertyBool
 *
 *   --> aUDI                   Device for which to get property.
 *   --> aKey                   Key for property to get.
 *   <-- aProperty              Property value.
 *
 *   This function returns in aProperty the boolean value for the property with
 * the key specified by aKey of the device specified by aUDI.
 */

nsresult sbLibHalCtx::DeviceGetPropertyBool(
    const nsACString            &aUDI,
    const char                  *aKey,
    bool                      *aProperty)
{
    dbus_bool_t                 property;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(aKey, "aKey is null");
    NS_ASSERTION(aProperty, "aProperty is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Get the property int. */
    property = libhal_device_get_property_bool(mpLibHalCtx,
                                               aUDI.BeginReading(),
                                               aKey,
                                               &dBusError);
    if (!dbus_error_is_set(&dBusError))
        *aProperty = property;
    else
        rv = NS_ERROR_FAILURE;

    /* Don't log errors. */

    /* Clean up. */
    dbus_error_free(&dBusError);

    return rv;
}


/*
 * DeviceGetPropertyStringList
 *
 *   --> aUDI                   Device for which to get property.
 *   --> aKey                   Key for property to get.
 *   <-- aProperty              Property value.
 *
 *   This function returns in aProperty the string list value for the property
 * with the key specified by aKey of the device specified by aUDI.
 */

nsresult sbLibHalCtx::DeviceGetPropertyStringList(
    const nsACString            &aUDI,
    const char                  *aKey,
    nsTArray<nsCString>         &aProperty)
{
    char                        **property = NULL;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(aKey, "aKey is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Get the property string list. */
    property = libhal_device_get_property_strlist(mpLibHalCtx,
                                                  aUDI.BeginReading(),
                                                  aKey,
                                                  &dBusError);
    if (property)
    {
        char                        **pPropStr;

        pPropStr = &(property[0]);
        while (*pPropStr)
        {
            aProperty.AppendElement(*pPropStr);
            pPropStr++;
        }
    }
    else
    {
        rv = NS_ERROR_FAILURE;
    }

    /* Don't log errors since property may not exist. */

    /* Clean up. */
    dbus_error_free(&dBusError);
    if (property)
        libhal_free_string_array(property);

    return rv;
}


/*
 * SetDeviceAdded
 *
 *   --> aCallback              Device added callback.
 *
 *   This function sets the device added callback to the callback specified by
 * aCallback.
 */

nsresult sbLibHalCtx::SetDeviceAdded(
    LibHalDeviceAdded           aCallback)
{
    dbus_bool_t                 success;

    /* Validate parameters. */
    NS_ASSERTION(aCallback, "aCallback is null");

    /* Set the callback. */
    success = libhal_ctx_set_device_added(mpLibHalCtx, aCallback);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    return NS_OK;
}


/*
 * SetDeviceRemoved
 *
 *   --> aCallback              Device removed callback.
 *
 *   This function sets the device removed callback to the callback specified by
 * aCallback.
 */

nsresult sbLibHalCtx::SetDeviceRemoved(
    LibHalDeviceRemoved         aCallback)
{
    dbus_bool_t                 success;

    /* Validate parameters. */
    NS_ASSERTION(aCallback, "aCallback is null");

    /* Set the callback. */
    success = libhal_ctx_set_device_removed(mpLibHalCtx, aCallback);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    return NS_OK;
}


/*
 * SetDevicePropertyModified
 *
 *   --> aCallback              Device property modified callback.
 *
 *   This function sets the device property modified callback to the callback
 * specified by aCallback.
 */

nsresult sbLibHalCtx::SetDevicePropertyModified(
    LibHalDevicePropertyModified
                                aCallback)
{
    dbus_bool_t                 success;

    /* Validate parameters. */
    NS_ASSERTION(aCallback, "aCallback is null");

    /* Set the callback. */
    success = libhal_ctx_set_device_property_modified(mpLibHalCtx, aCallback);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    return NS_OK;
}


/*
 * DevicePropertyWatchAll
 *
 *   This function sets the HAL library API services to watch for and deliver
 * callbacks for all device property changes.
 */

nsresult sbLibHalCtx::DevicePropertyWatchAll()
{
    DBusError                   dBusError;
    dbus_bool_t                 success;
    nsresult                    rv = NS_OK;

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Set to watch all property changes for all devices. */
    success = libhal_device_property_watch_all(mpLibHalCtx, &dBusError);
    if (!success)
        rv = NS_ERROR_UNEXPECTED;

    /* Log any errors. */
    if (dbus_error_is_set(&dBusError))
    {
        LOG("sbLibHalCtx::DevicePropertyWatchAll error %s: %s\n",
             dBusError.name,
             dBusError.message);
        dbus_error_init(&dBusError);
    }

    /* Clean up. */
    dbus_error_free(&dBusError);

    return rv;
}


/* *****************************************************************************
 *
 * HAL library interface services.
 *
 ******************************************************************************/

/*
 * DeviceHasInterface
 *
 *   --> aUDI                   Device for which to check for interface.
 *   --> aInterface             Interface for which to check.
 *   <-- aHasInterface          True if device has interface.
 *
 *   This function checks if the device specified by aUDI has the interface
 * specified by aInterface.  If it does, this function returns true in
 * aHasInterface; otherwise, it returns false.
 */

nsresult sbLibHalCtx::DeviceHasInterface(
    const nsACString            &aUDI,
    const char                  *aInterface,
    bool                      *aHasInterface)
{
    nsTArray<nsCString>         interfaceList;
    PRUint32                    interfaceCount;
    bool                      exists;
    PRUint32                    i;
    nsresult                    rv;

    /* Check if the device has any interfaces. */
    rv = DevicePropertyExists(aUDI, "info.interfaces", &exists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!exists)
    {
        *aHasInterface = PR_FALSE;
        return NS_OK;
    }

    /* Get the list of device interfaces. */
    rv = DeviceGetPropertyStringList(aUDI, "info.interfaces", interfaceList);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Search for the interface in the interface list. */
    interfaceCount = interfaceList.Length();
    for (i = 0; i < interfaceCount; i++)
    {
        if (interfaceList[i].EqualsLiteral(aInterface))
        {
            *aHasInterface = PR_TRUE;
            return NS_OK;
        }
    }

    /* Interface not found. */
    *aHasInterface = PR_FALSE;
    return NS_OK;
}


/* *****************************************************************************
 *
 * HAL library D-Bus message services.
 *
 ******************************************************************************/

/*
 * DeviceVolumeEject
 *
 *   --> aDeviceUDI             Target device UDI.
 *
 *   This function sends a call device volume eject method message for the
 * device specified by aDeviceUDI.
 */

nsresult sbLibHalCtx::DeviceVolumeEject(
    nsCString                   &aDeviceUDI)
{
    return (DeviceCallMethod(aDeviceUDI,
                             "org.freedesktop.Hal.Device.Volume",
                             "Eject"));
}


/*
 * DeviceVolumeUnmount
 *
 *   --> aDeviceUDI             Target device UDI.
 *
 *   This function sends a call device volume unmount method message for the
 * device specified by aDeviceUDI.
 */

nsresult sbLibHalCtx::DeviceVolumeUnmount(
    nsCString                   &aDeviceUDI)
{
    return (DeviceCallMethod(aDeviceUDI,
                             "org.freedesktop.Hal.Device.Volume",
                             "Unmount"));
}


/* *****************************************************************************
 *
 * Private HAL library D-Bus message services.
 *
 ******************************************************************************/

/*
 * DeviceCallMethod
 *
 *   --> aDeviceUDI             Target device UDI.
 *   --> aInterface             Method interface.
 *   --> aMethod                Method name.
 *
 *   This function sends the call method message specified by aInterface and
 * aMethod for the device specified by aDeviceUDI.
 */

nsresult sbLibHalCtx::DeviceCallMethod(
    nsCString                   &aDeviceUDI,
    const char                  *aInterface,
    const char                  *aMethod)
{
    DBusMessage                 *pDBusMessage = NULL;
    DBusMessage                 *pDBusReply = NULL;
    char                        **options = NULL;
    int                         numOptions = 0;
    int                         halRetCode = 0;
    dbus_bool_t                 success;
    DBusError                   dBusError;
    nsresult                    rv = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(aInterface, "aInterface is null");
    NS_ASSERTION(aMethod, "aMethod is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Set up the D-Bus message. */
    pDBusMessage = dbus_message_new_method_call("org.freedesktop.Hal", 
                                                aDeviceUDI.get(),
                                                aInterface,
                                                aMethod);
    if (!pDBusMessage)
        rv = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(rv))
    {
        success = dbus_message_append_args(pDBusMessage,
                                           DBUS_TYPE_ARRAY,
                                           DBUS_TYPE_STRING,
                                           &options,
                                           numOptions,
                                           DBUS_TYPE_INVALID);
        if (!success)
            rv = NS_ERROR_UNEXPECTED;
    }

    /* Send the D-Bus message. */
    if (NS_SUCCEEDED(rv))
    {
        pDBusReply = dbus_connection_send_with_reply_and_block
                                                            (mpDBusConnection, 
                                                             pDBusMessage, 
                                                             -1,
                                                             &dBusError);
        if (!pDBusReply)
            rv = NS_ERROR_FAILURE;
    }

    /* Check for an error return code.  If the return */
    /* code cannot be obtained, assume no error.      */
    if (NS_SUCCEEDED(rv))
    {
        rv = DeviceGetMethodRetCode(pDBusReply, &halRetCode);
        if (NS_SUCCEEDED(rv) && halRetCode)
            rv = NS_ERROR_FAILURE;
        else
            rv = NS_OK;
    }

    /* Log any errors. */
    if (dbus_error_is_set(&dBusError))
    {
        LOG("sbLibHalCtx::DeviceCallMethod error %s: %s\n",
             dBusError.name,
             dBusError.message);
        dbus_error_init(&dBusError);
    }

    /* Clean up. */
    dbus_error_free(&dBusError);
    if (pDBusMessage)
        dbus_message_unref(pDBusMessage);
    if (pDBusReply)
        dbus_message_unref(pDBusReply);

    return rv;
}


/*
 * DeviceGetMethodRetCode
 *
 *   --> apDBusReply            HAL D-Bus reply message.
 *   <-- apRetCode              HAL return code.
 *
 *   This method returns in apRetCode the HAL return code from the HAL device
 * call method reply message specified by apDBusReply.
 */

nsresult sbLibHalCtx::DeviceGetMethodRetCode(
    DBusMessage                 *apDBusReply,
    int                         *apRetCode)
{
    DBusMessageIter             dBusMessageIter;
    int                         retCode;
    int                         argType;
    dbus_bool_t                 success;
    nsresult                    rv = NS_OK;
    DBusError                   dBusError;

    /* Validate parameters. */
    NS_ASSERTION(apDBusReply, "apDBusReply is null");
    NS_ASSERTION(apRetCode, "apRetCode is null");

    /* Initialize D-Bus errors. */
    dbus_error_init(&dBusError);

    /* Set up a reply message iterator. */
    success = dbus_message_iter_init(apDBusReply, &dBusMessageIter);
    if (!success)
        rv = NS_ERROR_UNEXPECTED;

    /* Validate return code argument type.  Different version */
    /* of the HAL library API use signed or unsigned ints.    */
    argType = dbus_message_iter_get_arg_type(&dBusMessageIter);
    if ((argType != DBUS_TYPE_INT32) && (argType != DBUS_TYPE_UINT32))
    {
        LOG("sbLibHalCtx::DeviceGetMethodRetCode "
             "unexpected return code type %d\n",
             argType);
        rv = NS_ERROR_UNEXPECTED;
    }

    /* Get the return code. */
    if (NS_SUCCEEDED(rv))
        dbus_message_iter_get_basic(&dBusMessageIter, &retCode);

    /* Log any errors. */
    if (dbus_error_is_set(&dBusError))
    {
        LOG("sbLibHalCtx::DeviceGetMethodRetCode error %s: %s\n",
             dBusError.name,
             dBusError.message);
        dbus_error_init(&dBusError);
    }

    /* Clean up. */
    dbus_error_free(&dBusError);

    /* Return results. */
    if (NS_SUCCEEDED(rv))
        *apRetCode = retCode;

    return NS_OK;
}


