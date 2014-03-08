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


#include "ngDbusConnection.h"


#define DEBUG false
#define DEBUG_CERR false

using namespace std;


/* Implementation file */
NS_IMPL_ISUPPORTS1(ngDbusConnection, ngIDbusConnection)

ngDbusConnection::ngDbusConnection()
{
  /* member initializers and constructor code */
}

ngDbusConnection::~ngDbusConnection()
{
  /* destructor code */
}


/* attribute boolean debug_mode; */
NS_IMETHODIMP ngDbusConnection::GetDebug_mode(PRBool *aDebug_mode)
{
    *aDebug_mode = debug_mode;
    return NS_OK;
}
NS_IMETHODIMP ngDbusConnection::SetDebug_mode(PRBool aDebug_mode)
{
    debug_mode = aDebug_mode;
    
    if(debug_mode) cout << "MPRIS: DEBUG MODE Set" << endl;
    else cout << "MPRIS: DEBUG MODE Unset" << endl;
    return NS_OK;
}


/* long init (in string name); */
NS_IMETHODIMP ngDbusConnection::Init(const char *name, PRInt32 *_retval)
{
    if(debug_mode) cout << "-----------MPRIS: DEBUG MODE------------" << endl;

    *_retval = 0;
    DBusError error;
    
    if(DEBUG || debug_mode) cout << "Initializing DBus" << endl;
    
    dbus_error_init (&error);
    conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
    if (!conn) {
      if(DEBUG_CERR || debug_mode) cerr <<  "Mpris Module: " << error.name << ": " << error.message; 
      dbus_error_free(&error);
      *_retval = 1;
    }

    if(DEBUG || debug_mode) cout << "Requesting Name on DBus" << endl;
    dbus_bus_request_name(conn, name, 0, &error);
    if (dbus_error_is_set (&error)) {
      if(DEBUG_CERR || debug_mode) cerr <<  "Mpris Module: " << error.name << ": " << error.message;
      dbus_error_free(&error); 
      *_retval = 1;
    }
    
    if(DEBUG || debug_mode) cout << "Flushing" << endl;
    
    dbus_connection_flush(conn);
    if(DEBUG || debug_mode) cout << "End Init()" << endl;
    
    return NS_OK;
}

/* long setMatch (in string match); */
NS_IMETHODIMP ngDbusConnection::SetMatch(const char *match)
{
    DBusError error;
    dbus_error_init (&error);
    if(DEBUG || debug_mode) cout << "Setting match " << match << endl;
  
      /* listening to messages from all objects as no path is specified */
    dbus_bus_add_match (conn, match, &error);
    //~ dbus_bus_add_match (conn, match, NULL);
    
    if (dbus_error_is_set (&error)) {
      if(DEBUG_CERR || debug_mode) cerr <<  "Match Error: " << error.name << ": " << error.message;
      dbus_error_free(&error); 
    }
    
    
    dbus_connection_flush(conn);
  
    if(DEBUG || debug_mode) cout << "DBus Connection Init" << endl;
    
    return NS_OK;
}

