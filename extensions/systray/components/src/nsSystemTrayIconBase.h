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

#ifndef nsSystemTrayIconBase_h__
#define nsSystemTrayIconBase_h__

#include "nsISupports.h"
#include "nsIDOMEventTarget.h"
#include "nsPoint.h"
// for SetImageFromURI
#include "imgIDecoderObserver.h"
#include "imgIRequest.h"
#include "nsCOMPtr.h"

class nsIURI;
class nsIDOMEventTarget;

// this is a private class; it maps to a actual icon in the tray
class nsSystemTrayIconBase : public imgIDecoderObserver
{
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODEROBSERVER
  NS_DECL_IMGICONTAINEROBSERVER
public:
  nsSystemTrayIconBase();
  ~nsSystemTrayIconBase();
  nsresult SetImageFromURI(nsIURI* aURI);
  nsresult SetEventTarget(nsIDOMEventTarget* aEventTarget);

protected:
  virtual nsPoint GetPopupPosition() = 0;

protected:
  // for SetImageFromURI
  nsCOMPtr<imgIRequest> mRequest;
  nsCOMPtr<nsIDOMEventTarget> mEventTarget;
  void DispatchEvent(const nsAString& aType, PRUint16 aButton, PRInt32 aDetail,
                     bool aCtrlKey, bool aAltKey,
                     bool aShiftKey, bool aMetaKey);
};

#endif /* nsSystemTrayIconBase_h__ */
