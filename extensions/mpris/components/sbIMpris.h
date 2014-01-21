/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM sbIMpris.idl
 */

#ifndef __gen_sbIMpris_h__
#define __gen_sbIMpris_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class sbIDbusConnection; /* forward declaration */


/* starting interface:    sbIMethodHandler */
#define SBIMETHODHANDLER_IID_STR "bc3f9c62-9a5f-47e8-a00f-49d80f3bb6c9"

#define SBIMETHODHANDLER_IID \
  {0xbc3f9c62, 0x9a5f, 0x47e8, \
    { 0xa0, 0x0f, 0x49, 0xd8, 0x0f, 0x3b, 0xb6, 0xc9 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMethodHandler : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMETHODHANDLER_IID)

  /* void handleMethod (in string interface_name, in string path, in string member); */
  NS_SCRIPTABLE NS_IMETHOD HandleMethod(const char *interface_name, const char *path, const char *member) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMethodHandler, SBIMETHODHANDLER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMETHODHANDLER \
  NS_SCRIPTABLE NS_IMETHOD HandleMethod(const char *interface_name, const char *path, const char *member); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMETHODHANDLER(_to) \
  NS_SCRIPTABLE NS_IMETHOD HandleMethod(const char *interface_name, const char *path, const char *member) { return _to HandleMethod(interface_name, path, member); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMETHODHANDLER(_to) \
  NS_SCRIPTABLE NS_IMETHOD HandleMethod(const char *interface_name, const char *path, const char *member) { return !_to ? NS_ERROR_NULL_POINTER : _to->HandleMethod(interface_name, path, member); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMethodHandler : public sbIMethodHandler
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMETHODHANDLER

  sbMethodHandler();

private:
  ~sbMethodHandler();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMethodHandler, sbIMethodHandler)

sbMethodHandler::sbMethodHandler()
{
  /* member initializers and constructor code */
}

sbMethodHandler::~sbMethodHandler()
{
  /* destructor code */
}

/* void handleMethod (in string interface_name, in string path, in string member); */
NS_IMETHODIMP sbMethodHandler::HandleMethod(const char *interface_name, const char *path, const char *member)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMprisPlugin */
#define SBIMPRISPLUGIN_IID_STR "a8159f8d-7970-40d0-bd08-8d0adf69e435"

#define SBIMPRISPLUGIN_IID \
  {0xa8159f8d, 0x7970, 0x40d0, \
    { 0xbd, 0x08, 0x8d, 0x0a, 0xdf, 0x69, 0xe4, 0x35 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMprisPlugin : public sbIMethodHandler {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMPRISPLUGIN_IID)

  /* attribute sbIDbusConnection dbus; */
  NS_SCRIPTABLE NS_IMETHOD GetDbus(sbIDbusConnection * *aDbus) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetDbus(sbIDbusConnection * aDbus) = 0;

  /* attribute boolean debug_mode; */
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode) = 0;

  /* void init (in boolean debug); */
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool debug) = 0;

  /* void getMetadata (in long track_num); */
  NS_SCRIPTABLE NS_IMETHOD GetMetadata(PRInt32 track_num) = 0;

  /* void setMetadata (in long track_num); */
  NS_SCRIPTABLE NS_IMETHOD SetMetadata(PRInt32 track_num) = 0;

  /* void getStatus (); */
  NS_SCRIPTABLE NS_IMETHOD GetStatus(void) = 0;

  /* void getCaps (); */
  NS_SCRIPTABLE NS_IMETHOD GetCaps(void) = 0;

  /* void addTrack (in AString uri, in boolean play_now); */
  NS_SCRIPTABLE NS_IMETHOD AddTrack(const nsAString & uri, PRBool play_now) = 0;

  /* void delTrack (in long track_num); */
  NS_SCRIPTABLE NS_IMETHOD DelTrack(PRInt32 track_num) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMprisPlugin, SBIMPRISPLUGIN_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMPRISPLUGIN \
  NS_SCRIPTABLE NS_IMETHOD GetDbus(sbIDbusConnection * *aDbus); \
  NS_SCRIPTABLE NS_IMETHOD SetDbus(sbIDbusConnection * aDbus); \
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode); \
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode); \
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool debug); \
  NS_SCRIPTABLE NS_IMETHOD GetMetadata(PRInt32 track_num); \
  NS_SCRIPTABLE NS_IMETHOD SetMetadata(PRInt32 track_num); \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(void); \
  NS_SCRIPTABLE NS_IMETHOD GetCaps(void); \
  NS_SCRIPTABLE NS_IMETHOD AddTrack(const nsAString & uri, PRBool play_now); \
  NS_SCRIPTABLE NS_IMETHOD DelTrack(PRInt32 track_num); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMPRISPLUGIN(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetDbus(sbIDbusConnection * *aDbus) { return _to GetDbus(aDbus); } \
  NS_SCRIPTABLE NS_IMETHOD SetDbus(sbIDbusConnection * aDbus) { return _to SetDbus(aDbus); } \
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode) { return _to GetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode) { return _to SetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool debug) { return _to Init(debug); } \
  NS_SCRIPTABLE NS_IMETHOD GetMetadata(PRInt32 track_num) { return _to GetMetadata(track_num); } \
  NS_SCRIPTABLE NS_IMETHOD SetMetadata(PRInt32 track_num) { return _to SetMetadata(track_num); } \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(void) { return _to GetStatus(); } \
  NS_SCRIPTABLE NS_IMETHOD GetCaps(void) { return _to GetCaps(); } \
  NS_SCRIPTABLE NS_IMETHOD AddTrack(const nsAString & uri, PRBool play_now) { return _to AddTrack(uri, play_now); } \
  NS_SCRIPTABLE NS_IMETHOD DelTrack(PRInt32 track_num) { return _to DelTrack(track_num); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMPRISPLUGIN(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetDbus(sbIDbusConnection * *aDbus) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDbus(aDbus); } \
  NS_SCRIPTABLE NS_IMETHOD SetDbus(sbIDbusConnection * aDbus) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDbus(aDbus); } \
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD Init(PRBool debug) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(debug); } \
  NS_SCRIPTABLE NS_IMETHOD GetMetadata(PRInt32 track_num) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetMetadata(track_num); } \
  NS_SCRIPTABLE NS_IMETHOD SetMetadata(PRInt32 track_num) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMetadata(track_num); } \
  NS_SCRIPTABLE NS_IMETHOD GetStatus(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetStatus(); } \
  NS_SCRIPTABLE NS_IMETHOD GetCaps(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetCaps(); } \
  NS_SCRIPTABLE NS_IMETHOD AddTrack(const nsAString & uri, PRBool play_now) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddTrack(uri, play_now); } \
  NS_SCRIPTABLE NS_IMETHOD DelTrack(PRInt32 track_num) { return !_to ? NS_ERROR_NULL_POINTER : _to->DelTrack(track_num); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMprisPlugin : public sbIMprisPlugin
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMPRISPLUGIN

  sbMprisPlugin();

private:
  ~sbMprisPlugin();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMprisPlugin, sbIMprisPlugin)

