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
                            bool aUserViewable,
                            bool aUserEditable,
                            bool aRemoteReadable,
                            bool aRemoteWritable);

  nsresult RegisterDuration(const nsAString& aPropertyID,
                            const nsAString& aDisplayKey,
                            nsIStringBundle* aStringBundle,
                            bool aUserViewable,
                            bool aUserEditable,
                            bool aRemoteReadable,
                            bool aRemoteWritable);

  nsresult RegisterNumber(const nsAString& aPropertyID,
                          const nsAString& aDisplayKey,
                          nsIStringBundle* aStringBundle,
                          bool aUserViewable,
                          bool aUserEditable,
                          PRInt32 aMinValue,
                          bool aHasMinValue,
                          PRInt32 aMaxValue,
                          bool aHasMaxValue,
                          bool aRemoteReadable,
                          bool aRemoteWritable,
                          sbIPropertyUnitConverter *aConverter,
                          sbIPropertyArray* aSecondarySort = nsnull);

  nsresult RegisterProgress(const nsAString& aValuePropertyID,
                            const nsAString& aValueDisplayKey,
                            const nsAString& aModePropertyID,
                            const nsAString& aModeDisplayKey,
                            nsIStringBundle* aStringBundle,
                            bool aUserViewable,
                            bool aUserEditable,
                            bool aRemoteReadable,
                            bool aRemoteWritable);

  // Register a property for the sbIPropertyInfo interface. Use this
  // registration for:
  //   1. Text properties in the top-level media items table
  //   2. Any text-based property where the collation transforms provided
  //      by sbITextPropertyInfo are not desired.
  nsresult RegisterBlob(const nsAString& aPropertyID,
                        const nsAString& aDisplayKey,
                        nsIStringBundle* aStringBundle,
                        bool aUserViewable,
                        bool aUserEditable,
                        bool aUsedInIdentity,
                        PRUint32 aNullSort,
                        bool aHasNullSort,
                        bool aRemoteReadable,
                        bool aRemoteWritable);

  nsresult RegisterText(const nsAString& aPropertyID,
                        const nsAString& aDisplayKey,
                        nsIStringBundle* aStringBundle,
                        bool aUserViewable,
                        bool aUserEditable,
                        bool aUsedInIdentity,
                        PRUint32 aNullSort,
                        bool aHasNullSort,
                        bool aRemoteReadable,
                        bool aRemoteWritable,
                        bool aCompressWhitespace = PR_TRUE,
                        sbIPropertyArray* aSecondarySort = nsnull);

  nsresult RegisterURI(const nsAString& aPropertyID,
                       const nsAString& aDisplayKey,
                       nsIStringBundle* aStringBundle,
                       bool aUserViewable,
                       bool aUserEditable,
                       bool aRemoteReadable,
                       bool aRemoteWritable);

  nsresult RegisterImage(const nsAString& aPropertyID,
                         const nsAString& aDisplayKey,
                         nsIStringBundle* aStringBundle,
                         bool aUserViewable,
                         bool aUserEditable,
                         bool aRemoteReadable,
                         bool aRemoteWritable);

  nsresult RegisterBoolean(const nsAString &aPropertyID,
                           const nsAString &aDisplayKey,
                           nsIStringBundle* aStringBundle,
                           bool aUserViewable,
                           bool aUserEditable,
                           bool aRemoteReadable,
                           bool aRemoteWritable,
                           bool aShouldSuppress = PR_TRUE);

  nsresult RegisterImageLink(const nsAString &aPropertyID,
                             const nsAString &aDisplayKey,
                             nsIStringBundle* aStringBundle,
                             bool aUserViewable,
                             bool aUserEditable,
                             bool aRemoteReadable,
                             bool aRemoteWritable,
                             const nsAString &aUrlPropertyID);

  nsresult RegisterTrackTypeImageLabel(const nsAString& aPropertyID,
                                       const nsAString& aDisplayKey,
                                       nsIStringBundle* aStringBundle,
                                       bool aUserViewable,
                                       bool aUserEditable,
                                       bool aRemoteReadable,
                                       bool aRemoteWritable,
                                       const nsAString &aUrlPropertyID);

  nsresult RegisterDummy(sbDummyPropertyInfo *aDummyProperty,
                         const nsAString &aPropertyID,
                         const nsAString &aDisplayKey,
                         nsIStringBundle* aStringBundle);

  nsresult SetRemoteAccess(sbIPropertyInfo* aProperty,
                           bool aRemoteReadable,
                           bool aRemoteWritable);
protected:
  nsInterfaceHashtableMT<nsStringHashKey, sbIPropertyInfo> mPropInfoHashtable;

  // Maps property ID to all properties that depend on that ID in some way
  nsInterfaceHashtableMT<nsStringHashKey, sbIPropertyArray> mPropDependencyMap;

  PRLock* mPropIDsLock;
  nsTArray<nsString> mPropIDs;
};

#endif /* __SBPROPERTYMANAGER_H__ */
