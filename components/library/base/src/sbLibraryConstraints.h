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

#ifndef __SB_LIBRARY_CONSTRAINTS_H__
#define __SB_LIBRARY_CONSTRAINTS_H__

#include <sbILibraryConstraints.h>

#include <nsAutoPtr.h>
#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsIClassInfo.h>
#include <nsISerializable.h>

class sbLibraryConstraint;
class sbLibraryConstraintGroup;
typedef nsTArray<nsString> sbStringArray;
typedef nsClassHashtable<nsStringHashKey, sbStringArray> sbConstraintGroup;
typedef nsRefPtr<sbLibraryConstraintGroup> sbConstraintGroupRefPtr;
typedef nsTArray<sbConstraintGroupRefPtr> sbConstraintArray;

class sbLibraryConstraintBuilder : public sbILibraryConstraintBuilder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYCONSTRAINTBUILDER

  nsresult Init();
private:
  nsresult EnsureConstraint();
  nsRefPtr<sbLibraryConstraint> mConstraint;
};

class sbLibraryConstraint : public sbILibraryConstraint,
                            public nsISerializable,
                            public nsIClassInfo
{
friend class sbLibraryConstraintBuilder;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYCONSTRAINT
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  sbLibraryConstraint();

private:
  nsresult Init();
  nsresult Intersect();
  nsresult AddToCurrent(const nsAString& aProperty, sbStringArray* aArray);
  PRBool IsValid();

  PRBool mInitialized;
  sbConstraintArray mConstraint;
};

class sbLibraryConstraintGroup : public sbILibraryConstraintGroup
{
friend class sbLibraryConstraint;
friend class sbLibraryConstraintBuilder;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYCONSTRAINTGROUP

  sbLibraryConstraintGroup();

private:
  nsresult Init();
  PRBool IsEmpty();
  nsresult Add(const nsAString& aProperty, sbStringArray* aArray);
  nsresult Read(nsIObjectInputStream* aStream);
  nsresult Write(nsIObjectOutputStream* aStream);

  static PLDHashOperator PR_CALLBACK
    AddKeysToArrayCallback(nsStringHashKey::KeyType aKey,
                           sbStringArray* aEntry,
                           void* aUserData);

  PRBool mInitialized;
  sbConstraintGroup mConstraintGroup;
};

class sbLibrarySort : public sbILibrarySort,
                      public nsISerializable,
                      public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILIBRARYSORT
  NS_DECL_NSISERIALIZABLE
  NS_DECL_NSICLASSINFO

  sbLibrarySort();

private:
  PRBool mInitialized;
  nsString mProperty;
  PRBool mIsAscending;
};

#endif /* __SB_LIBRARY_CONSTRAINTS_H__ */

