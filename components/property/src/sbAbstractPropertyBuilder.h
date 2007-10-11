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

#ifndef __SBABSTRACTPROPERTYBUILDER_H__
#define __SBABSTRACTPROPERTYBUILDER_H__

#include <sbIPropertyBuilder.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsIStringBundle.h>

#define NS_FORWARD_SBIPROPERTYBUILDER_NO_GET(_to) \
  NS_IMETHOD GetPropertyName(nsAString & aPropertyName) { return _to GetPropertyName(aPropertyName); } \
  NS_IMETHOD SetPropertyName(const nsAString & aPropertyName) { return _to SetPropertyName(aPropertyName); } \
  NS_IMETHOD GetDisplayName(nsAString & aDisplayName) { return _to GetDisplayName(aDisplayName); } \
  NS_IMETHOD SetDisplayName(const nsAString & aDisplayName) { return _to SetDisplayName(aDisplayName); } \
  NS_IMETHOD GetDisplayNameKey(nsAString & aDisplayNameKey) { return _to GetDisplayNameKey(aDisplayNameKey); } \
  NS_IMETHOD SetDisplayNameKey(const nsAString & aDisplayNameKey) { return _to SetDisplayNameKey(aDisplayNameKey); } \
  NS_IMETHOD GetUserViewable(PRBool * aUserViewable) { return _to GetUserViewable(aUserViewable); } \
  NS_IMETHOD SetUserViewable(PRBool aUserViewable) { return _to SetUserViewable(aUserViewable); } \
  NS_IMETHOD GetUserEditable(PRBool * aUserEditable) { return _to GetUserEditable(aUserEditable); } \
  NS_IMETHOD SetUserEditable(PRBool aUserEditable) { return _to SetUserEditable(aUserEditable); } \
  NS_IMETHOD GetRemoteReadable(PRBool * aRemoteReadable) { return _to GetRemoteReadable(aRemoteReadable); } \
  NS_IMETHOD SetRemoteReadable(PRBool aRemoteReadable) { return _to SetRemoteReadable(aRemoteReadable); } \
  NS_IMETHOD GetRemoteWritable(PRBool * aRemoteWritable) { return _to GetRemoteWritable(aRemoteWritable); } \
  NS_IMETHOD SetRemoteWritable(PRBool aRemoteWritable) { return _to SetRemoteWritable(aRemoteWritable); }

class sbAbstractPropertyBuilder : public sbIPropertyBuilder
{
public:

  NS_DECL_ISUPPORTS
  NS_DECL_SBIPROPERTYBUILDER

  nsresult Init();

  static nsresult GetStringFromName(nsIStringBundle* aBundle,
                                    const nsAString& aName,
                                    nsAString& _retval);
  static nsresult CreateBundle(const char* aURLSpec, 
                               nsIStringBundle** _retval);

protected:

  nsresult GetFinalDisplayName(nsAString& aDisplayName);

  nsCOMPtr<nsIStringBundle> mBundle;

  nsString mPropertyName;
  nsString mDisplayName;
  nsString mDisplayNameKey;
  PRBool mUserViewable;
  PRBool mUserEditable;
  PRBool mRemoteReadable;
  PRBool mRemoteWritable;

};

#endif /* __SBABSTRACTPROPERTYBUILDER_H__ */

