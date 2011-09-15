/* vim: set fileencoding=utf-8 shiftwidth=2 : */
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
 * The Original Code is toolkit/components/systray
 *
 * The Initial Developer of the Original Code is
 * Mook <mook.moz+cvs.mozilla.org@gmail.com>.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Brad Peterson <b_peterson@yahoo.com>
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

#include "nsSystemTrayServiceGTK.h"
#include "nsIURI.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindow.h"
#include "nsIDOMXULElement.h"
#include "nsStringGlue.h"
#include "nsIDOMEventTarget.h"

NS_IMPL_ISUPPORTS1(nsSystemTrayService,
                   nsISystemTrayService)

nsSystemTrayService::nsSystemTrayService()
{
  mIconDataMap.Init();
}

nsSystemTrayService::~nsSystemTrayService()
{
  // no need to clean up the icons, because the hashtable will delete the
  // wrappers, which will remove the icons for us
}

/* void showIcon (in AString iconId, in nsIURI imageURI, in nsIDOMWindow window, in nsIBaseWindow baseWindow); */
NS_IMETHODIMP nsSystemTrayService::ShowIcon(const nsAString & aIconId,
                                            nsIURI *aImageURI,
                                            nsIDOMWindow *aDOMWindow)
{
  nsresult rv;
  nsCOMPtr<nsSystemTrayIconGTK> icon;
  if (mIconDataMap.Get(aIconId, getter_AddRefs(icon))) {
    // we are updating an old icon
    rv = icon->SetImageFromURI(aImageURI);
    // XXX Mook: hack until I figure out what to do with missing images
    if (NS_FAILED(rv)) {
      return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
    }
    return NS_OK;
  }

  // first make sure that we can find the element associated with the
  // <trayicon> element
  nsCOMPtr<nsIDOMDocument> document;
  rv = aDOMWindow->GetDocument(getter_AddRefs(document));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDOMElement> iconElement;
  rv = document->GetElementById(aIconId, getter_AddRefs(iconElement));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIDOMEventTarget> eventTarget =
    do_QueryInterface(iconElement, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // okay, make a new icon and tell it about the event target
  icon = new nsSystemTrayIconGTK();
  if (!icon || !icon->mIcon) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  
  rv = icon->SetEventTarget(eventTarget);
  NS_ENSURE_SUCCESS(rv, rv);
  
  mIconDataMap.Put(aIconId, icon);
  rv = icon->SetImageFromURI(aImageURI);
  if (NS_FAILED(rv)) {
    // XXX Mook: hack until I figure out what to do with missing images
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  return NS_OK;
}

/* void hideIcon (in AString iconId); */
NS_IMETHODIMP nsSystemTrayService::HideIcon(const nsAString & aIconId)
{
  if (!mIconDataMap.Get(aIconId, nsnull)) {
    // we don't know about this icon in the first place
    return NS_ERROR_INVALID_ARG;
  }
  // destructing the icon removes it for us
  mIconDataMap.Remove(aIconId);
  return NS_OK;
}

/* void setTitle (in AString iconId, in AString title); */
NS_IMETHODIMP nsSystemTrayService::SetTitle(const nsAString & aIconId, const nsAString & aTitle)
{
  nsCOMPtr<nsSystemTrayIconGTK> icon;
  mIconDataMap.Get(aIconId, getter_AddRefs(icon));
  if (!icon) {
    return NS_ERROR_INVALID_ARG;
  }
  
  // XXX Mook: OOM
  gtk_status_icon_set_tooltip(icon->mIcon, NS_ConvertUTF16toUTF8(aTitle).get());
  return NS_OK;
}
