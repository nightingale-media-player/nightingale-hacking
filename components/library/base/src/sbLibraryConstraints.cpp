/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbLibraryConstraints.h"
#include "sbLibraryCID.h"

#include <nsAutoPtr.h>
#include <nsArrayEnumerator.h>
#include <nsCOMArray.h>
#include <nsIClassInfoImpl.h>
#include <nsINetUtil.h>
#include <nsIObjectInputStream.h>
#include <nsIObjectOutputStream.h>
#include <nsIProgrammingLanguage.h>
#include <nsIStringEnumerator.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>

#include <sbTArrayStringEnumerator.h>
#include <sbStringUtils.h>

/**
 * == Constraint / Constraint Group String Serialization Format ==
 *
 * Whitespace is significant in the constraint serialization format.
 *
 * The constraint serialization is a JSON-style array,
 * constraint := '[' constraint-group ( ', ' constraint-group )* ']'
 *
 * The constraint group serialzation is a JSON-style object,
 * constraint-group := '{' constraint-entry ( ', ' constraint-entry )* '}'
 * constraint-entry := '"' property-name '": ' property-values
 *
 * The property name is assumed to not contain quotation mark characters
 * (which are invalid in URIs).  Any other characters are taken literally; this
 * may result in invalid property names being used if the input is not correct.
 * See sbIPropertyManager and sbStandardProperties.h.
 *
 * The property values serialization is a JSON-style string array,
 * property-values := '[' '"' escaped-value '"' ( ', "' escaped-value '"' )* ']'
 * where escaped-value is the value of the constraint, as escaped via
 * nsINetUtils::escapeString() with nsINetUtils::ESCAPE_XALPHAS on the UTF-8
 * encoding of the string.
 *
 * Sample (standard) constraint:
 * [{"http://songbirdnest.com/data/1.0#isList": ["0"]}, {"http://songbirdnest.com/data/1.0#hidden": ["0"]}]
 *
 * Sample constraint with escaped value:
 * [{"http://songbirdnest.com/data/1.0#genre": ["%E6%B8%AC%20%E8%A9%A6"]}]
 */

/*
 * sbLibraryConstraintBuilder
 */
NS_IMPL_ISUPPORTS1(sbLibraryConstraintBuilder,
                   sbILibraryConstraintBuilder)


static inline nsresult CheckStringAndSkip(const nsAString & aSource,
                                          PRUint32 & offset,
                                          const nsAString & aSubstring)
{
  if (!Substring(aSource, offset, aSubstring.Length()).Equals(aSubstring)) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  offset += aSubstring.Length();
  return NS_OK;
}