/* void check (); */
NS_IMETHODIMP ngDbusConnection::Check()
{
    if(conn == NULL) return NS_OK;
 
    //~ int i;
    //~ for(i = 0; i < 30; i++){
	//~ cout << NS_ConvertUTF16toUTF8("Flamboyant").get() << endl;
    //~ }


    
    DBusMessage* msg;
    DBusMessage* reply;
    DBusMessageIter *reply_iter;

    // non blocking read of the next available message
    dbus_connection_read_write(conn, 0);
    msg = dbus_connection_pop_message(conn);

    if(msg != NULL){
      if(dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_METHOD_CALL || dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL){
	    if(DEBUG || debug_mode) cout << "*****Calling " << dbus_message_get_member(msg) << endl;
	
	    reply_iter = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
	
	    dbus_message_iter_init(msg, &incoming_args);
	
	    reply = dbus_message_new_method_return(msg);
	    dbus_message_iter_init_append(reply, reply_iter);
	
	    outgoing_args.push_back(reply_iter);
	
	
	    const char* interface = dbus_message_get_interface(msg);
	    const char* path = dbus_message_get_path(msg);
	    const char* member = dbus_message_get_member(msg);
	      
	    if(DEBUG || debug_mode){
	        
	        if(interface != NULL) cout << "Interface: " << interface << endl;
	        if(path != NULL) cout << "Path: " << path << endl;
	        if(member != NULL) cout << "Member: " << member << endl;
	    }
	
	    handler->HandleMethod(interface, path, member); 
	
	    if(DEBUG || debug_mode) cout << "Just after Handler call" << endl;
	
	    dbus_uint32_t serial = 0;

	    // send the reply && flush the connection
	    if (!dbus_connection_send(conn, reply, &serial)) { 
	      if(DEBUG_CERR || debug_mode) cerr << "Out Of Memory!" << endl; 
	      return NS_OK;
	    }
	
	    while(!outgoing_args.empty()){
	        free(outgoing_args.back());
	        outgoing_args.pop_back();
	    }
	
	    dbus_connection_flush(conn);
	    dbus_message_unref(reply);
	
	
	    if(DEBUG || debug_mode) cout << "Done Handler call" << endl;
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
  if(DEBUG || debug_mode) cout << "*****Preparing method call " << path << ", " << inter << ", " << name << endl;
  DBusMessageIter* reply_iter = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));

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
    if(DEBUG || debug_mode) cout <<  "Starting to send method call" << endl;
    
    dbus_connection_send(conn, signal_msg, NULL);
  
    dbus_message_unref(signal_msg);
  
    if(DEBUG || debug_mode) cout <<  "Freeing args" << endl;
    while(!outgoing_args.empty()){
	free(outgoing_args.back());
	outgoing_args.pop_back();
    }

    dbus_connection_flush(conn);
    
    if(DEBUG || debug_mode) cout <<  "*****Method Call Sent" << endl;
    
    return NS_OK;
}

/* void sprepareSignal (in string path, in string inter, in string name); */
NS_IMETHODIMP ngDbusConnection::PrepareSignal(const char *path, const char *inter, const char *name)
{
    
    if(DEBUG || debug_mode) cout << "*****Preparing signal " << path << ", " << inter << ", " << name << endl;
    signal_msg = dbus_message_new_signal(path, inter, name);
    
    DBusMessageIter* reply_iter = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
    
    dbus_message_iter_init_append(signal_msg, reply_iter);
    outgoing_args.push_back(reply_iter);
    
    if(DEBUG || debug_mode) cout << "Signal prepared" << endl;
  
    return NS_OK;
}

/* void sendSignal (); */
NS_IMETHODIMP ngDbusConnection::SendSignal()
{
    if(DEBUG || debug_mode) cout <<  "Starting to send signal" << endl;
    
    dbus_connection_send(conn, signal_msg, NULL);
  
    dbus_message_unref(signal_msg);
  
    if(DEBUG || debug_mode) cout <<  "Freeing args" << endl;
    while(!outgoing_args.empty()){
	free(outgoing_args.back());
	outgoing_args.pop_back();
    }

    dbus_connection_flush(conn);
    
    if(DEBUG || debug_mode) cout <<  "*****Signal Sent" << endl;
    
    return NS_OK;
}
/* long setMethodHandler (in ngIMethodHandler handler); */
NS_IMETHODIMP ngDbusConnection::SetMethodHandler(ngIMethodHandler *handler)
{
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
      if(DEBUG_CERR || debug_mode) cerr << "WARNING: '" << (char)type << "' recieved but expecting a Int64" << endl;
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
      if(DEBUG_CERR || debug_mode) cerr << "WARNING: '" << (char)type << "' recieved but expecting a Boolean" << endl;
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
      *_retval = (char*)malloc(sizeof(char)*(strlen(dat) + 1));
      
      strcpy(*_retval, dat);
      if(DEBUG_CERR || debug_mode) cout << "String contained " << *_retval << endl;
      
      dbus_message_iter_next(&incoming_args);
      
    }
    else{
      if(DEBUG_CERR || debug_mode) cerr << "WARNING: '" << (char)type << "' recieved but expecting a String" << endl;
      
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
      *_retval = (char*)malloc(sizeof(char)*(strlen(dat) + 1));
      
      strcpy(*_retval, dat);
      
      dbus_message_iter_next(&incoming_args);
      
    }
    else{
      if(DEBUG_CERR || debug_mode) cerr << "WARNING: '" << (char)type << "' recieved but expecting an Object" << endl;
      
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
      if(DEBUG_CERR || debug_mode) cerr << "WARNING: '" << (char)type << "' recieved but expecting a Double" << endl;
    }
  
    return NS_OK;
}

