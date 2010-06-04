/* vim: fileencoding=utf-8 shiftwidth=2 */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org widget code.
 *
 * The Initial Developer of the Original Code is
 * Matthew Gertner <matthew@allpeers.com>.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * AllPeers Ltd. All Rights Reserved.
 *
 * Contributor(s):
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "nsStringGlue.h"
#include "nsImageToBitmap.h"
#include "gfxImageSurface.h"

#include <windows.h>

NS_IMPL_ISUPPORTS1(nsImageToBitmap, nsIImageToBitmap)

nsImageToBitmap::nsImageToBitmap()
{
}

nsImageToBitmap::~nsImageToBitmap()
{
}

NS_IMETHODIMP nsImageToBitmap::ConvertImageToBitmap(gfxImageSurface* aImage,
  HBITMAP& outBitmap)
{
  NS_ENSURE_ARG(aImage);
  outBitmap = NULL;
  
  BITMAPV4HEADER infoHeader;
  ::ZeroMemory(&infoHeader, sizeof(infoHeader));
  infoHeader.bV4Size = sizeof(infoHeader);
  infoHeader.bV4Width = aImage->Width();
  infoHeader.bV4Height = -aImage->Height(); // top-to-bottom
  infoHeader.bV4Planes = 1;
  infoHeader.bV4BitCount = 32; // RGBA
  infoHeader.bV4V4Compression = BI_BITFIELDS;
  infoHeader.bV4SizeImage = aImage->Height() * aImage->Stride();
  infoHeader.bV4XPelsPerMeter = 0;
  infoHeader.bV4YPelsPerMeter = 0;
  infoHeader.bV4ClrUsed = 0;
  infoHeader.bV4ClrImportant = 0;
  infoHeader.bV4RedMask   = 0x00FF0000;
  infoHeader.bV4GreenMask = 0x0000FF00;
  infoHeader.bV4BlueMask  = 0x000000FF;
  infoHeader.bV4AlphaMask = 0xFF000000;
  
  HBITMAP tBitmap = NULL, oldbits = NULL, bitmap = NULL;
  HDC dc = ::CreateCompatibleDC(NULL);
  if (!dc) return NS_ERROR_FAILURE;
  // we have to select a dummy bitmap in first;
  // otherwise we just end up with a boring black icon
  tBitmap = ::CreateBitmap(1, 1, 1, 32, NULL);
  if (!tBitmap) goto loser;
  oldbits = (HBITMAP)::SelectObject(dc, tBitmap);
  if (!oldbits) goto loser;
  
  PRUint8 *bits = aImage->Data();
  
  bitmap = ::CreateDIBitmap(dc,
                            reinterpret_cast<CONST BITMAPINFOHEADER*>(&infoHeader),
                            CBM_INIT,
                            bits,
                            reinterpret_cast<CONST BITMAPINFO*>(&infoHeader),
                            DIB_RGB_COLORS);
  
  if (bitmap) {
    // we were successful
    outBitmap = bitmap;
  }
  
loser:
  // cleanup
  if (oldbits)
    ::SelectObject(dc, oldbits);
  if (tBitmap)
    ::DeleteObject(tBitmap);
  ::DeleteObject(dc);
  
  return outBitmap ? NS_OK : NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsImageToBitmap::ConvertImageToIcon(gfxImageSurface* aImage,
  HICON& outIcon)
{
  return ImageToIcon(aImage, PR_TRUE, 0, 0, outIcon);
}

NS_IMETHODIMP nsImageToBitmap::ConvertImageToCursor(gfxImageSurface* aImage,
  PRUint32 aHotspotX, PRUint32 aHotspotY, HCURSOR& outCursor)
{
  return ImageToIcon(aImage, PR_FALSE, aHotspotX, aHotspotY, outCursor);
}

nsresult nsImageToBitmap::ImageToIcon(gfxImageSurface* aImage,
  PRBool aIcon, PRUint32 aHotspotX, PRUint32 aHotspotY,
  HICON& _retval)
{
  nsresult rv;

  HBITMAP hBMP;
  rv = ConvertImageToBitmap(aImage, hBMP);
  if (NS_FAILED(rv))
    return rv;

  ICONINFO info = {0};
  info.fIcon = aIcon;
  info.xHotspot = aHotspotX;
  info.yHotspot = aHotspotY;
  info.hbmMask = hBMP;
  info.hbmColor = hBMP;
  
  _retval = ::CreateIconIndirect(&info);
  ::DeleteObject(hBMP);

  if (_retval == NULL) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}
