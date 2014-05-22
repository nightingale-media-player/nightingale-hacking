/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
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

/*
 * Written by Logan F. Smyth Š 2009
 * http://logansmyth.com
 * me@logansmyth.com
 */

#include <cstring>

#include <nsXPCOM.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsStringAPI.h>

#include <nsIMutableArray.h>
#include <nsISupportsPrimitives.h>

#include "ngDbusConnection.h"

#include "prlog.h"
#include "prprf.h"

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=ngDbusConnection:5
 */
#ifdef PR_LOGGING
  static PRLogModuleInfo* gDBusConnectionLog = nsnull;
  #define LOG(args) PR_LOG(gDBusConnectionLog, PR_LOG_WARN, args)
#else
  #define LOG(args) /* nothing */
#endif /* PR_LOGGING */

using namespace std;


/* Implementation file */
NS_IMPL_ISUPPORTS1(ngDbusConnection, ngIDbusConnection)

ngDbusConnection::ngDbusConnection()
: conn(0)
, signal_msg(0)
, handler(0)
{
  /* member initializers and constructor code */
    #ifdef PR_LOGGING
        if(!gDBusConnectionLog)
            gDBusConnectionLog = PR_NewLogModule("ngDbusConnection");
    #endif
}

ngDbusConnection::~ngDbusConnection()
{
  /* destructor code */
}

/* long init (in string name); */
NS_IMETHODIMP ngDbusConnection::Init(const char *name, PRInt32 *_retval)
{
    NS_ENSURE_ARG_POINTER(name);
    *_retval = nsnull;
    DBusError error;

    LOG(("Initializing DBus"));

    dbus_error_init (&error);
    conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if (!conn) {
      NS_ERROR(("Error initializing DBus connection: %s:%s", error.name, error.message));
      dbus_error_free(&error);
      *_retval = 1;
    }

    LOG(("Requesting Name on DBus"));
    dbus_bus_request_name(conn, name, 0, &error);
    if (dbus_error_is_set (&error)) {
      NS_ERROR(("Error requesting DBus name: %s:%s", error.name, error.message));
      dbus_error_free(&error);
      *_retval = 1;
    }

    LOG(("Flushing"));

    dbus_connection_flush(conn);
    LOG(("End Init()"));

    return NS_OK;
}

/* long setMatch (in string match); */
NS_IMETHODIMP ngDbusConnection::SetMatch(const char *match)
{
    NS_ENSURE_ARG_POINTER(match);

    DBusError error;
    dbus_error_init (&error);
    LOG(("Setting match %s", match));

      /* listening to messages from all objects as no path is specified */
    dbus_bus_add_match (conn, match, &error);
    //~ dbus_bus_add_match (conn, match, NULL);

    if (dbus_error_is_set (&error)) {
      NS_ERROR(("Match Error: %s: %s", error.name, error.message));
      dbus_error_free(&error);
    }


    dbus_connection_flush(conn);

    LOG(("DBus Connection Init"));

    return NS_OK;
}

/* void check (); */
NS_IMETHODIMP ngDbusConnection::Check()
{
    if(conn == nsnull) return NS_OK;

    //~ int i;
    //~ for(i = 0; i < 30; i++){
	//~ LOG(NS_ConvertUTF16toUTF8("Flamboyant").get());
    //~ }



    DBusMessage* msg;
    DBusMessage* reply;
    DBusMessageIter *reply_iter;

    // non blocking read of the next available message
    dbus_connection_read_write(conn, 0);
    msg = dbus_connection_pop_message(conn);

    if(msg != nsnull){
      if(dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL || dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL){
	    LOG(("*****Calling %s", dbus_message_get_member(msg)));

	    reply_iter = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

	    dbus_message_iter_init(msg, &incoming_args);

	    reply = dbus_message_new_method_return(msg);
	    dbus_message_iter_init_append(reply, reply_iter);

	    outgoing_args.push_back(reply_iter);


	    const char* interface = dbus_message_get_interface(msg);
	    const char* path = dbus_message_get_path(msg);
	    const char* member = dbus_message_get_member(msg);

        #if PR_LOGGING
            if(interface != nsnull) LOG(("Interface: %s", interface));
            if(path != nsnull) LOG(("Path: %s", path));
            if(member != nsnull) LOG(("Member: %s", member));
        #endif

	    handler->HandleMethod(interface, path, member);

	    LOG(("Just after Handler call"));

	    dbus_uint32_t serial = 0;

	    // send the reply && flush the connection
	    NS_ENSURE_TRUE(dbus_connection_send(conn, reply, &serial), NS_ERROR_OUT_OF_MEMORY);

	    while(!outgoing_args.empty()){
	        NS_Free(outgoing_args.back());
	        outgoing_args.pop_back();
	    }

	    dbus_connection_flush(conn);
	    dbus_message_unref(reply);


	    LOG(("Done Handler call"));
      }

      dbus_message_unref(msg);

    }

    return NS_OK;
}