/* void setInt32Arg (in long val); */
NS_IMETHODIMP ngDbusConnection::SetInt32Arg(PRInt32 val)
{
    if(DEBUG || debug_mode) cout << "Setting Int32 " << val << endl;
    DBusMessageIter* args = outgoing_args.back();
  
    dbus_int32_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_INT32, &data);
  
    if(DEBUG || debug_mode) cout << "Set Int32" << endl;
  
    return NS_OK;
}

/* void setUInt32Arg (in long val); */
NS_IMETHODIMP ngDbusConnection::SetUInt32Arg(PRUint32 val)
{
    if(DEBUG || debug_mode) cout << "Setting uInt32 " << val << endl;
    
    DBusMessageIter* args = outgoing_args.back();
  
    dbus_message_iter_append_basic(args, DBUS_TYPE_UINT32, &val);
  
    if(DEBUG || debug_mode) cout << "Set uInt32" << endl;
  
    return NS_OK;
}

/* void setUInt16Arg (in long val); */
NS_IMETHODIMP ngDbusConnection::SetUInt16Arg(PRUint16 val)
{
    if(DEBUG || debug_mode) cout <<"Set uInt16 " << val << endl;
    DBusMessageIter* args = outgoing_args.back();
  
    dbus_uint16_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_UINT16, &data);
  
    if(DEBUG || debug_mode) cout <<"Set uInt16" << endl;
    return NS_OK;
}

/* void setStringArg (in string val); */
//NS_IMETHODIMP ngDbusConnection::SetStringArg(const nsAString &data)
NS_IMETHODIMP ngDbusConnection::SetStringArg(const char *data)
{
    if(DEBUG || debug_mode) cout << "Setting string " << data << endl;
    DBusMessageIter* args = outgoing_args.back();
    
    dbus_message_iter_append_basic(args, DBUS_TYPE_STRING, &data);
  
    if(DEBUG || debug_mode) cout << "Set string" << endl;
    return NS_OK;
}

/* void setDictSSEntryArg (in string key, in AString val, [optional] in boolean escape); */
NS_IMETHODIMP ngDbusConnection::SetDictSSEntryArg(const char *key, const nsAString &val)
{
    
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;
    
    char* data = ToNewUTF8String(val);
    
    if(DEBUG || debug_mode) cout << "Setting dict SS " << key << ":" << data << endl;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);  
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_STRING_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_STRING, &data);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);
  
    if(DEBUG || debug_mode) cout << "Set dict SS entry" << endl;
    
    return NS_OK;
}

/* void setDictSOEntryArg (in string key, in AString val, [optional] in boolean escape); */
NS_IMETHODIMP ngDbusConnection::SetDictSOEntryArg(const char *key, const char* data)
{
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;
    
    if(DEBUG || debug_mode) cout << "Setting dict SO " << key << ":" << data << endl;

    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);  
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_OBJECT_PATH_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_OBJECT_PATH, &data);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);
  
    if(DEBUG || debug_mode) cout << "Set dict SO entry" << endl;
    
    return NS_OK;
}

/* void setDictSIEntryArg (in string key, in long long val); */
NS_IMETHODIMP ngDbusConnection::SetDictSI64EntryArg(const char *key, PRInt64 val)
{
    if(DEBUG || debug_mode) cout << "Setting dict SI64 " << key << ":" << val << endl;
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;
  
    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_INT64_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_INT64, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);
    
    if(DEBUG || debug_mode) cout << "Set dict SI64 entry" << endl;
  
    return NS_OK;
}

/* void setDictSIEntryArg (in string key, in long val); */
NS_IMETHODIMP ngDbusConnection::SetDictSIEntryArg(const char *key, PRUint32 val)
{
    if(DEBUG || debug_mode) cout << "Setting dict SI " << key << ":" << val << endl;
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;
  
    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_UINT32_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_UINT32, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);
    
    if(DEBUG || debug_mode) cout << "Set dict SI entry" << endl;
  
    return NS_OK;
}

/* void setDictSBEntryArg (in string key, in boolean val); */
NS_IMETHODIMP ngDbusConnection::SetDictSBEntryArg(const char *key, PRBool val)
{
    if(DEBUG || debug_mode) cout << "Setting dict SB " << key << ":" << val << endl;
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;
  
    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_BOOLEAN_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_BOOLEAN, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);
    
    if(DEBUG || debug_mode) cout << "Set dict SB entry" << endl;
  
    return NS_OK;
}

