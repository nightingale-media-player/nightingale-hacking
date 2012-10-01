/* vim: fileencoding=utf-8 shiftwidth=4 */
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

#ifndef NSIMAGETOBITMAP_H_
#define NSIMAGETOBITMAP_H_

#include "nsIImageToBitmap.h"

class nsImageToBitmap : public nsIImageToBitmap {
    public:
        NS_DECL_ISUPPORTS

        nsImageToBitmap();
        ~nsImageToBitmap();

        NS_IMETHOD ConvertImageToBitmap(gfxImageSurface* aImage,
                                        HBITMAP& outBitmap);
        NS_IMETHOD ConvertImageToIcon(gfxImageSurface* aImage,
                                      HICON& outIcon);
        NS_IMETHOD ConvertImageToCursor(gfxImageSurface* aImage,
                                        PRUint32 aHotspotX,
                                        PRUint32 aHotspotY,
                                        HCURSOR& outCursor);

     protected:
        nsresult ImageToIcon(gfxImageSurface* aImage,
          PRBool aIcon, PRUint32 aHotspotX, PRUint32 aHotspotY,
          HICON& _retval);
};


// {7CA6DC5F-F216-421d-AD8F-30C6E8FCC3E3}
#define NS_IMAGE_TO_BITMAP_CID \
{ 0x7ca6dc5f, 0xf216, 0x421d, \
  { 0xad, 0x8f, 0x30, 0xc6, 0xe8, 0xfc, 0xc3, 0xe3 } }
#define NS_IMAGE_TO_BITMAP_CONTRACTID  "@mozilla.org/widget/image-to-win32-hbitmap;1"
#define NS_IMAGE_TO_BITMAP_CLASSNAME   "Image to bitmap converter"

#endif
