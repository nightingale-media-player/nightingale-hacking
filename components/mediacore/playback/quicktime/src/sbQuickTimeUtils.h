#ifndef __SB_QUICKTIMEUTILS_H__
#define __SB_QUICKTIMEUTILS_H__

#include <Movies.h>

#include <nsCOMPtr.h>
#include <nsIBaseWindow.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIDocument.h>
#include <nsIDOMAbstractView.h>
#include <nsIDOMDocument.h>
#include <nsIDOMDocumentView.h>
#include <nsIDOMEvent.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIDOMXULElement.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsILocalFile.h>
#include <nsILocalFileMac.h>
#include <nsIScriptGlobalObject.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIWebNavigation.h>
#include <nsIWidget.h>
#include <nsNetUtil.h>
#include <nsReadableUtils.h>
#include <nsRect.h>
#include <nsString.h>

#define NSRESULT_FROM_OSERR(err) ((err) == noErr) ? NS_OK : NS_ERROR_FAILURE

#define QT_ENSURE_NOERR(err)                                                   \
  NS_ENSURE_TRUE((err) == noErr, NS_ERROR_FAILURE);

#define QT_ENSURE_NOMOVIESERROR()                                              \
  QT_ENSURE_NOERR(::GetMoviesError());

/**
 * This is a helper class to auto-free any core foundation types
 */
template<class Type>
class sbCFRef
{
protected:
  Type mRef;

public:
  sbCFRef() : mRef(nsnull) { }
  sbCFRef(Type aRef) : mRef(aRef) { }
  
  ~sbCFRef() {
    if (mRef) ::CFRelease(mRef);
  }

  Type operator=(Type aRef) {
    if (mRef && (mRef != aRef))
      ::CFRelease(mRef);
    return (mRef = aRef);
  }
  
  operator Type() {
    return mRef;
  }
  
  operator int() {
    return (mRef != nsnull);
  }
  
  void reset() {
    mRef = nsnull;
  }
  
  Type* addressof() {
    return &mRef;
  }
};

/**
 * This is a helper class to auto-swap QuickDraw Graphics Ports
 */
class sbQDPortSwapper
{
protected:
  CGrafPtr mOldPort;
  PRBool mPortSwapped;
public:
  sbQDPortSwapper(CGrafPtr aNewPort) : mOldPort(nsnull),
                                       mPortSwapped(PR_FALSE) {
    if (aNewPort) {
      mPortSwapped = ::QDSwapPort(aNewPort, &mOldPort);
    }
  }
  
  ~sbQDPortSwapper() {
    if (mPortSwapped)
      ::QDSwapPort(mOldPort, nsnull);
  }
  
};

static nsresult
SB_CreateMovieFromURI(nsIURI* aURI,
                      Movie* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  OSErr err;
  nsresult rv;
  
  if (!aURI) {
    // null aURI means an empty movie
    err = ::NewMovieFromProperties(0, nsnull, 0, nsnull, _retval);
    QT_ENSURE_NOERR(err);
    return NS_OK;
  }
  
  nsCAutoString spec;
  aURI->GetSpec(spec);
  
  sbCFRef<CFStringRef> urlString;
  sbCFRef<CFURLRef> urlRef;
  
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(aURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsILocalFileMac> macFile = do_QueryInterface(file, &rv);
      if (NS_SUCCEEDED(rv)) {
        rv = macFile->GetCFURL(urlRef.addressof());
        if (NS_FAILED(rv))
          urlRef.reset();
      }
    }
  }
  
  if (!urlRef) {
    nsCAutoString spec;
    rv = aURI->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    
    urlString = 
      CFStringCreateWithCString(kCFAllocatorDefault,
                                spec.get(),
                                kCFStringEncodingASCII);
    urlRef =
      CFURLCreateWithString(kCFAllocatorDefault,
                            urlString,
                            nsnull);
  }
  
  NS_ENSURE_STATE(urlRef);
  
  QTNewMoviePropertyElement movieProperties[10] = {0};
  PRInt32 propCount = 0;

  movieProperties[propCount].propClass = kQTPropertyClass_DataLocation;
  movieProperties[propCount].propID = kQTDataLocationPropertyID_CFURL;
  movieProperties[propCount].propValueSize = sizeof(CFURLRef);
  movieProperties[propCount++].propValueAddress = urlRef.addressof();

  Boolean trueValue = PR_TRUE;
  movieProperties[propCount].propClass = kQTPropertyClass_NewMovieProperty;
  movieProperties[propCount].propID = kQTNewMoviePropertyID_Active;
  movieProperties[propCount].propValueSize = sizeof(trueValue);
  movieProperties[propCount++].propValueAddress = &trueValue;
  
  // Ensure that we have some sort of GWorld already. This is a QuickTime
  // requirement.
  CGrafPtr oldPort = nsnull;
  ::GetPort(&oldPort);
  if (!oldPort)
    ::SetPort(nsnull);

  err = ::NewMovieFromProperties(propCount, movieProperties, 0, nsnull,
                                 _retval);
  QT_ENSURE_NOERR(err);

  ::SetMovieVolume(*_retval, kFullVolume);
  
  return NS_OK;

}

