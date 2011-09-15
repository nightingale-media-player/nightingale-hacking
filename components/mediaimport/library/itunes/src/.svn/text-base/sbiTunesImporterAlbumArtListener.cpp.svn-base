/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbiTunesImporterAlbumArtListener.h"

#include <nsArrayUtils.h>
#include <nsCOMPtr.h>
#include <nsStringAPI.h>
#include <nsIArray.h>
#include <nsIURI.h>

#include <sbIAlbumArtFetcher.h>
#include <sbIMediaItem.h>
#include <sbStandardProperties.h>

NS_IMPL_THREADSAFE_ISUPPORTS1(sbiTunesImporterAlbumArtListener, 
                              sbIAlbumArtListener)

sbiTunesImporterAlbumArtListener::sbiTunesImporterAlbumArtListener() {
}

sbiTunesImporterAlbumArtListener::~sbiTunesImporterAlbumArtListener() {
}

sbiTunesImporterAlbumArtListener * sbiTunesImporterAlbumArtListener::New() {
  return new sbiTunesImporterAlbumArtListener;
}

/* void onChangeFetcher (in sbIAlbumArtFetcher aFetcher); */
NS_IMETHODIMP 
sbiTunesImporterAlbumArtListener::OnChangeFetcher(sbIAlbumArtFetcher *aFetcher)
{
  // Nothing to do here, move along
  return NS_OK;
}

/* void onTrackResult (in nsIURI aImageLocation, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP 
sbiTunesImporterAlbumArtListener::OnTrackResult(nsIURI *aImageLocation, 
                                                sbIMediaItem *aMediaItem)
{
  if (aImageLocation) {
    nsCString spec;
    nsresult rv = aImageLocation->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL), 
                                 NS_ConvertUTF8toUTF16(spec));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

/* void onAlbumResult (in nsIURI aImageLocation, in nsIArray aMediaItems); */
NS_IMETHODIMP 
sbiTunesImporterAlbumArtListener::OnAlbumResult(nsIURI *aImageLocation, 
                                                nsIArray *aMediaItems)
{
  if (aImageLocation) {
    nsCString spec;
    nsresult rv = aImageLocation->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> mediaItem;
    PRUint32 length;
    rv = aMediaItems->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);
   
    for (PRUint32 index = 0; index < length; ++index) {
      mediaItem = do_QueryElementAt(aMediaItems, index, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                                  NS_ConvertASCIItoUTF16(spec));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

/* void onSearchComplete (in nsIArray aMediaItems); */
NS_IMETHODIMP
sbiTunesImporterAlbumArtListener::OnSearchComplete(nsIArray *aMediaItems)
{
  // Nothing to do here, move along
  return NS_OK;
}
