/*
 * DO NOT EDIT.  THIS FILE IS GENERATED FROM mashTape.idl
 */

#ifndef __gen_mashTape_h__
#define __gen_mashTape_h__


#ifndef __gen_nsISupports_h__
#include "nsISupports.h"
#endif

/* For IDL files that don't want to include root IDL files. */
#ifndef NS_NO_VTABLE
#define NS_NO_VTABLE
#endif
class sbIMashTapeUtils; /* forward declaration */

class sbIMashTapeCallback; /* forward declaration */

class nsISimpleEnumerator; /* forward declaration */


/* starting interface:    sbIMashTapeProvider */
#define SBIMASHTAPEPROVIDER_IID_STR "a1b678c0-64a3-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPEPROVIDER_IID \
  {0xa1b678c0, 0x64a3, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeProvider : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEPROVIDER_IID)

  /* readonly attribute string providerName; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderName(char * *aProviderName) = 0;

  /* readonly attribute string providerType; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderType(char * *aProviderType) = 0;

  /* void query (in AUTF8String searchTerms, in sbIMashTapeCallback updateFn); */
  NS_SCRIPTABLE NS_IMETHOD Query(const nsACString & searchTerms, sbIMashTapeCallback *updateFn) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeProvider, SBIMASHTAPEPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetProviderName(char * *aProviderName); \
  NS_SCRIPTABLE NS_IMETHOD GetProviderType(char * *aProviderType); \
  NS_SCRIPTABLE NS_IMETHOD Query(const nsACString & searchTerms, sbIMashTapeCallback *updateFn); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderName(char * *aProviderName) { return _to GetProviderName(aProviderName); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderType(char * *aProviderType) { return _to GetProviderType(aProviderType); } \
  NS_SCRIPTABLE NS_IMETHOD Query(const nsACString & searchTerms, sbIMashTapeCallback *updateFn) { return _to Query(searchTerms, updateFn); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderName(char * *aProviderName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderName(aProviderName); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderType(char * *aProviderType) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderType(aProviderType); } \
  NS_SCRIPTABLE NS_IMETHOD Query(const nsACString & searchTerms, sbIMashTapeCallback *updateFn) { return !_to ? NS_ERROR_NULL_POINTER : _to->Query(searchTerms, updateFn); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeProvider : public sbIMashTapeProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEPROVIDER

  sbMashTapeProvider();

private:
  ~sbMashTapeProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeProvider, sbIMashTapeProvider)

sbMashTapeProvider::sbMashTapeProvider()
{
  /* member initializers and constructor code */
}

sbMashTapeProvider::~sbMashTapeProvider()
{
  /* destructor code */
}

/* readonly attribute string providerName; */
NS_IMETHODIMP sbMashTapeProvider::GetProviderName(char * *aProviderName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string providerType; */
NS_IMETHODIMP sbMashTapeProvider::GetProviderType(char * *aProviderType)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void query (in AUTF8String searchTerms, in sbIMashTapeCallback updateFn); */
NS_IMETHODIMP sbMashTapeProvider::Query(const nsACString & searchTerms, sbIMashTapeCallback *updateFn)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeInfoProvider */
#define SBIMASHTAPEINFOPROVIDER_IID_STR "65f60210-73bc-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPEINFOPROVIDER_IID \
  {0x65f60210, 0x73bc, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeInfoProvider : public sbIMashTapeProvider {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEINFOPROVIDER_IID)

  /* readonly attribute long numSections; */
  NS_SCRIPTABLE NS_IMETHOD GetNumSections(PRInt32 *aNumSections) = 0;

  /* readonly attribute string providerIconBio; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconBio(char * *aProviderIconBio) = 0;

  /* readonly attribute string providerIconTags; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconTags(char * *aProviderIconTags) = 0;

  /* readonly attribute string providerIconDiscography; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconDiscography(char * *aProviderIconDiscography) = 0;

  /* readonly attribute string providerIconMembers; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconMembers(char * *aProviderIconMembers) = 0;

  /* readonly attribute string providerIconLinks; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconLinks(char * *aProviderIconLinks) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeInfoProvider, SBIMASHTAPEINFOPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEINFOPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetNumSections(PRInt32 *aNumSections); \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconBio(char * *aProviderIconBio); \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconTags(char * *aProviderIconTags); \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconDiscography(char * *aProviderIconDiscography); \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconMembers(char * *aProviderIconMembers); \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconLinks(char * *aProviderIconLinks); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEINFOPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetNumSections(PRInt32 *aNumSections) { return _to GetNumSections(aNumSections); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconBio(char * *aProviderIconBio) { return _to GetProviderIconBio(aProviderIconBio); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconTags(char * *aProviderIconTags) { return _to GetProviderIconTags(aProviderIconTags); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconDiscography(char * *aProviderIconDiscography) { return _to GetProviderIconDiscography(aProviderIconDiscography); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconMembers(char * *aProviderIconMembers) { return _to GetProviderIconMembers(aProviderIconMembers); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconLinks(char * *aProviderIconLinks) { return _to GetProviderIconLinks(aProviderIconLinks); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEINFOPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetNumSections(PRInt32 *aNumSections) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetNumSections(aNumSections); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconBio(char * *aProviderIconBio) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIconBio(aProviderIconBio); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconTags(char * *aProviderIconTags) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIconTags(aProviderIconTags); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconDiscography(char * *aProviderIconDiscography) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIconDiscography(aProviderIconDiscography); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconMembers(char * *aProviderIconMembers) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIconMembers(aProviderIconMembers); } \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIconLinks(char * *aProviderIconLinks) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIconLinks(aProviderIconLinks); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeInfoProvider : public sbIMashTapeInfoProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEINFOPROVIDER

  sbMashTapeInfoProvider();

private:
  ~sbMashTapeInfoProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeInfoProvider, sbIMashTapeInfoProvider)

sbMashTapeInfoProvider::sbMashTapeInfoProvider()
{
  /* member initializers and constructor code */
}

sbMashTapeInfoProvider::~sbMashTapeInfoProvider()
{
  /* destructor code */
}

/* readonly attribute long numSections; */
NS_IMETHODIMP sbMashTapeInfoProvider::GetNumSections(PRInt32 *aNumSections)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string providerIconBio; */
NS_IMETHODIMP sbMashTapeInfoProvider::GetProviderIconBio(char * *aProviderIconBio)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string providerIconTags; */
NS_IMETHODIMP sbMashTapeInfoProvider::GetProviderIconTags(char * *aProviderIconTags)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string providerIconDiscography; */
NS_IMETHODIMP sbMashTapeInfoProvider::GetProviderIconDiscography(char * *aProviderIconDiscography)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string providerIconMembers; */
NS_IMETHODIMP sbMashTapeInfoProvider::GetProviderIconMembers(char * *aProviderIconMembers)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string providerIconLinks; */
NS_IMETHODIMP sbMashTapeInfoProvider::GetProviderIconLinks(char * *aProviderIconLinks)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapePhotoProvider */
#define SBIMASHTAPEPHOTOPROVIDER_IID_STR "8da914c0-64a6-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPEPHOTOPROVIDER_IID \
  {0x8da914c0, 0x64a6, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapePhotoProvider : public sbIMashTapeProvider {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEPHOTOPROVIDER_IID)

  /* readonly attribute string providerIcon; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapePhotoProvider, SBIMASHTAPEPHOTOPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEPHOTOPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEPHOTOPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return _to GetProviderIcon(aProviderIcon); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEPHOTOPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIcon(aProviderIcon); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapePhotoProvider : public sbIMashTapePhotoProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEPHOTOPROVIDER

  sbMashTapePhotoProvider();

private:
  ~sbMashTapePhotoProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapePhotoProvider, sbIMashTapePhotoProvider)

sbMashTapePhotoProvider::sbMashTapePhotoProvider()
{
  /* member initializers and constructor code */
}

sbMashTapePhotoProvider::~sbMashTapePhotoProvider()
{
  /* destructor code */
}

/* readonly attribute string providerIcon; */
NS_IMETHODIMP sbMashTapePhotoProvider::GetProviderIcon(char * *aProviderIcon)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeFlashProvider */
#define SBIMASHTAPEFLASHPROVIDER_IID_STR "a414db50-64c8-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPEFLASHPROVIDER_IID \
  {0xa414db50, 0x64c8, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeFlashProvider : public sbIMashTapeProvider {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEFLASHPROVIDER_IID)

  /* readonly attribute string providerIcon; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeFlashProvider, SBIMASHTAPEFLASHPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEFLASHPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEFLASHPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return _to GetProviderIcon(aProviderIcon); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEFLASHPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIcon(aProviderIcon); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeFlashProvider : public sbIMashTapeFlashProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEFLASHPROVIDER

  sbMashTapeFlashProvider();

private:
  ~sbMashTapeFlashProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeFlashProvider, sbIMashTapeFlashProvider)

sbMashTapeFlashProvider::sbMashTapeFlashProvider()
{
  /* member initializers and constructor code */
}

sbMashTapeFlashProvider::~sbMashTapeFlashProvider()
{
  /* destructor code */
}

/* readonly attribute string providerIcon; */
NS_IMETHODIMP sbMashTapeFlashProvider::GetProviderIcon(char * *aProviderIcon)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeRSSProvider */
#define SBIMASHTAPERSSPROVIDER_IID_STR "c8925af0-b4e8-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPERSSPROVIDER_IID \
  {0xc8925af0, 0xb4e8, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeRSSProvider : public sbIMashTapeProvider {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPERSSPROVIDER_IID)

  /* readonly attribute string providerIcon; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeRSSProvider, SBIMASHTAPERSSPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPERSSPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPERSSPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return _to GetProviderIcon(aProviderIcon); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPERSSPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIcon(aProviderIcon); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeRSSProvider : public sbIMashTapeRSSProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPERSSPROVIDER

  sbMashTapeRSSProvider();

private:
  ~sbMashTapeRSSProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeRSSProvider, sbIMashTapeRSSProvider)

sbMashTapeRSSProvider::sbMashTapeRSSProvider()
{
  /* member initializers and constructor code */
}

sbMashTapeRSSProvider::~sbMashTapeRSSProvider()
{
  /* destructor code */
}

/* readonly attribute string providerIcon; */
NS_IMETHODIMP sbMashTapeRSSProvider::GetProviderIcon(char * *aProviderIcon)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeReviewProvider */
#define SBIMASHTAPEREVIEWPROVIDER_IID_STR "097ef180-b12a-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPEREVIEWPROVIDER_IID \
  {0x097ef180, 0xb12a, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeReviewProvider : public sbIMashTapeProvider {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEREVIEWPROVIDER_IID)

  /* readonly attribute string providerIcon; */
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) = 0;

  /* void queryFull (in AUTF8String artist, in AUTF8String album, in AUTF8String track, in sbIMashTapeCallback updateFn); */
  NS_SCRIPTABLE NS_IMETHOD QueryFull(const nsACString & artist, const nsACString & album, const nsACString & track, sbIMashTapeCallback *updateFn) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeReviewProvider, SBIMASHTAPEREVIEWPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEREVIEWPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon); \
  NS_SCRIPTABLE NS_IMETHOD QueryFull(const nsACString & artist, const nsACString & album, const nsACString & track, sbIMashTapeCallback *updateFn); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEREVIEWPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return _to GetProviderIcon(aProviderIcon); } \
  NS_SCRIPTABLE NS_IMETHOD QueryFull(const nsACString & artist, const nsACString & album, const nsACString & track, sbIMashTapeCallback *updateFn) { return _to QueryFull(artist, album, track, updateFn); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEREVIEWPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetProviderIcon(char * *aProviderIcon) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetProviderIcon(aProviderIcon); } \
  NS_SCRIPTABLE NS_IMETHOD QueryFull(const nsACString & artist, const nsACString & album, const nsACString & track, sbIMashTapeCallback *updateFn) { return !_to ? NS_ERROR_NULL_POINTER : _to->QueryFull(artist, album, track, updateFn); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeReviewProvider : public sbIMashTapeReviewProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEREVIEWPROVIDER

  sbMashTapeReviewProvider();

private:
  ~sbMashTapeReviewProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeReviewProvider, sbIMashTapeReviewProvider)

sbMashTapeReviewProvider::sbMashTapeReviewProvider()
{
  /* member initializers and constructor code */
}

sbMashTapeReviewProvider::~sbMashTapeReviewProvider()
{
  /* destructor code */
}

/* readonly attribute string providerIcon; */
NS_IMETHODIMP sbMashTapeReviewProvider::GetProviderIcon(char * *aProviderIcon)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void queryFull (in AUTF8String artist, in AUTF8String album, in AUTF8String track, in sbIMashTapeCallback updateFn); */
NS_IMETHODIMP sbMashTapeReviewProvider::QueryFull(const nsACString & artist, const nsACString & album, const nsACString & track, sbIMashTapeCallback *updateFn)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeTabProvider */
#define SBIMASHTAPETABPROVIDER_IID_STR "8cb80ab0-b129-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPETABPROVIDER_IID \
  {0x8cb80ab0, 0xb129, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeTabProvider : public sbIMashTapeProvider {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPETABPROVIDER_IID)

  /* readonly attribute string tabName; */
  NS_SCRIPTABLE NS_IMETHOD GetTabName(char * *aTabName) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeTabProvider, SBIMASHTAPETABPROVIDER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPETABPROVIDER \
  NS_SCRIPTABLE NS_IMETHOD GetTabName(char * *aTabName); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPETABPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetTabName(char * *aTabName) { return _to GetTabName(aTabName); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPETABPROVIDER(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetTabName(char * *aTabName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetTabName(aTabName); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeTabProvider : public sbIMashTapeTabProvider
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPETABPROVIDER

  sbMashTapeTabProvider();

private:
  ~sbMashTapeTabProvider();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeTabProvider, sbIMashTapeTabProvider)

sbMashTapeTabProvider::sbMashTapeTabProvider()
{
  /* member initializers and constructor code */
}

sbMashTapeTabProvider::~sbMashTapeTabProvider()
{
  /* destructor code */
}

/* readonly attribute string tabName; */
NS_IMETHODIMP sbMashTapeTabProvider::GetTabName(char * *aTabName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeCallback */
#define SBIMASHTAPECALLBACK_IID_STR "959b2c40-68ca-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPECALLBACK_IID \
  {0x959b2c40, 0x68ca, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeCallback : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPECALLBACK_IID)

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeCallback, SBIMASHTAPECALLBACK_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPECALLBACK \
  /* no methods! */

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPECALLBACK(_to) \
  /* no methods! */

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPECALLBACK(_to) \
  /* no methods! */

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeCallback : public sbIMashTapeCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPECALLBACK

  sbMashTapeCallback();

private:
  ~sbMashTapeCallback();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeCallback, sbIMashTapeCallback)

sbMashTapeCallback::sbMashTapeCallback()
{
  /* member initializers and constructor code */
}

sbMashTapeCallback::~sbMashTapeCallback()
{
  /* destructor code */
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapePhoto */
#define SBIMASHTAPEPHOTO_IID_STR "10adb8e6-4a7d-4789-acda-b0975e6f137d"

#define SBIMASHTAPEPHOTO_IID \
  {0x10adb8e6, 0x4a7d, 0x4789, \
    { 0xac, 0xda, 0xb0, 0x97, 0x5e, 0x6f, 0x13, 0x7d }}

/*******************************/
class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapePhoto : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEPHOTO_IID)

  /* readonly attribute string imageUrl; */
  NS_SCRIPTABLE NS_IMETHOD GetImageUrl(char * *aImageUrl) = 0;

  /* readonly attribute string imageTitle; */
  NS_SCRIPTABLE NS_IMETHOD GetImageTitle(char * *aImageTitle) = 0;

  /* readonly attribute string authorName; */
  NS_SCRIPTABLE NS_IMETHOD GetAuthorName(char * *aAuthorName) = 0;

  /* readonly attribute string authorUrl; */
  NS_SCRIPTABLE NS_IMETHOD GetAuthorUrl(char * *aAuthorUrl) = 0;

  /* readonly attribute unsigned long timestamp; */
  NS_SCRIPTABLE NS_IMETHOD GetTimestamp(PRUint32 *aTimestamp) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapePhoto, SBIMASHTAPEPHOTO_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEPHOTO \
  NS_SCRIPTABLE NS_IMETHOD GetImageUrl(char * *aImageUrl); \
  NS_SCRIPTABLE NS_IMETHOD GetImageTitle(char * *aImageTitle); \
  NS_SCRIPTABLE NS_IMETHOD GetAuthorName(char * *aAuthorName); \
  NS_SCRIPTABLE NS_IMETHOD GetAuthorUrl(char * *aAuthorUrl); \
  NS_SCRIPTABLE NS_IMETHOD GetTimestamp(PRUint32 *aTimestamp); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEPHOTO(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetImageUrl(char * *aImageUrl) { return _to GetImageUrl(aImageUrl); } \
  NS_SCRIPTABLE NS_IMETHOD GetImageTitle(char * *aImageTitle) { return _to GetImageTitle(aImageTitle); } \
  NS_SCRIPTABLE NS_IMETHOD GetAuthorName(char * *aAuthorName) { return _to GetAuthorName(aAuthorName); } \
  NS_SCRIPTABLE NS_IMETHOD GetAuthorUrl(char * *aAuthorUrl) { return _to GetAuthorUrl(aAuthorUrl); } \
  NS_SCRIPTABLE NS_IMETHOD GetTimestamp(PRUint32 *aTimestamp) { return _to GetTimestamp(aTimestamp); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEPHOTO(_to) \
  NS_SCRIPTABLE NS_IMETHOD GetImageUrl(char * *aImageUrl) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetImageUrl(aImageUrl); } \
  NS_SCRIPTABLE NS_IMETHOD GetImageTitle(char * *aImageTitle) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetImageTitle(aImageTitle); } \
  NS_SCRIPTABLE NS_IMETHOD GetAuthorName(char * *aAuthorName) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAuthorName(aAuthorName); } \
  NS_SCRIPTABLE NS_IMETHOD GetAuthorUrl(char * *aAuthorUrl) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetAuthorUrl(aAuthorUrl); } \
  NS_SCRIPTABLE NS_IMETHOD GetTimestamp(PRUint32 *aTimestamp) { return !_to ? NS_ERROR_NULL_POINTER : _to->GetTimestamp(aTimestamp); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapePhoto : public sbIMashTapePhoto
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEPHOTO

  sbMashTapePhoto();

private:
  ~sbMashTapePhoto();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapePhoto, sbIMashTapePhoto)

sbMashTapePhoto::sbMashTapePhoto()
{
  /* member initializers and constructor code */
}

sbMashTapePhoto::~sbMashTapePhoto()
{
  /* destructor code */
}

/* readonly attribute string imageUrl; */
NS_IMETHODIMP sbMashTapePhoto::GetImageUrl(char * *aImageUrl)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string imageTitle; */
NS_IMETHODIMP sbMashTapePhoto::GetImageTitle(char * *aImageTitle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string authorName; */
NS_IMETHODIMP sbMashTapePhoto::GetAuthorName(char * *aAuthorName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute string authorUrl; */
NS_IMETHODIMP sbMashTapePhoto::GetAuthorUrl(char * *aAuthorUrl)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute unsigned long timestamp; */
NS_IMETHODIMP sbMashTapePhoto::GetTimestamp(PRUint32 *aTimestamp)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeListener */
#define SBIMASHTAPELISTENER_IID_STR "7373b260-c584-11dd-ad8b-0800200c9a66"

#define SBIMASHTAPELISTENER_IID \
  {0x7373b260, 0xc584, 0x11dd, \
    { 0xad, 0x8b, 0x08, 0x00, 0x20, 0x0c, 0x9a, 0x66 }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeListener : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPELISTENER_IID)

  /* void onInfoUpdated (in AUTF8String section, in AUTF8String data); */
  NS_SCRIPTABLE NS_IMETHOD OnInfoUpdated(const nsACString & section, const nsACString & data) = 0;

  /* void onPhotosUpdated ([array, size_is (photoCount)] in sbIMashTapePhoto photos, in unsigned long photoCount); */
  NS_SCRIPTABLE NS_IMETHOD OnPhotosUpdated(sbIMashTapePhoto **photos, PRUint32 photoCount) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeListener, SBIMASHTAPELISTENER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPELISTENER \
  NS_SCRIPTABLE NS_IMETHOD OnInfoUpdated(const nsACString & section, const nsACString & data); \
  NS_SCRIPTABLE NS_IMETHOD OnPhotosUpdated(sbIMashTapePhoto **photos, PRUint32 photoCount); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPELISTENER(_to) \
  NS_SCRIPTABLE NS_IMETHOD OnInfoUpdated(const nsACString & section, const nsACString & data) { return _to OnInfoUpdated(section, data); } \
  NS_SCRIPTABLE NS_IMETHOD OnPhotosUpdated(sbIMashTapePhoto **photos, PRUint32 photoCount) { return _to OnPhotosUpdated(photos, photoCount); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPELISTENER(_to) \
  NS_SCRIPTABLE NS_IMETHOD OnInfoUpdated(const nsACString & section, const nsACString & data) { return !_to ? NS_ERROR_NULL_POINTER : _to->OnInfoUpdated(section, data); } \
  NS_SCRIPTABLE NS_IMETHOD OnPhotosUpdated(sbIMashTapePhoto **photos, PRUint32 photoCount) { return !_to ? NS_ERROR_NULL_POINTER : _to->OnPhotosUpdated(photos, photoCount); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeListener : public sbIMashTapeListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPELISTENER

  sbMashTapeListener();

private:
  ~sbMashTapeListener();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeListener, sbIMashTapeListener)

sbMashTapeListener::sbMashTapeListener()
{
  /* member initializers and constructor code */
}

sbMashTapeListener::~sbMashTapeListener()
{
  /* destructor code */
}

/* void onInfoUpdated (in AUTF8String section, in AUTF8String data); */
NS_IMETHODIMP sbMashTapeListener::OnInfoUpdated(const nsACString & section, const nsACString & data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onPhotosUpdated ([array, size_is (photoCount)] in sbIMashTapePhoto photos, in unsigned long photoCount); */
NS_IMETHODIMP sbMashTapeListener::OnPhotosUpdated(sbIMashTapePhoto **photos, PRUint32 photoCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


/* starting interface:    sbIMashTapeManager */
#define SBIMASHTAPEMANAGER_IID_STR "7e991583-030f-4945-8273-a34a5b2752cf"

#define SBIMASHTAPEMANAGER_IID \
  {0x7e991583, 0x030f, 0x4945, \
    { 0x82, 0x73, 0xa3, 0x4a, 0x5b, 0x27, 0x52, 0xcf }}

class NS_NO_VTABLE NS_SCRIPTABLE sbIMashTapeManager : public nsISupports {
 public: 

  NS_DECLARE_STATIC_IID_ACCESSOR(SBIMASHTAPEMANAGER_IID)

  /* void addListener (in sbIMashTapeListener aListener); */
  NS_SCRIPTABLE NS_IMETHOD AddListener(sbIMashTapeListener *aListener) = 0;

  /* void removeListener (in sbIMashTapeListener aListener); */
  NS_SCRIPTABLE NS_IMETHOD RemoveListener(sbIMashTapeListener *aListener) = 0;

  /* void updateInfo (in AUTF8String section, in AUTF8String data); */
  NS_SCRIPTABLE NS_IMETHOD UpdateInfo(const nsACString & section, const nsACString & data) = 0;

  /* void updatePhotos ([array, size_is (photoCount)] in sbIMashTapePhoto photos, in unsigned long photoCount); */
  NS_SCRIPTABLE NS_IMETHOD UpdatePhotos(sbIMashTapePhoto **photos, PRUint32 photoCount) = 0;

};

  NS_DEFINE_STATIC_IID_ACCESSOR(sbIMashTapeManager, SBIMASHTAPEMANAGER_IID)

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBIMASHTAPEMANAGER \
  NS_SCRIPTABLE NS_IMETHOD AddListener(sbIMashTapeListener *aListener); \
  NS_SCRIPTABLE NS_IMETHOD RemoveListener(sbIMashTapeListener *aListener); \
  NS_SCRIPTABLE NS_IMETHOD UpdateInfo(const nsACString & section, const nsACString & data); \
  NS_SCRIPTABLE NS_IMETHOD UpdatePhotos(sbIMashTapePhoto **photos, PRUint32 photoCount); 

/* Use this macro to declare functions that forward the behavior of this interface to another object. */
#define NS_FORWARD_SBIMASHTAPEMANAGER(_to) \
  NS_SCRIPTABLE NS_IMETHOD AddListener(sbIMashTapeListener *aListener) { return _to AddListener(aListener); } \
  NS_SCRIPTABLE NS_IMETHOD RemoveListener(sbIMashTapeListener *aListener) { return _to RemoveListener(aListener); } \
  NS_SCRIPTABLE NS_IMETHOD UpdateInfo(const nsACString & section, const nsACString & data) { return _to UpdateInfo(section, data); } \
  NS_SCRIPTABLE NS_IMETHOD UpdatePhotos(sbIMashTapePhoto **photos, PRUint32 photoCount) { return _to UpdatePhotos(photos, photoCount); } 

/* Use this macro to declare functions that forward the behavior of this interface to another object in a safe way. */
#define NS_FORWARD_SAFE_SBIMASHTAPEMANAGER(_to) \
  NS_SCRIPTABLE NS_IMETHOD AddListener(sbIMashTapeListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->AddListener(aListener); } \
  NS_SCRIPTABLE NS_IMETHOD RemoveListener(sbIMashTapeListener *aListener) { return !_to ? NS_ERROR_NULL_POINTER : _to->RemoveListener(aListener); } \
  NS_SCRIPTABLE NS_IMETHOD UpdateInfo(const nsACString & section, const nsACString & data) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpdateInfo(section, data); } \
  NS_SCRIPTABLE NS_IMETHOD UpdatePhotos(sbIMashTapePhoto **photos, PRUint32 photoCount) { return !_to ? NS_ERROR_NULL_POINTER : _to->UpdatePhotos(photos, photoCount); } 

#if 0
/* Use the code below as a template for the implementation class for this interface. */

/* Header file */
class sbMashTapeManager : public sbIMashTapeManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMASHTAPEMANAGER

  sbMashTapeManager();

private:
  ~sbMashTapeManager();

protected:
  /* additional members */
};

/* Implementation file */
NS_IMPL_ISUPPORTS1(sbMashTapeManager, sbIMashTapeManager)

sbMashTapeManager::sbMashTapeManager()
{
  /* member initializers and constructor code */
}

sbMashTapeManager::~sbMashTapeManager()
{
  /* destructor code */
}

/* void addListener (in sbIMashTapeListener aListener); */
NS_IMETHODIMP sbMashTapeManager::AddListener(sbIMashTapeListener *aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void removeListener (in sbIMashTapeListener aListener); */
NS_IMETHODIMP sbMashTapeManager::RemoveListener(sbIMashTapeListener *aListener)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void updateInfo (in AUTF8String section, in AUTF8String data); */
NS_IMETHODIMP sbMashTapeManager::UpdateInfo(const nsACString & section, const nsACString & data)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void updatePhotos ([array, size_is (photoCount)] in sbIMashTapePhoto photos, in unsigned long photoCount); */
NS_IMETHODIMP sbMashTapeManager::UpdatePhotos(sbIMashTapePhoto **photos, PRUint32 photoCount)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* End of implementation class template. */
#endif


#endif /* __gen_mashTape_h__ */