sbMprisPlugin::sbMprisPlugin()
{
  /* member initializers and constructor code */
}

sbMprisPlugin::~sbMprisPlugin()
{
  /* destructor code */
}

/* attribute sbIDbusConnection dbus; */
NS_IMETHODIMP sbMprisPlugin::GetDbus(sbIDbusConnection * *aDbus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbMprisPlugin::SetDbus(sbIDbusConnection * aDbus)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute boolean debug_mode; */
NS_IMETHODIMP sbMprisPlugin::GetDebug_mode(PRBool *aDebug_mode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbMprisPlugin::SetDebug_mode(PRBool aDebug_mode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void init (in boolean debug); */
NS_IMETHODIMP sbMprisPlugin::Init(PRBool debug)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getMetadata (in long track_num); */
NS_IMETHODIMP sbMprisPlugin::GetMetadata(PRInt32 track_num)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setMetadata (in long track_num); */
NS_IMETHODIMP sbMprisPlugin::SetMetadata(PRInt32 track_num)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getStatus (); */
NS_IMETHODIMP sbMprisPlugin::GetStatus()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getCaps (); */
NS_IMETHODIMP sbMprisPlugin::GetCaps()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addTrack (in AString uri, in boolean play_now); */
NS_IMETHODIMP sbMprisPlugin::AddTrack(const nsAString & uri, PRBool play_now)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void delTrack (in long track_num); */
NS_IMETHODIMP sbMprisPlugin::DelTrack(PRInt32 track_num)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIDbusConnection */
#define SBIDBUSCONNECTION_IID_STR "4c809340-0f6f-4122-b242-d4e900bed92b"

#define SBIDBUSCONNECTION_IID \
  {0x4c809340, 0x0f6f, 0x4122, \
    { 0xb2, 0x42, 0xd4, 0xe9, 0x00, 0xbe, 0xd9, 0x2b }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIDbusConnection : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIDBUSCONNECTION_IID)

  /* attribute boolean debug_mode; */
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode) = 0;
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode) = 0;

  /* long init (in string name, in boolean debug); */
  NS_SCRIPTABLE NS_IMETHOD Init(const char *name, PRBool debug, PRInt32 *_retval NS_OUTPARAM) = 0;

  /* void setMatch (in string match); */
  NS_SCRIPTABLE NS_IMETHOD SetMatch(const char *match) = 0;

  /* void check (); */
  NS_SCRIPTABLE NS_IMETHOD Check(void) = 0;

  /* long end (); */
  NS_SCRIPTABLE NS_IMETHOD End(PRInt32 *_retval NS_OUTPARAM) = 0;

  /* void prepareSignal (in string path, in string inter, in string name); */
  NS_SCRIPTABLE NS_IMETHOD PrepareSignal(const char *path, const char *inter, const char *name) = 0;

  /* void sendSignal (); */
  NS_SCRIPTABLE NS_IMETHOD SendSignal(void) = 0;

  /* void setMethodHandler (in sbIMethodHandler handler); */
  NS_SCRIPTABLE NS_IMETHOD SetMethodHandler(sbIMethodHandler *handler) = 0;

  /* long getInt32Arg (); */
  NS_SCRIPTABLE NS_IMETHOD GetInt32Arg(PRInt32 *_retval NS_OUTPARAM) = 0;

  /* boolean getBoolArg (); */
  NS_SCRIPTABLE NS_IMETHOD GetBoolArg(PRBool *_retval NS_OUTPARAM) = 0;

  /* string getStringArg (); */
  NS_SCRIPTABLE NS_IMETHOD GetStringArg(char **_retval NS_OUTPARAM) = 0;

  /* void setUInt32Arg (in unsigned long val); */
  NS_SCRIPTABLE NS_IMETHOD SetUInt32Arg(PRUint32 val) = 0;

  /* void setInt32Arg (in long val); */
  NS_SCRIPTABLE NS_IMETHOD SetInt32Arg(PRInt32 val) = 0;

  /* void setUInt16Arg (in unsigned short val); */
  NS_SCRIPTABLE NS_IMETHOD SetUInt16Arg(PRUint16 val) = 0;

  /* void setStringArg (in string val); */
  NS_SCRIPTABLE NS_IMETHOD SetStringArg(const char *val) = 0;

  /* void setDictSSEntryArg (in string key, in AString val); */
  NS_SCRIPTABLE NS_IMETHOD SetDictSSEntryArg(const char *key, const nsAString & val) = 0;

  /* void setDictSIEntryArg (in string key, in unsigned long val); */
  NS_SCRIPTABLE NS_IMETHOD SetDictSIEntryArg(const char *key, PRUint32 val) = 0;

  /* void openDictEntryArray (); */
  NS_SCRIPTABLE NS_IMETHOD OpenDictEntryArray(void) = 0;

  /* void closeDictEntryArray (); */
  NS_SCRIPTABLE NS_IMETHOD CloseDictEntryArray(void) = 0;

  /* void openStruct (); */
  NS_SCRIPTABLE NS_IMETHOD OpenStruct(void) = 0;

  /* void closeStruct (); */
  NS_SCRIPTABLE NS_IMETHOD CloseStruct(void) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIDbusConnection, SBIDBUSCONNECTION_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIDBUSCONNECTION \
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode); \
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode); \
  NS_SCRIPTABLE NS_IMETHOD Init(const char *name, PRBool debug, PRInt32 *_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD SetMatch(const char *match); \
  NS_SCRIPTABLE NS_IMETHOD Check(void); \
  NS_SCRIPTABLE NS_IMETHOD End(PRInt32 *_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD PrepareSignal(const char *path, const char *inter, const char *name); \
  NS_SCRIPTABLE NS_IMETHOD SendSignal(void); \
  NS_SCRIPTABLE NS_IMETHOD SetMethodHandler(sbIMethodHandler *handler); \
  NS_SCRIPTABLE NS_IMETHOD GetInt32Arg(PRInt32 *_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetBoolArg(PRBool *_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD GetStringArg(char **_retval NS_OUTPARAM); \
  NS_SCRIPTABLE NS_IMETHOD SetUInt32Arg(PRUint32 val); \
  NS_SCRIPTABLE NS_IMETHOD SetInt32Arg(PRInt32 val); \
  NS_SCRIPTABLE NS_IMETHOD SetUInt16Arg(PRUint16 val); \
  NS_SCRIPTABLE NS_IMETHOD SetStringArg(const char *val); \
  NS_SCRIPTABLE NS_IMETHOD SetDictSSEntryArg(const char *key, const nsAString & val); \
  NS_SCRIPTABLE NS_IMETHOD SetDictSIEntryArg(const char *key, PRUint32 val); \
  NS_SCRIPTABLE NS_IMETHOD OpenDictEntryArray(void); \
  NS_SCRIPTABLE NS_IMETHOD CloseDictEntryArray(void); \
  NS_SCRIPTABLE NS_IMETHOD OpenStruct(void); \
  NS_SCRIPTABLE NS_IMETHOD CloseStruct(void); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIDBUSCONNECTION(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode) { return _to GetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode) { return _to SetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD Init(const char *name, PRBool debug, PRInt32 *_retval NS_OUTPARAM) { return _to Init(name, debug, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD SetMatch(const char *match) { return _to SetMatch(match); } \
  NS_SCRIPTABLE NS_IMETHOD Check(void) { return _to Check(); } \
  NS_SCRIPTABLE NS_IMETHOD End(PRInt32 *_retval NS_OUTPARAM) { return _to End(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD PrepareSignal(const char *path, const char *inter, const char *name) { return _to PrepareSignal(path, inter, name); } \
  NS_SCRIPTABLE NS_IMETHOD SendSignal(void) { return _to SendSignal(); } \
  NS_SCRIPTABLE NS_IMETHOD SetMethodHandler(sbIMethodHandler *handler) { return _to SetMethodHandler(handler); } \
  NS_SCRIPTABLE NS_IMETHOD GetInt32Arg(PRInt32 *_retval NS_OUTPARAM) { return _to GetInt32Arg(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetBoolArg(PRBool *_retval NS_OUTPARAM) { return _to GetBoolArg(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetStringArg(char **_retval NS_OUTPARAM) { return _to GetStringArg(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD SetUInt32Arg(PRUint32 val) { return _to SetUInt32Arg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetInt32Arg(PRInt32 val) { return _to SetInt32Arg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetUInt16Arg(PRUint16 val) { return _to SetUInt16Arg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetStringArg(const char *val) { return _to SetStringArg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetDictSSEntryArg(const char *key, const nsAString & val) { return _to SetDictSSEntryArg(key, val); } \
  NS_SCRIPTABLE NS_IMETHOD SetDictSIEntryArg(const char *key, PRUint32 val) { return _to SetDictSIEntryArg(key, val); } \
  NS_SCRIPTABLE NS_IMETHOD OpenDictEntryArray(void) { return _to OpenDictEntryArray(); } \
  NS_SCRIPTABLE NS_IMETHOD CloseDictEntryArray(void) { return _to CloseDictEntryArray(); } \
  NS_SCRIPTABLE NS_IMETHOD OpenStruct(void) { return _to OpenStruct(); } \
  NS_SCRIPTABLE NS_IMETHOD CloseStruct(void) { return _to CloseStruct(); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIDBUSCONNECTION(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetDebug_mode(PRBool *aDebug_mode) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD SetDebug_mode(PRBool aDebug_mode) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDebug_mode(aDebug_mode); } \
  NS_SCRIPTABLE NS_IMETHOD Init(const char *name, PRBool debug, PRInt32 *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->Init(name, debug, _retval); } \
  NS_SCRIPTABLE NS_IMETHOD SetMatch(const char *match) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMatch(match); } \
  NS_SCRIPTABLE NS_IMETHOD Check(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->Check(); } \
  NS_SCRIPTABLE NS_IMETHOD End(PRInt32 *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->End(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD PrepareSignal(const char *path, const char *inter, const char *name) { return !_to ? NS_ERROR_NULL_POINTER : _to->PrepareSignal(path, inter, name); } \
  NS_SCRIPTABLE NS_IMETHOD SendSignal(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->SendSignal(); } \
  NS_SCRIPTABLE NS_IMETHOD SetMethodHandler(sbIMethodHandler *handler) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetMethodHandler(handler); } \
  NS_SCRIPTABLE NS_IMETHOD GetInt32Arg(PRInt32 *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetInt32Arg(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetBoolArg(PRBool *_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetBoolArg(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD GetStringArg(char **_retval NS_OUTPARAM) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetStringArg(_retval); } \
  NS_SCRIPTABLE NS_IMETHOD SetUInt32Arg(PRUint32 val) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUInt32Arg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetInt32Arg(PRInt32 val) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetInt32Arg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetUInt16Arg(PRUint16 val) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetUInt16Arg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetStringArg(const char *val) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetStringArg(val); } \
  NS_SCRIPTABLE NS_IMETHOD SetDictSSEntryArg(const char *key, const nsAString & val) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDictSSEntryArg(key, val); } \
  NS_SCRIPTABLE NS_IMETHOD SetDictSIEntryArg(const char *key, PRUint32 val) { return !_to ? NS_ERROR_NULL_POINTER : _to->SetDictSIEntryArg(key, val); } \
  NS_SCRIPTABLE NS_IMETHOD OpenDictEntryArray(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->OpenDictEntryArray(); } \
  NS_SCRIPTABLE NS_IMETHOD CloseDictEntryArray(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->CloseDictEntryArray(); } \
  NS_SCRIPTABLE NS_IMETHOD OpenStruct(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->OpenStruct(); } \
  NS_SCRIPTABLE NS_IMETHOD CloseStruct(void) { return !_to ? NS_ERROR_NULL_POINTER : _to->CloseStruct(); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbDbusConnection : public sbIDbusConnection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDBUSCONNECTION

  sbDbusConnection();

private:
  ~sbDbusConnection();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbDbusConnection, sbIDbusConnection)

sbDbusConnection::sbDbusConnection()
{
  /* member initializers and constructor code */
}

sbDbusConnection::~sbDbusConnection()
{
  /* destructor code */
}

/* attribute boolean debug_mode; */
NS_IMETHODIMP sbDbusConnection::GetDebug_mode(PRBool *aDebug_mode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbDbusConnection::SetDebug_mode(PRBool aDebug_mode)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long init (in string name, in boolean debug); */
NS_IMETHODIMP sbDbusConnection::Init(const char *name, PRBool debug, PRInt32 *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setMatch (in string match); */
NS_IMETHODIMP sbDbusConnection::SetMatch(const char *match)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void check (); */
NS_IMETHODIMP sbDbusConnection::Check()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long end (); */
NS_IMETHODIMP sbDbusConnection::End(PRInt32 *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void prepareSignal (in string path, in string inter, in string name); */
NS_IMETHODIMP sbDbusConnection::PrepareSignal(const char *path, const char *inter, const char *name)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void sendSignal (); */
NS_IMETHODIMP sbDbusConnection::SendSignal()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setMethodHandler (in sbIMethodHandler handler); */
NS_IMETHODIMP sbDbusConnection::SetMethodHandler(sbIMethodHandler *handler)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* long getInt32Arg (); */
NS_IMETHODIMP sbDbusConnection::GetInt32Arg(PRInt32 *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* boolean getBoolArg (); */
NS_IMETHODIMP sbDbusConnection::GetBoolArg(PRBool *_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* string getStringArg (); */
NS_IMETHODIMP sbDbusConnection::GetStringArg(char **_retval NS_OUTPARAM)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setUInt32Arg (in unsigned long val); */
NS_IMETHODIMP sbDbusConnection::SetUInt32Arg(PRUint32 val)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setInt32Arg (in long val); */
NS_IMETHODIMP sbDbusConnection::SetInt32Arg(PRInt32 val)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setUInt16Arg (in unsigned short val); */
NS_IMETHODIMP sbDbusConnection::SetUInt16Arg(PRUint16 val)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setStringArg (in string val); */
NS_IMETHODIMP sbDbusConnection::SetStringArg(const char *val)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setDictSSEntryArg (in string key, in AString val); */
NS_IMETHODIMP sbDbusConnection::SetDictSSEntryArg(const char *key, const nsAString & val)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setDictSIEntryArg (in string key, in unsigned long val); */
NS_IMETHODIMP sbDbusConnection::SetDictSIEntryArg(const char *key, PRUint32 val)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void openDictEntryArray (); */
NS_IMETHODIMP sbDbusConnection::OpenDictEntryArray()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void closeDictEntryArray (); */
NS_IMETHODIMP sbDbusConnection::CloseDictEntryArray()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void openStruct (); */
NS_IMETHODIMP sbDbusConnection::OpenStruct()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void closeStruct (); */
NS_IMETHODIMP sbDbusConnection::CloseStruct()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_sbIMpris_h__ */