inline nsresult
SB_CreateMovieFromFile(nsIFile* aFile,
                       Movie* _retval)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewFileURI(getter_AddRefs(uri), aFile);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "NS_NewFileURI failed");
  return SB_CreateMovieFromURI(uri, _retval);

}

inline nsresult
SB_CreateMovieFromURL(nsIURL* aURL,
                      Movie* _retval)
{
  nsresult rv;
  nsCOMPtr<nsIURI> uri = do_QueryInterface(aURL, &rv);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "QI failed");
  return SB_CreateMovieFromURI(uri, _retval);
}

inline nsresult
SB_CreateMovieFromString(nsAString& aURL,
                         Movie* _retval)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "NS_NewURI failed");
  return SB_CreateMovieFromURI(uri, _retval);
}

inline nsresult
SB_CreateMovieFromCString(nsACString& aURL,
                          Movie* _retval)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aURL);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "NS_NewURI failed");
  return SB_CreateMovieFromURI(uri, _retval);
}

inline nsresult
SB_CreateEmptyMovie(Movie* _retval)
{
  return SB_CreateMovieFromURI(nsnull, _retval);
}

static nsresult
GetDomEventTargetFromElement(nsIDOMXULElement* aElement,
                             nsIDOMEventTarget** _retval)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv;
  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = aElement->GetOwnerDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> doc = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMWindow> domWindow =
    do_QueryInterface(doc->GetScriptGlobalObject(), &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(domWindow, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  NS_ADDREF(*_retval = target);
  return NS_OK;
}

static nsresult
GetWidgetFromElement(nsIDOMXULElement* aElement,
                     nsIWidget** _retval)
{
  NS_ENSURE_ARG_POINTER(aElement);
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv;
  nsCOMPtr<nsIDOMDocument> domDoc;
  rv = aElement->GetOwnerDocument(getter_AddRefs(domDoc));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMDocumentView> domDocView = do_QueryInterface(domDoc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMAbstractView> domAbsView;
  rv = domDocView->GetDefaultView(getter_AddRefs(domAbsView));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIWebNavigation> webNav = do_GetInterface(domAbsView, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDocShellTreeItem> docTreeItem = do_QueryInterface(webNav, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDocShellTreeOwner> docTreeOwner;
  rv = docTreeItem->GetTreeOwner(getter_AddRefs(docTreeOwner));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(docTreeOwner, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIWidget> widget;
  rv = baseWindow->GetMainWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = widget);
  return NS_OK;
}

inline void
RectToMacRect(const nsRect& aRect,
              Rect& aMacRect)
{
  aMacRect.left = aRect.x;
  aMacRect.top = aRect.y;
  aMacRect.right = aRect.x + aRect.width;
  aMacRect.bottom = aRect.y + aRect.height;
}

inline void
MacRectToRect(const Rect& aMacRect,
              nsRect& aRect)
{
  aRect.x = aMacRect.left;
  aRect.y = aMacRect.top;
  aRect.width = aMacRect.right - aMacRect.left;
  aRect.height = aMacRect.bottom - aMacRect.top;
}

static nsresult
SB_GetFileExtensionFromURI(nsIURI* aURI,
                           nsACString& _retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  
  nsCAutoString strExtension;
  
  nsresult rv;
  nsCOMPtr<nsIURL> url = do_QueryInterface(aURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    // Try the easy way...
    rv = url->GetFileExtension(strExtension);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Try the hard way...
    nsCAutoString path;
    rv = aURI->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCAutoString::const_iterator start, end, realend;
    path.BeginReading(start);
    path.EndReading(end);
    realend = end;
    if (RFindInReadable(NS_LITERAL_CSTRING("."), start, end) &&
        end != realend)
      strExtension.Assign(Substring(end, realend));
  }
  
  if (strExtension.IsEmpty())
    return NS_ERROR_MALFORMED_URI;
  
  // Strip '.' from the beginning and end, if it exists
  strExtension.Trim(".");
  
  _retval.Assign(strExtension);
  return NS_OK;
}

#endif /* __SB_QUICKTIMEUTILS_H__ */