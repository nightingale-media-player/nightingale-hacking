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

#ifndef NSIIMAGETOBITMAP_H_
#define NSIIMAGETOBITMAP_H_

#include "nsISupports.h"

// {337D3FFB-F15F-4df5-83E5-4C7C3D595392}
#define NSIIMAGETOBITMAP_IID \
{ 0x337d3ffb, 0xf15f, 0x4df5, \
  { 0x83, 0xe5, 0x4c, 0x7c, 0x3d, 0x59, 0x53, 0x92 } }

class gfxImageSurface;

struct HBITMAP__;
struct HICON__;

typedef struct HBITMAP__ *HBITMAP;
typedef struct HICON__ *HICON;
typedef HICON HCURSOR;

/**
 * An interface that allows converting a gfxImageSurface to a Windows bitmap.
 */
class nsIImageToBitmap : public nsISupports {
    public:
        NS_DECLARE_STATIC_IID_ACCESSOR(NSIIMAGETOBITMAP_IID)

        // convert an image to a DDB
        NS_IMETHOD ConvertImageToBitmap(gfxImageSurface* aImage,
                                        HBITMAP& outBitmap) = 0;
        
        // convert an image to a HICON
        NS_IMETHOD ConvertImageToIcon(gfxImageSurface* aImage,
                                      HICON& outIcon) = 0;
        
        // convert an image to a HCURSOR
        NS_IMETHOD ConvertImageToCursor(gfxImageSurface* aImage,
                                        PRUint32 aHotspotX,
                                        PRUint32 aHotspotY,
                                        HCURSOR& outCursor) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIImageToBitmap, NSIIMAGETOBITMAP_IID)

#endif
