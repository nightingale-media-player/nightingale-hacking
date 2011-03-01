/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbDeviceMediaItemDecorator.h"
#include "sbStandardProperties.h"
#include "sbStringBundle.h"

#include "nsCOMPtr.h"
#include "nsIURI.h"


/* static */ nsresult
sbDeviceMediaItemDecorator::DecorateMediaItem(sbIDevice * aDevice,
                                              sbIMediaItem * aMediaItem,
                                              const nsAString & aImportType)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;

#if DEBUG
  // Get media item info for viewing in debugger:
  nsAutoString mi;
  aMediaItem->ToString(mi);

  nsCOMPtr<nsIURI> contentUri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(contentUri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString contentSpec;
  rv = contentUri->GetSpec(contentSpec);
  NS_ENSURE_SUCCESS(rv, rv);
#endif // #if DEBUG

  // Do nothing if no import type:
  if (aImportType.IsEmpty()) {
    return NS_OK;
  }

  // Localizable strings reside in the songbird.properties bundle and
  // use the device.sync. prefix:
  sbStringBundle bundle("chrome://songbird/locale/songbird.properties");

  // Dispatch on import type and adjust the media item properties as
  // specified at
  // http://wiki.songbirdnest.com/Releases/Ratatat/Device_Import_and_Sync#Sync_Logic
  nsAutoString importType;
  importType = aImportType;
  if (importType == NS_LITERAL_STRING("fm-recording")) {
    // An FM recording.  Set the genre to identify it as such:
    nsAutoString genre;
    genre = bundle.Get("device.sync.import_type.fm_recording");
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_GENRE), genre);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the artist name to the device name:
    nsAutoString deviceName;
    rv = aDevice->GetName(deviceName);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                                 deviceName);
    NS_ENSURE_SUCCESS(rv, rv);

    /// @todo Include the file creation timestamp in the track name
    ///       if possible.
  }
  else if (importType == NS_LITERAL_STRING("voice-recording")) {
    // A voice recording.  Set the genre to identify it as such:
    nsAutoString genre;
    genre = bundle.Get("device.sync.import_type.voice_recording");
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_GENRE), genre);
    NS_ENSURE_SUCCESS(rv, rv);

    // Set the artist name to the device name:
    nsAutoString deviceName;
    rv = aDevice->GetName(deviceName);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                                 deviceName);
    NS_ENSURE_SUCCESS(rv, rv);

    /// @todo Include the file creation timestamp in the track name
    ///       if possible.
  }

  return NS_OK;
}