/* long end (); */
NS_IMETHODIMP ngDbusConnection::End(PRInt32 *_retval)
{
    return NS_OK;
}

/* vois prepareMethodCall(in string dest, in string path, in string inter, in string name); */
NS_IMETHODIMP ngDbusConnection::PrepareMethodCall(const char* dest, const char* path,  const char* inter, const char* name)
{
    NS_ENSURE_ARG_POINTER(dest);
    NS_ENSURE_ARG_POINTER(path);
    NS_ENSURE_ARG_POINTER(inter);
    NS_ENSURE_ARG_POINTER(name);

  LOG(("*****Preparing method call %s, %s, %s", path, inter, name));
  DBusMessageIter* reply_iter = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

  // Create a D-Bus method call message.
  signal_msg = dbus_message_new_method_call(dest,
                                            path,
                                            inter,
                                            name);
  dbus_message_iter_init_append(signal_msg, reply_iter);
  outgoing_args.push_back(reply_iter);

  return NS_OK;
}

/* void sendMethodCall (); */
NS_IMETHODIMP ngDbusConnection::SendMethodCall()
{
    LOG(("Starting to send method call"));

    dbus_connection_send(conn, signal_msg, nsnull);

    dbus_message_unref(signal_msg);

    LOG(("Freeing args"));
    while(!outgoing_args.empty()){
	NS_Free(outgoing_args.back());
	outgoing_args.pop_back();
    }

    dbus_connection_flush(conn);

    LOG(("*****Method Call Sent"));

    return NS_OK;
}

/* void sprepareSignal (in string path, in string inter, in string name); */
NS_IMETHODIMP ngDbusConnection::PrepareSignal(const char *path, const char *inter, const char *name)
{
    NS_ENSURE_ARG_POINTER(path);
    NS_ENSURE_ARG_POINTER(inter);
    NS_ENSURE_ARG_POINTER(name);

    LOG(("*****Preparing signal %s, %s, %s", path, inter, name));
    signal_msg = dbus_message_new_signal(path, inter, name);

    DBusMessageIter* reply_iter = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    dbus_message_iter_init_append(signal_msg, reply_iter);
    outgoing_args.push_back(reply_iter);

    LOG(("Signal prepared"));

    return NS_OK;
}

/* void sendSignal (); */
NS_IMETHODIMP ngDbusConnection::SendSignal()
{
    LOG(("Starting to send signal"));

    dbus_connection_send(conn, signal_msg, nsnull);

    dbus_message_unref(signal_msg);

    LOG(("Freeing args"));
    while(!outgoing_args.empty()){
	NS_Free(outgoing_args.back());
	outgoing_args.pop_back();
    }

    dbus_connection_flush(conn);

    LOG(("*****Signal Sent"));

    return NS_OK;
}
/* long setMethodHandler (in ngIMethodHandler handler); */
NS_IMETHODIMP ngDbusConnection::SetMethodHandler(ngIMethodHandler *handler)
{
    NS_ENSURE_ARG_POINTER(handler);
    NS_ADDREF(handler);	//not sure if this is needed
    this->handler = handler;
    return NS_OK;
}

/* long getInt64Arg (); */
NS_IMETHODIMP ngDbusConnection::GetInt64Arg(PRInt64 *_retval)
{
    int type = dbus_message_iter_get_arg_type(&incoming_args);

    if(type == DBUS_TYPE_INT64){
      dbus_int64_t data;
      dbus_message_iter_get_basic(&incoming_args, &data);
      dbus_message_iter_next(&incoming_args);

      *_retval = data;
    }
    else{
      char* msg = PR_smprintf("WARNING: %i received but expecting a Int64", type);
      NS_WARNING(msg);
      PR_smprintf_free(msg);
    }

    return NS_OK;
}