/* void setDictSDEntryArg (in string key, in double val); */
NS_IMETHODIMP ngDbusConnection::SetDictSDEntryArg(const char *key, PRFloat64 val)
{
    if(DEBUG || debug_mode) cout << "Setting dict SD " << key << ":" << val << endl;
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter entry_obj;
    DBusMessageIter var_obj;
  
    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, &entry_obj);
      dbus_message_iter_append_basic(&entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(&entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_DOUBLE_AS_STRING, &var_obj);
	dbus_message_iter_append_basic(&var_obj, DBUS_TYPE_DOUBLE, &val);
      dbus_message_iter_close_container(&entry_obj, &var_obj);
    dbus_message_iter_close_container(array_obj, &entry_obj);
    
    if(DEBUG || debug_mode) cout << "Set dict SD entry" << endl;
  
    return NS_OK;
}

/* void openDictSAEntryArg (in string key); */
NS_IMETHODIMP ngDbusConnection::OpenDictSAEntryArg(const char *key)
{
    if(DEBUG || debug_mode) cout << "Opening dict SA :" << key << endl;
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter* entry_obj = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));

    DBusMessageIter* new_val = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
    DBusMessageIter* new_arr = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
  
    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, entry_obj);
      dbus_message_iter_append_basic(entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_ARRAY_AS_STRING DBUS_TYPE_STRING_AS_STRING, new_val);
    dbus_message_iter_open_container(new_val, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, new_arr);

    outgoing_args.push_back(entry_obj);
    outgoing_args.push_back(new_val);
    outgoing_args.push_back(new_arr);
    
    if(DEBUG || debug_mode) cout << "Opened dict SA entry" << endl;
  
    return NS_OK;
}

/* void closeDictSAEntryArg (); */
NS_IMETHODIMP ngDbusConnection::CloseDictSAEntryArg()
{
    if(DEBUG || debug_mode) cout << "Closing dict SA " << endl;

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
    
    if(DEBUG || debug_mode) cout << "Closed dict SA entry" << endl;
  
    return NS_OK;
}
/* void openDictSDEntryArg (in string key); */
NS_IMETHODIMP ngDbusConnection::OpenDictSDEntryArg(const char *key)
{
    if(DEBUG || debug_mode) cout << "Opening dict SD :" << key << endl;
    
    DBusMessageIter* array_obj = outgoing_args.back();
    DBusMessageIter* entry_obj = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));

    DBusMessageIter* new_val = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
    DBusMessageIter* new_arr = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
  
    dbus_message_iter_open_container(array_obj, DBUS_TYPE_DICT_ENTRY, NULL, entry_obj);
      dbus_message_iter_append_basic(entry_obj, DBUS_TYPE_STRING, &key);
      
      dbus_message_iter_open_container(entry_obj, DBUS_TYPE_VARIANT, DBUS_TYPE_ARRAY_AS_STRING DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, new_val);
    dbus_message_iter_open_container(new_val, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, new_arr);

    outgoing_args.push_back(entry_obj);
    outgoing_args.push_back(new_val);
    outgoing_args.push_back(new_arr);
    
    if(DEBUG || debug_mode) cout << "Opened dict SD entry" << endl;
  
    return NS_OK;
}

/* void closeDictSDEntryArg (); */
NS_IMETHODIMP ngDbusConnection::CloseDictSDEntryArg()
{
    if(DEBUG || debug_mode) cout << "Closing dict SD " << endl;

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
    
    if(DEBUG || debug_mode) cout << "Closed dict SD entry" << endl;
  
    return NS_OK;
}


/* void setBoolArg (in bool val); */
NS_IMETHODIMP ngDbusConnection::SetBoolArg(PRBool val)
{
    if(DEBUG || debug_mode) cout << "Setting Bool " << val << endl;
    DBusMessageIter* args = outgoing_args.back();
  
    dbus_bool_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_BOOLEAN, &data);
  
    if(DEBUG || debug_mode) cout << "Set Bool" << endl;
  
    return NS_OK;
}

/* void setDoubleArg (in double val); */
NS_IMETHODIMP ngDbusConnection::SetDoubleArg(PRFloat64 val)
{
    if(DEBUG || debug_mode) cout <<"Set Double " << val << endl;
    DBusMessageIter* args = outgoing_args.back();
  
    double data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_DOUBLE, &data);
  
    if(DEBUG || debug_mode) cout <<"Set Double" << endl;
    return NS_OK;
}

