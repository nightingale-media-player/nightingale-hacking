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

#ifndef __SBPROPERTYMANAGER_H__
#define __SBPROPERTYMANAGER_H__

#include <sbIPropertyManager.h>
#include <nsIObserver.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <nsTArray.h>
#include <nsInterfaceHashtable.h>

struct nsModuleComponentInfo;
class nsIComponentManager;
class nsIFile;
class sbIPropertyUnitConverter;
class sbDummyPropertyInfo;

class sbPropertyManager : public sbIPropertyManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYMANAGER

  sbPropertyManager();
  virtual ~sbPropertyManager();

  NS_METHOD Init();
  NS_METHOD CreateSystemProperties();
private:

  nsresult RegisterFilterListPickerProperties();

  nsresult RegisterDateTime(const nsAString& aPropertyID,
                            const nsAString& aDisplayKey,
                            PRInt32 aType,
                            nsIStringBundle* aStringBundle,
                            PRBool aUserViewable,
                            PRBool aUserEditable,
                            PRBool aRemoteReadable,
                            PRBool aRemoteWritable);

  nsresult RegisterDuration(const nsAString& aPropertyID,
                            const nsAString& aDisplayKey,
                            nsIStringBundle* aStringBundle,
                            PRBool aUserViewable,
                            PRBool aUserEditable,
                            PRBool aRemoteReadable,
                            PRBool aRemoteWritable);

  nsresult RegisterNumber(const nsAString& aPropertyID,
                          const nsAString& aDisplayKey,
                          nsIStringBundle* aStringBundle,
                          PRBool aUserViewable,
                          PRBool aUserEditable,
                          PRInt32 aMinValue,
                          PRBool aHasMinValue,
                          PRInt32 aMaxValue,
                          PRBool aHasMaxValue,
                          PRBool aRemoteReadable,
                          PRBool aRemoteWritable,
                          sbIPropertyUnitConverter *aConverter,
                          sbIPropertyArray* aSecondarySort = nsnull);

  nsresult RegisterProgress(const nsAString& aValuePropertyID,
                            const nsAString& aValueDisplayKey,
                            const nsAString& aModePropertyID,
                            const nsAString& aModeDisplayKey,
                            nsIStringBundle* aStringBundle,
                            PRBool aUserViewable,
                            PRBool aUserEditable,
                            PRBool aRemoteReadable,
                            PRBool aRemoteWritable);

  nsresult RegisterText(const nsAString& aPropertyID,
                        const nsAString& aDisplayKey,
                        nsIStringBundle* aStringBundle,
                        PRBool aUserViewable,
                        PRBool aUserEditable,
                        PRUint32 aNullSort,
                        PRBool aHasNullSort,
                        PRBool aRemoteReadable,
                        PRBool aRemoteWritable,
                        PRBool aCompressWhitespace = PR_TRUE,
                        sbIPropertyArray* aSecondarySort = nsnull);

  nsresult RegisterURI(const nsAString& aPropertyID,
                       const nsAString& aDisplayKey,
                       nsIStringBundle* aStringBundle,
                       PRBool aUserViewable,
                       PRBool aUserEditable,
                       PRBool aRemoteReadable,
                       PRBool aRemoteWritable);

  nsresult RegisterImage(const nsAString& aPropertyID,
                         const nsAString& aDisplayKey,
                         nsIStringBundle* aStringBundle,
                         PRBool aUserViewable,
                         PRBool aUserEditable,
                         PRBool aRemoteReadable,
                         PRBool aRemoteWritable);

  nsresult RegisterBoolean(const nsAString &aPropertyID,
                           const nsAString &aDisplayKey,
                           nsIStringBundle* aStringBundle,
                           PRBool aUserViewable,
                           PRBool aUserEditable,
                           PRBool aRemoteReadable,
                           PRBool aRemoteWritable,
                           PRBool aShouldSuppress = PR_TRUE);

  nsresult RegisterImageLink(const nsAString &aPropertyID,
                             const nsAString &aDisplayKey,
                             nsIStringBundle* aStringBundle,
                             PRBool aUserViewable,
                             PRBool aUserEditable,
                             PRBool aRemoteReadable,
                             PRBool aRemoteWritable,
                             const nsAString &aUrlPropertyID);

  nsresult RegisterTrackTypeImageLabel(const nsAString& aPropertyID,
                                       const nsAString& aDisplayKey,
                                       nsIStringBundle* aStringBundle,
                                       PRBool aUserViewable,
                                       PRBool aUserEditable,
                                       PRBool aRemoteReadable,
                                       PRBool aRemoteWritable,
                                       const nsAString &aUrlPropertyID);

  nsresult RegisterDummy(sbDummyPropertyInfo *aDummyProperty,
                         const nsAString &aPropertyID,
                         const nsAString &aDisplayKey,
                         nsIStringBundle* aStringBundle);

  nsresult SetRemoteAccess(sbIPropertyInfo* aProperty,
                           PRBool aRemoteReadable,
                           PRBool aRemoteWritable);
protected:
  nsInterfaceHashtableMT<nsStringHashKey, sbIPropertyInfo> mPropInfoHashtable;

  // Maps property ID to all properties that depend on that ID in some way
  nsInterfaceHashtableMT<nsStringHashKey, sbIPropertyArray> mPropDependencyMap;

  PRLock* mPropIDsLock;
  nsTArray<nsString> mPropIDs;
};

#endif /* __SBPROPERTYMANAGER_H__ */