/* long getBoolArg (); */
NS_IMETHODIMP ngDbusConnection::GetBoolArg(PRBool *_retval)
{
    int type = dbus_message_iter_get_arg_type(&incoming_args);

    if(type == DBUS_TYPE_BOOLEAN){
      dbus_bool_t data;
      dbus_message_iter_get_basic(&incoming_args, &data);
      dbus_message_iter_next(&incoming_args);

      *_retval = data;
    }
    else{
      char* msg = PR_smprintf("WARNING: %i received but expecting a Bool", type);
      NS_WARNING(msg);
      PR_smprintf_free(msg);
    }

    return NS_OK;
}

/* long getStringArg (); */
NS_IMETHODIMP ngDbusConnection::GetStringArg(char **_retval)
{

    int type = dbus_message_iter_get_arg_type(&incoming_args);

    if(type == DBUS_TYPE_STRING){
      const char* dat;
      dbus_message_iter_get_basic(&incoming_args, &dat);

      //this is going to leak
      //not sure if the garbage collector will get this or not...
      *_retval = (char*)NS_Alloc(sizeof(char)*(strlen(dat) + 1));

      strcpy(*_retval, dat);
      LOG(("String contained %s", *_retval));

      dbus_message_iter_next(&incoming_args);

    }
    else{
      char* msg = PR_smprintf("WARNING: %i received but expecting a String", type);
      NS_WARNING(msg);
      PR_smprintf_free(msg);
    }

    return NS_OK;
}

/* long getObjectPathArg (); */
NS_IMETHODIMP ngDbusConnection::GetObjectPathArg(char **_retval)
{

    int type = dbus_message_iter_get_arg_type(&incoming_args);

    if(type == DBUS_TYPE_OBJECT_PATH){
      const char* dat;
      dbus_message_iter_get_basic(&incoming_args, &dat);

      //this is going to leak
      //not sure if the garbage collector will get this or not...
      *_retval = (char*)NS_Alloc(sizeof(char)*(strlen(dat) + 1));

      strcpy(*_retval, dat);

      dbus_message_iter_next(&incoming_args);

    }
    else{
      char* msg = PR_smprintf("WARNING: %i received but expecting a Object", type);
      NS_WARNING(msg);
      PR_smprintf_free(msg);
    }

    return NS_OK;
}

/* double getInt64Arg (); */
NS_IMETHODIMP ngDbusConnection::GetDoubleArg(PRFloat64 *_retval)
{
    int type = dbus_message_iter_get_arg_type(&incoming_args);

    if(type == DBUS_TYPE_DOUBLE){
      double data;
      dbus_message_iter_get_basic(&incoming_args, &data);
      dbus_message_iter_next(&incoming_args);

      *_retval = data;
    }
    else{
      char* msg = PR_smprintf("WARNING: %i received but expecting a Double", type);
      NS_WARNING(msg);
      PR_smprintf_free(msg);
    }

    return NS_OK;
}

/* nsIArray getArrayArg (); */
NS_IMETHODIMP ngDbusConnection::GetArrayArg(nsIArray** _retval)
{
    LOG(("Getting array argument"));

    nsCOMPtr<nsIMutableArray> array = do_CreateInstance("@mozilla.org/array;1");
    int type = dbus_message_iter_get_arg_type(&incoming_args);
    nsresult rv = NS_OK;

    if(type == DBUS_TYPE_ARRAY){
      DBusMessageIter data;
      DBusBasicValue value;
      int type;
      dbus_message_iter_recurse(&incoming_args, &data);
      while(dbus_message_iter_has_next(&data)) {
        dbus_message_iter_get_basic(&data, &value);
        type = dbus_message_iter_get_arg_type(&data);
        dbus_message_iter_next(&data);

        if(type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
            // Convert the DBusBasicValue to something that implements nsISupports
            nsCOMPtr<nsISupportsString> stringVal = do_CreateInstance("@mozilla.org/supports-string;1");
            rv = stringVal->SetData(NS_ConvertUTF8toUTF16(NS_LITERAL_CSTRING(value.string)));
            NS_ENSURE_SUCCESS(rv, rv);

            rv = array->AppendElement(stringVal, false);
            NS_ENSURE_SUCCESS(rv, rv);
        }
        else {
            rv = NS_ERROR_NOT_IMPLEMENTED;
        }
      }

      dbus_message_iter_next(&incoming_args);

      *_retval = array;
      NS_ADDREF(*_retval);
    }
    else{
      char* msg = PR_smprintf("WARNING: %i received but expecting an Array", type);
      NS_WARNING(msg);
      PR_smprintf_free(msg);;
    }

    LOG(("Array argument got"));

    return rv;
}