/* void setInt64Arg (in long long val); */
NS_IMETHODIMP ngDbusConnection::SetInt64Arg(PRInt64 val)
{
    if(DEBUG || debug_mode) cout <<"Set Int64 " << val << endl;
    DBusMessageIter* args = outgoing_args.back();
  
    dbus_int64_t data = val;
    dbus_message_iter_append_basic(args, DBUS_TYPE_INT64, &data);
  
    if(DEBUG || debug_mode) cout <<"Set Int64" << endl;
    return NS_OK;
}

/* void setArrayStringArg (in string key, in long val); */
NS_IMETHODIMP ngDbusConnection::SetArrayStringArg(const char* val)
{
    if(DEBUG || debug_mode) cout << "Setting array S " << val << endl;
 
    DBusMessageIter* array_obj = outgoing_args.back();

	dbus_message_iter_append_basic(array_obj, DBUS_TYPE_STRING, &val);
    
    if(DEBUG || debug_mode) cout << "Set array S entry" << endl;
  
    return NS_OK;
}

/* void setObjectPathArg (in string val); */
//NS_IMETHODIMP ngDbusConnection::SetObjectPathArg(const nsAString &val)
NS_IMETHODIMP ngDbusConnection::SetObjectPathArg(const char *data)
{
    DBusMessageIter* args = outgoing_args.back();

    if(DEBUG || debug_mode) cout << "Setting object path " << data << endl;
    
    dbus_message_iter_append_basic(args, DBUS_TYPE_OBJECT_PATH, &data);
  
    if(DEBUG || debug_mode) cout << "Set object path" << endl;
    return NS_OK;
}

/* void openDictEntryArray (); */
NS_IMETHODIMP ngDbusConnection::OpenDictEntryArray()
{
    if(DEBUG || debug_mode) cout << "Opening dict entry" << endl;
    
    DBusMessageIter* args = outgoing_args.back();
    DBusMessageIter* new_args = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
  
    dbus_message_iter_open_container(args, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, new_args);
  
    outgoing_args.push_back(new_args);
    
    if(DEBUG || debug_mode) cout << "Dict entry open" << endl;
    return NS_OK;
}

/* void closeDictEntryArray (); */
NS_IMETHODIMP ngDbusConnection::CloseDictEntryArray()
{
    if(DEBUG || debug_mode) cout << "Closing dict entry" << endl;
    
    DBusMessageIter* new_args = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* args = outgoing_args.back();
    
    dbus_message_iter_close_container(args, new_args);
  
    if(DEBUG || debug_mode) cout << "Dict entry closed" << endl;
    return NS_OK;
}

/* void openArray (); */
NS_IMETHODIMP ngDbusConnection::OpenArray()
{
    if(DEBUG || debug_mode) cout << "Opening array" << endl;

    DBusMessageIter* args = outgoing_args.back();
    DBusMessageIter* new_args = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
  
    dbus_message_iter_open_container(args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, new_args);
  
    outgoing_args.push_back(new_args);
    
    if(DEBUG || debug_mode) cout << "Array open" << endl;
    return NS_OK;
}

/* void closeArray (); */
NS_IMETHODIMP ngDbusConnection::CloseArray()
{
    if(DEBUG || debug_mode) cout << "Closing array" << endl;

    DBusMessageIter* new_args = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* args = outgoing_args.back();
    
    dbus_message_iter_close_container(args, new_args);

    if(DEBUG || debug_mode) cout << "Array closed" << endl;
    return NS_OK;
}

/* void openStruct (); */
NS_IMETHODIMP ngDbusConnection::OpenStruct()
{
    if(DEBUG || debug_mode) cout << "Opening struct" << endl;
    
    DBusMessageIter* args = outgoing_args.back();
    DBusMessageIter* new_args = (DBusMessageIter*)malloc(sizeof(DBusMessageIter));
  
    dbus_message_iter_open_container(args, DBUS_TYPE_STRUCT, NULL, new_args);
    
    outgoing_args.push_back(new_args);
    if(DEBUG || debug_mode) cout << "Struct opened" << endl;
  
    return NS_OK;
}

/* void closeStruct (); */
NS_IMETHODIMP ngDbusConnection::CloseStruct()
{
    if(DEBUG || debug_mode) cout << "Closing struct" << endl;
    DBusMessageIter* new_args = outgoing_args.back();
    outgoing_args.pop_back();
    DBusMessageIter* args = outgoing_args.back();
    
    dbus_message_iter_close_container(args, new_args);
    if(DEBUG || debug_mode) cout << "Struct closed" << endl;
  
    return NS_OK;
}
