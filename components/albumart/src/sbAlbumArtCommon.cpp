
#include "sbAlbumArtCommon.h"

#include <nsCOMPtr.h>
#include <sbStandardProperties.h>
#include <nsISimpleEnumerator.h>
#include <sbTArrayStringEnumerator.h>
#include <nsStringGlue.h>
#include <sbIFileMetadataService.h>
#include <nsServiceManagerUtils.h>
#include <sbIJobProgress.h>
#include <nsIMutableArray.h>
#include <nsComponentManagerUtils.h>
#include <nsArrayUtils.h>
#include <sbILibrary.h>

nsresult SetItemArtwork(nsIURI* aImageLocation, sbIMediaItem* aMediaItem) {
  NS_ENSURE_ARG_POINTER(aImageLocation);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  nsCAutoString imageFileURISpec;
  rv = aImageLocation->GetSpec(imageFileURISpec);
  if (NS_SUCCEEDED(rv)) {
    rv = aMediaItem->SetProperty(
            NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
            NS_ConvertUTF8toUTF16(imageFileURISpec));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

nsresult SetItemsArtwork(nsIURI* aImageLocation, nsIArray* aMediaItems) {
  NS_ENSURE_ARG_POINTER(aMediaItems);
  NS_ENSURE_ARG_POINTER(aMediaItems);

  nsresult rv;
  nsCOMPtr<nsISimpleEnumerator> listEnum;
  rv = aMediaItems->Enumerate(getter_AddRefs(listEnum));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool hasMore;
  while (NS_SUCCEEDED(listEnum->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> next;
    if (NS_SUCCEEDED(listEnum->GetNext(getter_AddRefs(next))) && next) {
      nsCOMPtr<sbIMediaItem> mediaItem(do_QueryInterface(next));
      rv = SetItemArtwork(aImageLocation, mediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  return NS_OK;
}

nsresult WriteImageMetadata(nsIArray* aMediaItems) {
  NS_ENSURE_ARG_POINTER(aMediaItems);

  nsresult rv;
  PRUint32 numItems;
  rv = aMediaItems->GetLength(&numItems);
  NS_ENSURE_SUCCESS(rv, rv);
  if (numItems > 0) {
    // Do nothing if the media items' library is marked to not write metadata.
    nsCOMPtr<sbIMediaItem> mediaItem = do_QueryElementAt(aMediaItems, 0, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<sbILibrary> library;
    rv = mediaItem->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);
    nsAutoString dontWriteMetadata;
    rv = library->GetProperty
                    (NS_LITERAL_STRING(SB_PROPERTY_DONT_WRITE_METADATA),
                     dontWriteMetadata);
    NS_ENSURE_SUCCESS(rv, rv);
    if (dontWriteMetadata.Equals(NS_LITERAL_STRING("1")))
      return NS_OK;

    nsTArray<nsString> propArray;
    if (!propArray.AppendElement(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL)))
    {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    nsCOMPtr<nsIStringEnumerator> propsToWrite =
      new sbTArrayStringEnumerator(&propArray);
    NS_ENSURE_TRUE(propsToWrite, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbIFileMetadataService> metadataService =
      do_GetService( "@getnightingale.com/Nightingale/FileMetadataService;1", &rv );
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbIJobProgress> job;
    rv = metadataService->Write(aMediaItems, propsToWrite, getter_AddRefs(job));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