/* sbILibraryConstraintBuilder parseFromString (in AString aSerializedConstraint); */
NS_IMETHODIMP
sbLibraryConstraintBuilder::ParseFromString(const nsAString & aConstraint,
                                            sbILibraryConstraintBuilder **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRUint32 offset = 0, next;

  rv = EnsureConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 groupCount;
  rv = mConstraint->GetGroupCount(&groupCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(groupCount == 1, NS_ERROR_ALREADY_INITIALIZED);

  nsCOMPtr<sbILibraryConstraintGroup> group;
  rv = mConstraint->GetGroup(0, getter_AddRefs(group));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringEnumerator> props;
  rv = group->GetProperties(getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  rv = props->HasMore(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_FALSE(hasMore, NS_ERROR_ALREADY_INITIALIZED);

  nsCOMPtr<nsINetUtil> netUtil =
    do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CheckStringAndSkip(aConstraint, offset, NS_LITERAL_STRING("[{\""));
  NS_ENSURE_SUCCESS(rv, rv);

  do {
    // find the leading " at the end of the of property name
    next = aConstraint.FindChar(PRUnichar('"'), offset);
    if (next < 0) {
      return NS_ERROR_ILLEGAL_VALUE;
    }
    // get the property name
    nsString property(Substring(aConstraint, offset, next - offset));
    offset = next + 1;

    // check for the start of the values
    rv = CheckStringAndSkip(aConstraint, offset, NS_LITERAL_STRING(": [\""));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoPtr<sbStringArray> array(new sbStringArray);
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    while (true) {
      // look for the " at the end of the value
      next = aConstraint.FindChar(PRUnichar('"'), offset);
      if (next < 0) {
        return NS_ERROR_ILLEGAL_VALUE;
      }
      nsString value(Substring(aConstraint, offset, next - offset));
      // we need to unescape the string
      nsCString unescapedValue;
      rv = netUtil->UnescapeString(NS_ConvertUTF16toUTF8(value),
                                   nsINetUtil::ESCAPE_XALPHAS,
                                   unescapedValue);
      NS_ENSURE_SUCCESS(rv, rv);
      nsString* added = array->AppendElement(NS_ConvertUTF8toUTF16(unescapedValue));
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
      offset = next + 1;
      if (Substring(aConstraint, offset, 1).EqualsLiteral("]")) {
        // end of the values
        ++offset;
        break;
      }
      rv = CheckStringAndSkip(aConstraint, offset, NS_LITERAL_STRING(", \""));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    rv = mConstraint->AddToCurrent(property, array);
    NS_ENSURE_SUCCESS(rv, rv);
    array.forget();

    // check for end of group
    if (Substring(aConstraint, offset).EqualsLiteral("}]")) {
      // end of the group
      break;
    }
    rv = CheckStringAndSkip(aConstraint, offset, NS_LITERAL_STRING("}, {\""));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mConstraint->Intersect();
    NS_ENSURE_SUCCESS(rv, rv);

  } while (true);
  NS_ENSURE_TRUE(offset == aConstraint.Length() - 2, NS_ERROR_ILLEGAL_VALUE);

  NS_ADDREF(*_retval = this);
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintBuilder::IncludeConstraint(sbILibraryConstraint* aConstraint,
                                              sbILibraryConstraintBuilder** _retval)
{
  NS_ENSURE_ARG_POINTER(aConstraint);
  nsresult rv;

  rv = EnsureConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 groupCount;
  rv = aConstraint->GetGroupCount(&groupCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < groupCount; i++) {
    nsCOMPtr<sbILibraryConstraintGroup> group;
    rv = aConstraint->GetGroup(i, getter_AddRefs(group));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringEnumerator> properties;
    rv = group->GetProperties(getter_AddRefs(properties));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool hasMore;
    while (NS_SUCCEEDED(properties->HasMore(&hasMore)) && hasMore) {
      nsString property;
      rv = properties->GetNext(property);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIStringEnumerator> values;
      rv = group->GetValues(property, getter_AddRefs(values));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = IncludeList(property, values, nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (i + 1 < groupCount) {
      rv = Intersect(nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }

  if (_retval) {
    NS_ADDREF(*_retval = this);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintBuilder::Include(const nsAString& aProperty,
                                    const nsAString& aValue,
                                    sbILibraryConstraintBuilder** _retval)
{
  NS_ENSURE_ARG(!aProperty.IsEmpty());

  nsresult rv;

  rv = EnsureConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: VALIDATE VALUES
  nsAutoPtr<sbStringArray> array(new sbStringArray);
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  nsString* success = array->AppendElement(aValue);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Transfer ownership of array to the callee
  rv = mConstraint->AddToCurrent(aProperty, array.forget());
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    NS_ADDREF(*_retval = this);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintBuilder::IncludeList(const nsAString& aProperty,
                                        nsIStringEnumerator* aValues,
                                        sbILibraryConstraintBuilder** _retval)
{
  NS_ENSURE_ARG(!aProperty.IsEmpty());

  nsresult rv;

  rv = EnsureConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: VALIDATE VALUES
  nsAutoPtr<sbStringArray> array(new sbStringArray);
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  PRBool hasMore;
  while (NS_SUCCEEDED(aValues->HasMore(&hasMore)) && hasMore) {
    nsString value;
    rv = aValues->GetNext(value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString* success = array->AppendElement(value);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Transfer ownership of array to the callee
  rv = mConstraint->AddToCurrent(aProperty, array.forget());
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    NS_ADDREF(*_retval = this);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintBuilder::Intersect(sbILibraryConstraintBuilder** _retval)
{
  nsresult rv;

  rv = EnsureConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_STATE(mConstraint->IsValid());

  rv = mConstraint->Intersect();
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    NS_ADDREF(*_retval = this);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintBuilder::Get(sbILibraryConstraint** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  rv = EnsureConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_STATE(mConstraint->IsValid());

  NS_ADDREF(*_retval = mConstraint);
  mConstraint = nsnull;

  return NS_OK;
}

nsresult
sbLibraryConstraintBuilder::EnsureConstraint()
{
  if (mConstraint) {
    return NS_OK;
  }

  nsRefPtr<sbLibraryConstraint> constraint = new sbLibraryConstraint();
  NS_ENSURE_TRUE(constraint, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = constraint->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mConstraint = constraint;

  return NS_OK;
}

/*
 * sbLibraryConstraint
 */

static NS_DEFINE_CID(kLibraryConstraintCID,
                     SONGBIRD_LIBRARY_CONSTRAINT_CID);

NS_IMPL_ISUPPORTS3(sbLibraryConstraint,
                   sbILibraryConstraint,
                   nsISerializable,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER3(sbLibraryConstraint,
                             sbILibraryConstraint,
                             nsISerializable,
                             nsIClassInfo)

sbLibraryConstraint::sbLibraryConstraint() :
  mInitialized(PR_FALSE)
{
}

NS_IMETHODIMP
sbLibraryConstraint::GetGroupCount(PRUint32* aGroupCount)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aGroupCount);

  *aGroupCount = mConstraint.Length();

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetGroups(nsISimpleEnumerator** aGroups)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aGroups);

  nsCOMArray<sbILibraryConstraintGroup> array;
  PRUint32 length = mConstraint.Length();
  for (PRUint32 i = 0; i < length; i++) {
    PRBool success = array.AppendObject(mConstraint[i]);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  nsresult rv = NS_NewArrayEnumerator(aGroups, array);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetGroup(PRUint32 aIndex,
                              sbILibraryConstraintGroup** _retval)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(aIndex < mConstraint.Length(), NS_ERROR_INVALID_ARG);

  NS_ADDREF(*_retval = mConstraint[aIndex]);
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::Equals(sbILibraryConstraint* aOtherConstraint,
                            PRBool* _retval)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  if (!aOtherConstraint) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  PRUint32 groupCount;
  rv = aOtherConstraint->GetGroupCount(&groupCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mConstraint.Length() != groupCount) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  nsCOMArray<sbILibraryConstraintGroup> matchesRemaining(groupCount);
  for (PRUint32 i = 0; i < groupCount; i++) {

    nsCOMPtr<sbILibraryConstraintGroup> otherGroup;
    rv = aOtherConstraint->GetGroup(i, getter_AddRefs(otherGroup));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = matchesRemaining.AppendObject(otherGroup);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  for (PRUint32 i = 0; i < groupCount; i++) {
    for (PRInt32 j = 0; j < matchesRemaining.Count(); j++) {
      PRBool equals;
      rv = mConstraint[i]->Equals(matchesRemaining[j], &equals);
      NS_ENSURE_SUCCESS(rv, rv);

      if (equals) {
        matchesRemaining.RemoveObjectAt(j);
      }
    }
  }

  *_retval = matchesRemaining.Count() == 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::ToString(nsAString& _retval)
{
  NS_ENSURE_STATE(mInitialized);

  nsString buff;
  buff.AssignLiteral("[");

  PRUint32 length = mConstraint.Length();
  for (PRUint32 i = 0; i < length; i++) {

    nsString temp;
    nsresult rv = mConstraint[i]->ToString(temp);
    NS_ENSURE_SUCCESS(rv, rv);

    buff.Append(temp);
    if (i + 1 < length) {
      buff.AppendLiteral(", ");
    }

  }

  buff.AppendLiteral("]");
  _retval = buff;
  return NS_OK;
}

// nsISerializable
NS_IMETHODIMP
sbLibraryConstraint::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_STATE(!mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  PRUint32 length;
  rv = aStream->Read32(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    sbConstraintGroupRefPtr group = new sbLibraryConstraintGroup();
    NS_ENSURE_TRUE(group, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = group->Read(aStream);
    NS_ENSURE_SUCCESS(rv, rv);

    sbConstraintGroupRefPtr* success = mConstraint.AppendElement(group);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  PRUint32 length = mConstraint.Length();
  rv = aStream->Write32(length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    rv = mConstraint[i]->Write(aStream);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLibraryConstraint::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLibraryConstraint)(count, array);
}

NS_IMETHODIMP
sbLibraryConstraint::GetHelperForLanguage(PRUint32 language,
                                      nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetFlags(PRUint32* aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraint::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kLibraryConstraintCID;
  return NS_OK;
}

nsresult
sbLibraryConstraint::Init()
{
  mInitialized = PR_TRUE;
  return Intersect();
}

nsresult
sbLibraryConstraint::Intersect()
{
  sbConstraintGroupRefPtr group = new sbLibraryConstraintGroup();
  NS_ENSURE_TRUE(group, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = group->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  sbConstraintGroupRefPtr* success = mConstraint.AppendElement(group);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbLibraryConstraint::AddToCurrent(const nsAString& aProperty,
                                  sbStringArray* aArray)
{
  PRUint32 length = mConstraint.Length();
  NS_ENSURE_TRUE(length, NS_ERROR_UNEXPECTED);

  nsresult rv = mConstraint[length - 1]->Add(aProperty, aArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
sbLibraryConstraint::IsValid()
{
  PRUint32 length = mConstraint.Length();
  NS_ENSURE_TRUE(length, NS_ERROR_UNEXPECTED);

  return !mConstraint[length - 1]->IsEmpty();
}

/*
 * sbLibraryConstraintGroup
 */

NS_IMPL_ISUPPORTS1(sbLibraryConstraintGroup,
                   sbILibraryConstraintGroup)

sbLibraryConstraintGroup::sbLibraryConstraintGroup() :
  mInitialized(PR_FALSE)
{
}

/* static */ PLDHashOperator PR_CALLBACK
sbLibraryConstraintGroup::AddKeysToArrayCallback(nsStringHashKey::KeyType aKey,
                                                 sbStringArray* aEntry,
                                                 void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");

  sbStringArray* array = static_cast<sbStringArray*>(aUserData);
  NS_ASSERTION(array, "null userdata");

  nsString* success = array->AppendElement(aKey);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
sbLibraryConstraintGroup::GetProperties(nsIStringEnumerator** aProperties)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aProperties);

  nsAutoTArray<nsString, 10> array;
  mConstraintGroup.EnumerateRead(AddKeysToArrayCallback, &array);
  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&array);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(aProperties);

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintGroup::GetValues(const nsAString& aProperty,
                                    nsIStringEnumerator** _retval)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(_retval);

  sbStringArray* array;
  if (!mConstraintGroup.Get(aProperty, &array)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(array);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintGroup::HasProperty(const nsAString& aProperty,
                                      PRBool* _retval)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = mConstraintGroup.Get(aProperty, nsnull);

  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintGroup::Equals(sbILibraryConstraintGroup* aOtherGroup,
                                 PRBool* _retval)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  *_retval = PR_FALSE;

  if (!aOtherGroup) {
    return NS_OK;
  }

  nsCOMPtr<nsIStringEnumerator> properties;
  rv = GetProperties(getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringEnumerator> otherProperties;
  rv = aOtherGroup->GetProperties(getter_AddRefs(otherProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool equals;
  rv = SB_StringEnumeratorEquals(properties, otherProperties, &equals);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!equals) {
    return NS_OK;
  }

  sbStringArray propertyArray;
  mConstraintGroup.EnumerateRead(AddKeysToArrayCallback, &propertyArray);
  PRUint32 propertyCount = propertyArray.Length();
  NS_ENSURE_TRUE(propertyCount == mConstraintGroup.Count(),
                 NS_ERROR_UNEXPECTED);

  for (PRUint32 i = 0; i < propertyCount; i++) {

    nsCOMPtr<nsIStringEnumerator> values;
    rv = GetValues(propertyArray[i], getter_AddRefs(values));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringEnumerator> otherValues;
    rv = aOtherGroup->GetValues(propertyArray[i], getter_AddRefs(otherValues));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SB_StringEnumeratorEquals(values, otherValues, &equals);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!equals) {
      return NS_OK;
    }
  }

  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibraryConstraintGroup::ToString(nsAString& _retval)
{
  NS_ENSURE_STATE(mInitialized);
  nsString buff(NS_LITERAL_STRING("{"));
  nsCString cbuff;
  nsresult rv;

  nsAutoTArray<nsString, 10> properties;
  mConstraintGroup.EnumerateRead(AddKeysToArrayCallback, &properties);

  nsCOMPtr<nsINetUtil> netUtil =
    do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyCount = properties.Length();
  for (PRUint32 i = 0; i < propertyCount; i++) {
    buff.AppendLiteral("\"");
    buff.Append(properties[i]);
    buff.AppendLiteral("\": [");
    sbStringArray* values;
    PRBool success = mConstraintGroup.Get(properties[i], &values);
    NS_ENSURE_SUCCESS(success, NS_ERROR_UNEXPECTED);
    PRUint32 valueCount = values->Length();
    for (PRUint32 j = 0; j < valueCount; j++) {
      buff.AppendLiteral("\"");
      // we need to escape all quotes in the values
      rv = netUtil->EscapeString(NS_ConvertUTF16toUTF8(values->ElementAt(j)),
                                 nsINetUtil::ESCAPE_XALPHAS,
                                 cbuff);
      NS_ENSURE_SUCCESS(rv, rv);
      buff.Append(NS_ConvertUTF8toUTF16(cbuff));
      buff.AppendLiteral("\"");

      if (j + 1 < valueCount) {
        buff.AppendLiteral(", ");
      }
    }
    buff.AppendLiteral("]");

    if (i + 1 < propertyCount) {
      buff.AppendLiteral(", ");
    }
  }

  buff.AppendLiteral("}");
  _retval = buff;

  return NS_OK;
}

nsresult
sbLibraryConstraintGroup::Init()
{
  PRBool success = mConstraintGroup.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mInitialized = PR_TRUE;

  return NS_OK;
}

inline PRBool
sbLibraryConstraintGroup::IsEmpty()
{
  return mConstraintGroup.Count() == 0;
}

nsresult
sbLibraryConstraintGroup::Add(const nsAString& aProperty,
                              sbStringArray* aArray)
{
  NS_ASSERTION(aArray, "sbStringArray is null");
  nsAutoPtr<sbStringArray> array(aArray);

  sbStringArray* existing;
  if (mConstraintGroup.Get(aProperty, &existing)) {
    nsString* success = existing->AppendElements(*array);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    // Let the nsAutoPtr delete the array since we've copied its contents
    // in to the hashtable
  }
  else {
    PRBool success = mConstraintGroup.Put(aProperty, array);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    // We've transfered ownership to the hashtable so forget it
    array.forget();
  }

  return NS_OK;
}

nsresult
sbLibraryConstraintGroup::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_STATE(!mInitialized);
  NS_ASSERTION(aStream, "aStream is null");

  nsresult rv;

  rv = Init();
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length;
  rv = aStream->Read32(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    nsString property;
    rv = aStream->ReadString(property);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 valueCount;
    rv = aStream->Read32(&valueCount);
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringArray* array = new sbStringArray;
    NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 j = 0; j < valueCount; j++) {
      nsString value;
      rv = aStream->ReadString(value);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString* added = array->AppendElement(value);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }

    PRBool success = mConstraintGroup.Put(property, array);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbLibraryConstraintGroup::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ASSERTION(aStream, "aStream is null");
  nsresult rv;

  nsAutoTArray<nsString, 10> array;
  mConstraintGroup.EnumerateRead(AddKeysToArrayCallback, &array);

  PRUint32 length = array.Length();
  rv = aStream->Write32(length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    rv = aStream->WriteWStringZ(array[i].BeginReading());
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringArray* values;
    PRBool success = mConstraintGroup.Get(array[i], &values);
    NS_ENSURE_SUCCESS(success, NS_ERROR_UNEXPECTED);

    PRUint32 valueCount = values->Length();
    rv = aStream->Write32(valueCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 j = 0; j < valueCount; j++) {
      rv = aStream->WriteWStringZ(values->ElementAt(j).BeginReading());
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

static NS_DEFINE_CID(kLibrarySortCID,
                     SONGBIRD_LIBRARYSORT_CID);

NS_IMPL_ISUPPORTS3(sbLibrarySort,
                   sbILibrarySort,
                   nsISerializable,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER3(sbLibrarySort,
                             sbILibrarySort,
                             nsISerializable,
                             nsIClassInfo)

sbLibrarySort::sbLibrarySort() :
  mInitialized(PR_FALSE),
  mIsAscending(PR_FALSE)
{
}

NS_IMETHODIMP
sbLibrarySort::Init(const nsAString& aProperty, PRBool aIsAscending)
{
  NS_ENSURE_STATE(!mInitialized);
  mProperty = aProperty;
  mIsAscending = aIsAscending;
  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetProperty(nsAString& aProperty)
{
  NS_ENSURE_STATE(mInitialized);
  aProperty = mProperty;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetIsAscending(PRBool* aIsAscending)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aIsAscending);

  *aIsAscending = mIsAscending;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::ToString(nsAString& aString)
{
  NS_ENSURE_STATE(mInitialized);

  nsString buff;
  buff.AssignLiteral("sort: property = '");
  buff.Append(mProperty);
  buff.AppendLiteral("' is ascending = ");
  buff.AppendLiteral(mIsAscending ? "yes" : "no");

  aString = buff;
  return NS_OK;
}

// nsISerializable
NS_IMETHODIMP
sbLibrarySort::Read(nsIObjectInputStream* aStream)
{
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->ReadString(mProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->ReadBoolean(&mIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::Write(nsIObjectOutputStream* aStream)
{
  NS_ENSURE_STATE(mInitialized);
  NS_ENSURE_ARG_POINTER(aStream);

  nsresult rv;

  rv = aStream->WriteWStringZ(mProperty.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->WriteBoolean(mIsAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLibrarySort::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLibrarySort)(count, array);
}

NS_IMETHODIMP
sbLibrarySort::GetHelperForLanguage(PRUint32 language,
                                    nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetFlags(PRUint32* aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLibrarySort::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  NS_ENSURE_ARG_POINTER(aClassIDNoAlloc);
  *aClassIDNoAlloc = kLibrarySortCID;
  return NS_OK;
}