/* void setInt32Arg (in long val); */
NS_IMETHODIMP ngDbusConnection::SetInt32Arg(PRInt32 val)
{
    LOG(("Setting Int32 %d", val));
    DBusMessageIter* args = outgoing_args.back();

    dbus_int32_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_INT32, &data);

    LOG(("Set Int32"));

    return NS_OK;
}


/* void setUInt32Arg (in long val); */
NS_IMETHODIMP ngDbusConnection::SetUInt32Arg(PRUint32 val)
{
    LOG(("Setting uInt32 %d", val));

    DBusMessageIter* args = outgoing_args.back();

    dbus_message_iter_append_basic(args, DBUS_TYPE_UINT32, &val);

    LOG(("Set uInt32"));

    return NS_OK;
}

/* void setUInt16Arg (in long val); */
NS_IMETHODIMP ngDbusConnection::SetUInt16Arg(PRUint16 val)
{
    LOG(("Set uInt16 %d", val));
    DBusMessageIter* args = outgoing_args.back();

    dbus_uint16_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_UINT16, &data);

    LOG(("Set uInt16"));
    return NS_OK;
}

/* void setStringArg (in string val); */
//NS_IMETHODIMP ngDbusConnection::SetStringArg(const nsAString &data)
NS_IMETHODIMP ngDbusConnection::SetStringArg(const char *data)
{
    NS_ENSURE_ARG_POINTER(data);
    LOG(("Setting string %s", data));
    DBusMessageIter* args = outgoing_args.back();

    dbus_message_iter_append_basic(args, DBUS_TYPE_STRING, &data);

    LOG(("Set string"));
    return NS_OK;
}

/* void setDictSSEntryArg (in string key, in AString val, [optional] in boolean escape); */
NS_IMETHODIMP ngDbusConnection::SetDictSSEntryArg(const char *key, const nsAString &val)
{
    NS_ENSURE_ARG_POINTER(key);

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;

    char* data = ToNewUTF8String(val);

    LOG(("Setting dict SS %s:%s", key, data));

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_STRING, &data);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);

    LOG(("Set dict SS entry"));

    return NS_OK;
}

/* void setDictSOEntryArg (in string key, in AString val, [optional] in boolean escape); */
NS_IMETHODIMP ngDbusConnection::SetDictSOEntryArg(const char *key, const char* data)
{
    NS_ENSURE_ARG_POINTER(key);
    NS_ENSURE_ARG_POINTER(data);

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;

    LOG(("Setting dict SO %s:%s", key, data));

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_OBJECT_PATH_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_OBJECT_PATH, &data);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);

    LOG(("Set dict SO entry"));

    return NS_OK;
}

/* void setDictSIEntryArg (in string key, in long long val); */
NS_IMETHODIMP ngDbusConnection::SetDictSI64EntryArg(const char *key, PRInt64 val)
{
    NS_ENSURE_ARG_POINTER(key);
    LOG(("Setting dict SI64 %s:%d", key, val));

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_INT64_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_INT64, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);

    LOG(("Set dict SI64 entry"));

    return NS_OK;
}

/* void setDictSIEntryArg (in string key, in long val); */
NS_IMETHODIMP ngDbusConnection::SetDictSIEntryArg(const char *key, PRUint32 val)
{
    NS_ENSURE_ARG_POINTER(key);
    LOG(("Setting dict SI %s:%d", key, val));

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_UINT32_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_UINT32, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);

    LOG(("Set dict SI entry"));

    return NS_OK;
}

