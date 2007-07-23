/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

#ifndef __SBPROPERTYMANAGER_H__
#define __SBPROPERTYMANAGER_H__

#include <sbIPropertyManager.h>
#include <nsIObserver.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <nsTArray.h>
#include <nsInterfaceHashtable.h>

class nsModuleComponentInfo;
class nsIComponentManager;
class nsIFile;

class sbPropertyManager : public sbIPropertyManager,
                          public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIPROPERTYMANAGER

  sbPropertyManager();
  virtual ~sbPropertyManager();

  static NS_METHOD RegisterSelf(nsIComponentManager* aCompMgr,
    nsIFile* aPath,
    const char* aLoaderStr,
    const char* aType,
    const nsModuleComponentInfo *aInfo);

  NS_METHOD Init();
  NS_METHOD CreateSystemProperties();
private:

  nsresult RegisterDateTime(const nsAString& aPropertyName,
                            const nsAString& aDisplayKey,
                            PRInt32 aType,
                            nsIStringBundle* aStringBundle,
                            PRBool aUserViewable,
                            PRBool aUserEditable);

  nsresult RegisterNumber(const nsAString& aPropertyName,
                          const nsAString& aDisplayKey,
                          nsIStringBundle* aStringBundle,
                          PRBool aUserViewable,
                          PRBool aUserEditable,
                          PRInt32 aMinValue = 0,
                          PRBool aHasMinValue = PR_FALSE,
                          PRInt32 aMaxValue = 0,
                          PRBool aHasMaxValue = PR_FALSE);

  nsresult RegisterProgress(const nsAString& aValuePropertyName,
                            const nsAString& aValueDisplayKey,
                            const nsAString& aModePropertyName,
                            const nsAString& aModeDisplayKey,
                            nsIStringBundle* aStringBundle,
                            PRBool aUserViewable,
                            PRBool aUserEditable);

  nsresult RegisterText(const nsAString& aPropertyName,
                        const nsAString& aDisplayKey,
                        nsIStringBundle* aStringBundle,
                        PRBool aUserViewable,
                        PRBool aUserEditable,
                        PRUint32 aNullSort = 0,
                        PRBool aHasNullSort = PR_FALSE);

  nsresult RegisterURI(const nsAString& aPropertyName,
                       const nsAString& aDisplayKey,
                       nsIStringBundle* aStringBundle,
                       PRBool aUserViewable,
                       PRBool aUserEditable);

  nsresult RegisterCheckbox(const nsAString& aPropertyName,
                            const nsAString& aDisplayKey,
                            nsIStringBundle* aStringBundle,
                            PRBool aUserViewable,
                            PRBool aUserEditable);

  nsresult RegisterRatingProperty(nsIStringBundle* aStringBundle);

  nsresult RegisterButton(const nsAString& aPropertyName,
                          const nsAString& aDisplayKey,
                          nsIStringBundle* aStringBundle,
                          const nsAString& aLabel);

protected:
  nsInterfaceHashtableMT<nsStringHashKey, sbIPropertyInfo> mPropInfoHashtable;
  
  PRLock* mPropNamesLock;
  nsTArray<nsString> mPropNames;
};

#endif /* __SBPROPERTYMANAGER_H__ */
