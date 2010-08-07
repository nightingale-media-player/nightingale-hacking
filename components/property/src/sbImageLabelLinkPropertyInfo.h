/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef __SBIMAGELABELLINKPROPERTYINFO_H__
#define __SBIMAGELABELLINKPROPERTYINFO_H__

#include "sbImageLinkPropertyInfo.h"

#include <sbIPropertyManager.h>
#include <sbITreeViewPropertyInfo.h>
#include <sbIClickablePropertyInfo.h>
#include <sbIPropertyBuilder.h>

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include "sbSimpleButtonPropertyBuilder.h"

class sbImageLabelLinkPropertyInfo : public sbImageLinkPropertyInfo
{
public:
  typedef nsClassHashtable<nsCStringHashKey, nsCString> ImageMap_t;
  typedef nsClassHashtable<nsCStringHashKey, nsString> LabelMap_t;
  typedef nsTHashtable<nsISupportsHashKey> InterfaceSet_t;

public:
  sbImageLabelLinkPropertyInfo(ImageMap_t *&aImages,
                               LabelMap_t *&aLabels,
                               InterfaceSet_t *&aClickHandlers);
  virtual ~sbImageLabelLinkPropertyInfo();

public:
  nsresult Init();

  /* setters because I think a billion constructor arguments is ugly */
  NS_IMETHOD SetPropertyID(const nsAString& aPropertyID);
  NS_IMETHOD SetDisplayName(const nsAString& aDisplayName);
  NS_IMETHOD SetLocalizationKey(const nsAString& aLocalizationKey);
  NS_IMETHOD SetRemoteReadable(PRBool aRemoteReadable);
  NS_IMETHOD SetRemoteWritable(PRBool aRemoteWritable);
  NS_IMETHOD SetUserViewable(PRBool aUserViewable);
  NS_IMETHOD SetUserEditable(PRBool aUserEditable);
  NS_IMETHOD SetUrlPropertyID(const nsAString& aUrlPropertyID);
  
  /* partial implementation of sbITreeViewPropertyInfo */
  NS_IMETHOD GetImageSrc(const nsAString& aValue, nsAString& _retval);
  NS_IMETHOD GetCellProperties(const nsAString& aValue, nsAString& _retval);

  /* partial implementation of sbIClickablePropertyInfo */
  NS_IMETHOD HitTest(const nsAString& aCurrentValue,
                     const nsAString& aPart,
                     PRUint32 aBoxWidth,
                     PRUint32 aBoxHeight,
                     PRUint32 aMouseX,
                     PRUint32 aMouseY,
                     PRBool* _retval);
  NS_IMETHOD OnClick(sbIMediaItem *aItem,
                     nsISupports *aEvent,
                     nsISupports *aContext,
                     PRBool *_retval);

  /* partial implementation of sbIPropertyInfo */
  NS_IMETHOD Format(const nsAString& aValue, nsAString& _retval);
private:
  ImageMap_t *mImages;
  LabelMap_t *mLabels;
  InterfaceSet_t *mClickHandlers;
};

#endif /* __SBIMAGELABELLINKPROPERTYINFO_H__ */