/* void setDictSBEntryArg (in string key, in boolean val); */
NS_IMETHODIMP ngDbusConnection::SetDictSBEntryArg(const char *key, PRBool val)
{
    NS_ENSURE_ARG_POINTER(key);
    LOG(("Setting dict SB %s:%i", key, val));

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_BOOLEAN, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);

    LOG(("Set dict SB entry"));

    return NS_OK;
}

/* void setDictSDEntryArg (in string key, in double val); */
NS_IMETHODIMP ngDbusConnection::SetDictSDEntryArg(const char *key, PRFloat64 val)
{
    NS_ENSURE_ARG_POINTER(key);
    LOG(("Setting dict SD %s:%s", key, val));

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_DOUBLE_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_DOUBLE, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);

    LOG(("Set dict SD entry"));

    return NS_OK;
}

/* void openDictSAEntryArg (in string key); */
NS_IMETHODIMP ngDbusConnection::OpenDictSAEntryArg(const char *key, PRInt16 aType = TYPE_STRING)
{
    NS_ENSURE_ARG_POINTER(key);
    LOG(("Opening dict SA : %s", key));

    nsresult rv;
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter* entry_obj = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    DBusMessageIter* new_val = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, entry_obj);
    dbus_message_iter_append_basic(entry_obj, DBUS_TYPE_STRING, &key);

    char combinedType[strlen(DBUS_TYPE_ARRAY_AS_STRING) + strlen(this->ngTypeToDBusType(aType)) + 1];
    combinedType[0] = '\0';
    strcat(combinedType, DBUS_TYPE_ARRAY_AS_STRING);
    strcat(combinedType, this->ngTypeToDBusType(aType));

    dbus_message_iter_open_container(entry_obj, DBUS_TYPE_VARIANT, combinedType, new_val);

    outgoing_args.push_back(entry_obj);
    outgoing_args.push_back(new_val);

    rv = this->OpenArray(aType);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("Opened dict SA entry"));

    return rv;
}

/* void closeDictSAEntryArg (); */
NS_IMETHODIMP ngDbusConnection::CloseDictSAEntryArg()
{
    LOG(("Closing dict SA "));

    nsresult rv;
    rv = this->CloseArray();
    NS_ENSURE_SUCCESS(rv, rv);

    DBusMessageIter* new_val = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* entry_obj = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* array_obj = outgoing_args.back();

    dbus_message_iter_close_container(entry_obj, new_val);
    dbus_message_iter_close_container(array_obj, entry_obj);

    LOG(("Closed dict SA entry"));

    return rv;
}
/* void openDictSDEntryArg (in string key); */
NS_IMETHODIMP ngDbusConnection::OpenDictSDEntryArg(const char *key)
{
    NS_ENSURE_ARG_POINTER(key);
    LOG(("Opening dict SD :%s", key));

    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter* entry_obj = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    DBusMessageIter* new_val = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));
    DBusMessageIter* new_arr = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, entry_obj);
      dbus_message_iter_append_basic(entry_obj, DBUS_TYPE_STRING, &key);

      dbus_message_iter_open_container(entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, new_val);
    dbus_message_iter_open_container(new_val, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, new_arr);

    outgoing_args.push_back(entry_obj);
    outgoing_args.push_back(new_val);
    outgoing_args.push_back(new_arr);

    LOG(("Opened dict SD entry"));

    return NS_OK;
}

/* void closeDictSDEntryArg (); */
NS_IMETHODIMP ngDbusConnection::CloseDictSDEntryArg()
{
    LOG(("Closing dict SD "));

    DBusMessageIter* new_arr = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* new_val = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* entry_obj = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* array_obj = outgoing_args.back();

    dbus_message_iter_close_container(new_val, new_arr);
      dbus_message_iter_close_container(entry_obj, new_val);
    dbus_message_iter_close_container(array_obj, entry_obj);

    LOG(("Closed dict SD entry"));

    return NS_OK;
}


/* void setBoolArg (in bool val); */
NS_IMETHODIMP ngDbusConnection::SetBoolArg(PRBool val)
{
    LOG(("Setting Bool %s", val));
    DBusMessageIter* args = outgoing_args.back();

    dbus_bool_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_BOOLEAN, &data);

    LOG(("Set Bool"));

    return NS_OK;
}

