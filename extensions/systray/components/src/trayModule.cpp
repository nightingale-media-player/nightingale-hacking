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
 * The Original Code is MinimizeToTray.
 *
 * The Initial Developer of the Original Code are
 * Mark Yen and Brad Peterson.
 * Portions created by the Initial Developer are Copyright (C) 2004
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mark Yen <mook.moz+cvs.mozilla.org@gmail.com>, Original author
 *   Brad Peterson <b_peterson@yahoo.com>, Original author
 *   Daniel Glazman <daniel.glazman@disruptive-innovations.com>
 *   Matthew Gertner <matthew@allpeers.com>
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

#include "nsIGenericFactory.h"
#include "nsServiceManagerUtils.h"

#if XP_WIN
#include "nsSystemTrayServiceWin.h"
#include "nsWindowUtilWin.h"
#include "nsImageToBitmap.h"
#elif MOZ_WIDGET_GTK2
#include "nsSystemTrayServiceGTK.h"
#include "nsWindowUtilGTK.h"
#else
#include "nsSystemTrayService.h"
#endif

#if XP_WIN || MOZ_WIDGET_GTK2
NS_GENERIC_FACTORY_CONSTRUCTOR(nsWindowUtil)
#endif
#if XP_WIN
NS_GENERIC_FACTORY_CONSTRUCTOR(nsImageToBitmap);
#endif

NS_GENERIC_FACTORY_CONSTRUCTOR(nsSystemTrayService)

static nsModuleComponentInfo components[] =
{
#if XP_WIN || MOZ_WIDGET_GTK2
  {
    NS_WINDOWUTIL_CLASSNAME,
    NS_WINDOWUTIL_CID,
    NS_WINDOWUTIL_CONTRACTID,
    nsWindowUtilConstructor
  },
#endif
#if XP_WIN
  {
    NS_IMAGE_TO_BITMAP_CLASSNAME,
    NS_IMAGE_TO_BITMAP_CID,
    NS_IMAGE_TO_BITMAP_CONTRACTID,
    nsImageToBitmapConstructor
  },
#endif
  {
    NS_SYSTEMTRAYSERVICE_CLASSNAME,
    NS_SYSTEMTRAYSERVICE_CID,
    NS_SYSTEMTRAYSERVICE_CONTRACTID,
    nsSystemTrayServiceConstructor
  }
};

NS_IMPL_NSGETMODULE(systrayModule, components)