/* void setDoubleArg (in double val); */
NS_IMETHODIMP ngDbusConnection::SetDoubleArg(PRFloat64 val)
{
    LOG(("Set Double %d", val));
    DBusMessageIter* args = outgoing_args.back();

    double data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_DOUBLE, &data);

    LOG(("Set Double"));
    return NS_OK;
}

/* void setInt64Arg (in long long val); */
NS_IMETHODIMP ngDbusConnection::SetInt64Arg(PRInt64 val)
{
    LOG(("Set Int64 %d", val));
    DBusMessageIter* args = outgoing_args.back();

    dbus_int64_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_INT64, &data);

    LOG(("Set Int64"));
    return NS_OK;
}

/* void setArrayStringArg (in string key, in long val); */
NS_IMETHODIMP ngDbusConnection::SetArrayStringArg(const char* val)
{
    NS_ENSURE_ARG_POINTER(val);
    return this->SetStringArg(val);
}

/* void setObjectPathArg (in string val); */
//NS_IMETHODIMP ngDbusConnection::SetObjectPathArg(const nsAString &val)
NS_IMETHODIMP ngDbusConnection::SetObjectPathArg(const char *data)
{
    NS_ENSURE_ARG_POINTER(data);
    DBusMessageIter* args = outgoing_args.back();

    LOG(("Setting object path %s", data));

    dbus_message_iter_append_basic(args, DBUS_TYPE_OBJECT_PATH, &data);

    LOG(("Set object path"));
    return NS_OK;
}

/* void openDictEntryArray (); */
NS_IMETHODIMP ngDbusConnection::OpenDictEntryArray()
{
    LOG(("Opening dict entry"));

    DBusMessageIter* args = outgoing_args.back();
    DBusMessageIter* new_args = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    dbus_message_iter_open_container(args, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, new_args);

    outgoing_args.push_back(new_args);

    LOG(("Dict entry open"));
    return NS_OK;
}

/* void closeDictEntryArray (); */
NS_IMETHODIMP ngDbusConnection::CloseDictEntryArray()
{
    LOG(("Closing dict entry"));

    DBusMessageIter* new_args = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* args = outgoing_args.back();

    dbus_message_iter_close_container(args, new_args);

    LOG(("Dict entry closed"));
    return NS_OK;
}

/* void openArray (); */
NS_IMETHODIMP ngDbusConnection::OpenArray(PRInt16 aType = TYPE_STRING)
{
    LOG(("Opening array"));

    DBusMessageIter* args = outgoing_args.back();
    DBusMessageIter* new_args = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    dbus_message_iter_open_container(args, DBUS_TYPE_ARRAY, this->ngTypeToDBusType(aType), new_args);

    outgoing_args.push_back(new_args);

    LOG(("Array open"));
    return NS_OK;
}

/* void closeArray (); */
NS_IMETHODIMP ngDbusConnection::CloseArray()
{
    LOG(("Closing array"));

    DBusMessageIter* new_args = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* args = outgoing_args.back();

    dbus_message_iter_close_container(args, new_args);

    LOG(("Array closed"));
    return NS_OK;
}

/* void openStruct (); */
NS_IMETHODIMP ngDbusConnection::OpenStruct()
{
    LOG(("Opening struct"));

    DBusMessageIter* args = outgoing_args.back();
    DBusMessageIter* new_args = (DBusMessageIter*)NS_Alloc(sizeof(DBusMessageIter));

    dbus_message_iter_open_container(args, DBUS_TYPE_STRUCT, NULL, new_args);

    outgoing_args.push_back(new_args);
    LOG(("Struct opened"));

    return NS_OK;
}

/* void closeStruct (); */
NS_IMETHODIMP ngDbusConnection::CloseStruct()
{
    LOG(("Closing struct"));
    DBusMessageIter* new_args = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* args = outgoing_args.back();

    dbus_message_iter_close_container(args, new_args);
    LOG(("Struct closed"));

    return NS_OK;
}

const char* ngDbusConnection::ngTypeToDBusType(const int ngType) const
{
    if(ngType == TYPE_OBJECT_PATH) {
        return DBUS_TYPE_OBJECT_PATH_AS_STRING;
    }

    return DBUS_TYPE_STRING_AS_STRING;
}
