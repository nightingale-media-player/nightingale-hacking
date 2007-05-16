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

#include "sbLocalDatabaseSmartMediaList.h"
#include "sbLocalDatabaseCID.h"

#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbILocalDatabaseMediaItem.h>
#include <sbILocalDatabaseSimpleMediaList.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIPropertyManager.h>
#include <sbISQLBuilder.h>
#include <sbLocalDatabaseSchemaInfo.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbStandardOperators.h>
#include <sbStandardProperties.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsINetUtil.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsNetCID.h>
#include <nsServiceManagerUtils.h>
#include <nsVoidArray.h>
#include <prlog.h>

#define STATE_PROPERTY "http://songbirdnest.com/data/1.0#smartMediaListState"

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseSmartMediaList:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gLocalDatabaseSmartMediaListLog = nsnull;
#define TRACE(args) PR_LOG(gLocalDatabaseSmartMediaListLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabaseSmartMediaListLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

nsresult ParseQueryStringIntoHashtable(const nsAString& aString,
                                       sbStringMap& aMap)
{
  nsresult rv;

  // Parse a string that looks like name1=value1&name1=value2
  const PRUnichar *start, *end;
  PRUint32 length = aString.BeginReading(&start, &end);

  if (length == 0) {
    return NS_OK;
  }

  nsDependentSubstring chunk;

  const PRUnichar* chunkStart = start;
  static const PRUnichar sAmp = '&';
  for (const PRUnichar* current = start; current < end; current++) {

    if (*current == sAmp) {
      chunk.Rebind(chunkStart, current - chunkStart);

      rv = ParseAndAddChunk(chunk, aMap);
      NS_ENSURE_SUCCESS(rv, rv);
      if (current + 1 < end) {
        chunkStart = current + 1;
      }
      else {
        chunkStart = nsnull;
      }
    }
  }

  if (chunkStart) {
    chunk.Rebind(chunkStart, end - chunkStart);
    rv = ParseAndAddChunk(chunk, aMap);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
ParseAndAddChunk(const nsAString& aString,
                 sbStringMap& aMap)
{
  nsresult rv;
  nsCOMPtr<nsINetUtil> netUtil = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  static const PRUnichar sEquals = '=';

  const PRUnichar *start, *end;
  PRInt32 length = aString.BeginReading(&start, &end);

  if (length == 0) {
    return NS_OK;
  }

  PRInt32 pos = aString.FindChar(sEquals);
  if (pos > 1) {
    nsAutoString name(nsDependentSubstring(aString, 0, pos));
    nsCAutoString unescapedNameUtf8;
    rv = netUtil->UnescapeString(NS_ConvertUTF16toUTF8(name), unescapedNameUtf8);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value(nsDependentSubstring(aString, pos + 1, length - pos));
    nsCAutoString unescapedValueUtf8;
    if (pos < length - 1) {
      rv = netUtil->UnescapeString(NS_ConvertUTF16toUTF8(value), unescapedValueUtf8);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    PRBool success = aMap.Put(NS_ConvertUTF8toUTF16(unescapedNameUtf8),
                              NS_ConvertUTF8toUTF16(unescapedValueUtf8));
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

PLDHashOperator PR_CALLBACK
JoinStringMapCallback(nsStringHashKey::KeyType aKey,
                      nsString aEntry,
                      void* aUserData)
{
  NS_ASSERTION(aUserData, "Null user data");

  nsresult rv;
  nsCOMPtr<nsINetUtil> netUtil = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  nsString* str =
    NS_STATIC_CAST(nsString*, aUserData);
  NS_ENSURE_TRUE(str, PL_DHASH_STOP);

  nsCAutoString key;
  rv = netUtil->EscapeString(NS_ConvertUTF16toUTF8(aKey),
                             nsINetUtil::ESCAPE_XALPHAS,
                             key);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  nsCAutoString value;
  rv = netUtil->EscapeString(NS_ConvertUTF16toUTF8(aEntry),
                             nsINetUtil::ESCAPE_XALPHAS,
                             value);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  str->Append(NS_ConvertUTF8toUTF16(key));
  str->AppendLiteral("=");
  str->Append(NS_ConvertUTF8toUTF16(value));
  str->AppendLiteral("&");

  return PL_DHASH_NEXT;
}

nsresult
JoinStringMapIntoQueryString(sbStringMap& aMap,
                             nsAString &aString)
{

  nsAutoString str;
  aMap.EnumerateRead(JoinStringMapCallback, &str);

  if (str.Length() > 0) {
    aString = nsDependentSubstring(str, 0, str.Length() - 1);
  }
  else {
    aString = EmptyString();
  }

  return NS_OK;
}

//==============================================================================
// sbLocalDatabaseSmartMediaListCondition
//==============================================================================
NS_IMPL_ISUPPORTS1(sbLocalDatabaseSmartMediaListCondition,
                   sbILocalDatabaseSmartMediaListCondition)

sbLocalDatabaseSmartMediaListCondition::sbLocalDatabaseSmartMediaListCondition(const nsAString& aPropertyName,
                                                                               sbIPropertyOperator* aOperator,
                                                                               const nsAString& aLeftValue,
                                                                               const nsAString& aRightValue,
                                                                               PRBool aLimit)
: mLock(nsnull)
, mPropertyName(aPropertyName)
, mOperator(aOperator)
, mLeftValue(aLeftValue)
, mRightValue(aRightValue)
, mLimit(aLimit)
{
  mLock = PR_NewLock();
  NS_ASSERTION(mLock,
    "sbLocalDatabaseSmartMediaListCondition::mLock failed to create lock!");
}

sbLocalDatabaseSmartMediaListCondition::~sbLocalDatabaseSmartMediaListCondition()
{
  if(mLock) {
    PR_DestroyLock(mLock);
  }
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetPropertyName(nsAString& aPropertyName)
{
  nsAutoLock lock(mLock);
  aPropertyName = mPropertyName;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetOperator(sbIPropertyOperator** aOperator)
{
  NS_ENSURE_ARG_POINTER(aOperator);

  nsAutoLock lock(mLock);
  *aOperator = mOperator;
  NS_ADDREF(*aOperator);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetLeftValue(nsAString& aLeftValue)
{
  nsAutoLock lock(mLock);
  aLeftValue = mLeftValue;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetRightValue(nsAString& aRightValue)
{
  nsAutoLock lock(mLock);
  aRightValue = mRightValue;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetLimit(PRBool* aLimit)
{
  NS_ENSURE_ARG_POINTER(aLimit);

  nsAutoLock lock(mLock);
  *aLimit = mLimit;

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaListCondition::ToString(nsAString& _retval)
{
  nsAutoLock lock(mLock);

  nsresult rv;

  sbStringMap map;
  PRBool success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("property"), mPropertyName);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString op;
  rv = mOperator->GetOperator(op);
  NS_ENSURE_SUCCESS(rv, rv);

  success = map.Put(NS_LITERAL_STRING("operator"), op);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("leftValue"), mLeftValue);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("rightValue"), mRightValue);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString limit;
  limit.AppendInt(mLimit);
  success = map.Put(NS_LITERAL_STRING("limit"), limit);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  rv = JoinStringMapIntoQueryString(map, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//==============================================================================
// sbLocalDatabaseSmartMediaList
//==============================================================================


NS_IMPL_THREADSAFE_ISUPPORTS7(sbLocalDatabaseSmartMediaList,
                              nsIClassInfo,
                              nsISupportsWeakReference,
                              sbILibraryResource,
                              sbILocalDatabaseSmartMediaList,
                              sbIMediaItem,
                              sbIMediaList,
                              sbIMediaListListener);

NS_IMPL_CI_INTERFACE_GETTER7(sbLocalDatabaseSmartMediaList,
                             nsIClassInfo,
                             nsISupportsWeakReference,
                             sbILibraryResource,
                             sbILocalDatabaseSmartMediaList,
                             sbIMediaItem,
                             sbIMediaList,
                             sbIMediaListListener);

sbLocalDatabaseSmartMediaList::sbLocalDatabaseSmartMediaList()
: mInnerLock(nsnull)
, mConditionsLock(nsnull)
, mMatch(sbILocalDatabaseSmartMediaList::MATCH_ANY)
, mRandomSelection(PR_FALSE)
, mItemLimit(sbILocalDatabaseSmartMediaList::LIMIT_NONE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseSmartMediaListLog) {
    gLocalDatabaseSmartMediaListLog =
      PR_NewLogModule("sbLocalDatabaseSmartMediaList");
  }
#endif
}

sbLocalDatabaseSmartMediaList::~sbLocalDatabaseSmartMediaList()
{
  if(mInnerLock) {
    PR_DestroyLock(mInnerLock);
  }
  if(mConditionsLock) {
    PR_DestroyLock(mConditionsLock);
  }
}

nsresult
sbLocalDatabaseSmartMediaList::Init(sbIMediaItem *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  mInnerLock = PR_NewLock();
  NS_ENSURE_TRUE(mInnerLock, NS_ERROR_OUT_OF_MEMORY);

  mConditionsLock = PR_NewLock();
  NS_ENSURE_TRUE(mConditionsLock, NS_ERROR_OUT_OF_MEMORY);

  mItem = aItem;

  nsAutoString storageGuid;
  nsresult rv = mItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
                                   storageGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = mItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = library->GetMediaItem(storageGuid, getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);

  mList = do_QueryInterface(mediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register self for "before delete" so we can delete our storage list
  nsCOMPtr<sbIMediaList> libraryList = do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryList->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the property cache for id lookups
  mLocalDatabaseLibrary = do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mLocalDatabaseLibrary->GetPropertyCache(getter_AddRefs(mPropertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  // Read out saved state
  rv = ReadConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("smart"));
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetMatch(PRUint32* aMatch)
{
  NS_ENSURE_ARG_POINTER(aMatch);
  nsAutoLock lock(mConditionsLock);

  *aMatch = mMatch;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetMatch(PRUint32 aMatch)
{
  NS_ENSURE_ARG_RANGE(aMatch,
    sbILocalDatabaseSmartMediaList::MATCH_ANY,
    sbILocalDatabaseSmartMediaList::MATCH_ALL);

  nsAutoLock lock(mConditionsLock);
  mMatch = aMatch;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetConditionCount(PRUint32* aConditionCount)
{
  NS_ENSURE_ARG_POINTER(aConditionCount);

  nsAutoLock lock(mConditionsLock);
  *aConditionCount = mConditions.Length();

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetItemLimit(PRUint32* aItemLimit)
{
  NS_ENSURE_ARG_POINTER(aItemLimit);
  *aItemLimit = mItemLimit;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetItemLimit(PRUint32 aItemLimit)
{
  nsAutoLock lock(mConditionsLock);
  mItemLimit = aItemLimit;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetRandomSelection(PRBool* aRandomSelection)
{
  NS_ENSURE_ARG_POINTER(aRandomSelection);

  nsAutoLock lock(mConditionsLock);
  *aRandomSelection = mRandomSelection;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetRandomSelection(PRBool aRandomSelection)
{
  nsAutoLock lock(mConditionsLock);
  mRandomSelection = aRandomSelection;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::AppendCondition(const nsAString& aPropertyName,
                                               sbIPropertyOperator* aOperator,
                                               const nsAString& aLeftValue,
                                               const nsAString& aRightValue,
                                               PRBool aLimit,
                                               sbILocalDatabaseSmartMediaListCondition** _retval)
{
  NS_ENSURE_ARG_POINTER(aOperator);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG(aPropertyName.Length() > 1);

  // Make sure the right value is void if the operator is anything else but
  // between
  nsAutoString op;
  nsresult rv = aOperator->GetOperator(op);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXsteve IsEmpty should really be IsVoid
  if (!op.EqualsLiteral(SB_OPERATOR_BETWEEN) && !aRightValue.IsEmpty()) {
    return NS_ERROR_ILLEGAL_VALUE;
  };

  sbRefPtrCondition condition;
  condition = new sbLocalDatabaseSmartMediaListCondition(aPropertyName,
                                                         aOperator,
                                                         aLeftValue,
                                                         aRightValue,
                                                         aLimit);
  NS_ENSURE_TRUE(condition, NS_ERROR_OUT_OF_MEMORY);

  sbRefPtrCondition* success = mConditions.AppendElement(condition);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = condition);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::RemoveConditionAt(PRUint32 aConditionIndex)
{
  nsAutoLock lock(mConditionsLock);
  PRUint32 count = mConditions.Length();

  NS_ENSURE_ARG(aConditionIndex < count);
  mConditions.RemoveElementAt(aConditionIndex);

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetConditionAt(PRUint32 aConditionIndex,
                                              sbILocalDatabaseSmartMediaListCondition** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoLock lock(mConditionsLock);
  PRUint32 count = mConditions.Length();

  NS_ENSURE_ARG(aConditionIndex < count);

  *_retval = mConditions[aConditionIndex];
  NS_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::ClearConditions()
{
  nsAutoLock lock(mConditionsLock);

  mConditions.Clear();

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::Rebuild()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - Rebuild()", this));

  nsAutoLock lock(mConditionsLock);
  PRUint32 count = mConditions.Length();

  if(!count) {
    return NS_OK;
  }

  // Prepare the sequence of queries needed to update this smart media list
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = mLocalDatabaseLibrary->CreateQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // Drop and create the temporary table that we are going to select into
  rv = query->AddQuery(NS_LITERAL_STRING("drop table if exists temp_smart"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->AddQuery(NS_LITERAL_STRING(
    "create table temp_smart (media_item_id integer, count integer primary key autoincrement)"));
  NS_ENSURE_SUCCESS(rv, rv);

  // The contents of the smart media list will be inserted using a single
  // insert statement with a compound select statemet made up of select
  // statements for each condition.
  nsAutoString insert;

  // The sql builder does not know how to create compound select statements so
  // we'll do a little string appending here
  insert.AssignLiteral("insert into temp_smart (media_item_id) ");

  for (PRUint32 i = 0; i < count; i++) {
    nsAutoString sql;
    rv = CreateSQLForCondition(mConditions[i], sql);
    NS_ENSURE_SUCCESS(rv, rv);

    insert.Append(sql);
    if (i + 1 < count) {
      // Join the select statements with "intersect" if we are doing an a
      // match, otherwise use "union"
      if (mMatch == sbILocalDatabaseSmartMediaList::MATCH_ALL) {
        insert.AppendLiteral(" intersect ");
      }
      else {
        insert.AppendLiteral(" union ");
      }
    }
  }

  rv = query->AddQuery(insert);
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete the contents of our inner simple media list
  rv = query->AddQuery(mClearListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - Rebuild() - AddQuery %s",
         this, NS_ConvertUTF16toUTF8(mClearListQuery).get()));

  // Copy the contents of the temp list into the smart media list with the
  // user supplied limit
  if (mItemLimit == sbILocalDatabaseSmartMediaList::LIMIT_NONE) {
    rv = query->AddQuery(mCopyToListQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - Rebuild() - AddQuery %s",
           this, NS_ConvertUTF16toUTF8(mCopyToListQuery).get()));
  }
  else {
    nsAutoString copyToListQuery(mCopyToListQuery);
    copyToListQuery.AppendLiteral(" limit ");
    copyToListQuery.AppendInt(mItemLimit);

    rv = query->AddQuery(copyToListQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - Rebuild() - AddQuery %s",
           this, NS_ConvertUTF16toUTF8(copyToListQuery).get()));
  }

  // Execute the query
  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbILocalDatabaseSimpleMediaList> ldsml =
    do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ldsml->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::CreateSQLForCondition(sbRefPtrCondition& aCondition,
                                                     nsAString& _retval)
{
  nsresult rv;

  // Get the property info for this property.  If the property info is not
  // found, return not available
  nsCOMPtr<sbIPropertyInfo> info;
  rv = mPropMan->GetPropertyInfo(aCondition->mPropertyName,
                                 getter_AddRefs(info));
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    else {
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isTopLevelProperty = SB_IsTopLevelProperty(aCondition->mPropertyName);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (isTopLevelProperty) {
    rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_c"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddColumn(NS_LITERAL_STRING("_c"),
                             NS_LITERAL_STRING("media_item_id"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddCriterionForCondition(builder, aCondition, info);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_mi"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddColumn(NS_LITERAL_STRING("_mi"),
                             NS_LITERAL_STRING("media_item_id"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddJoin(sbISQLBuilder::JOIN_INNER,
                          NS_LITERAL_STRING("resource_properties"),
                          NS_LITERAL_STRING("_c"),
                          NS_LITERAL_STRING("guid"),
                          NS_LITERAL_STRING("_mi"),
                          NS_LITERAL_STRING("guid"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddCriterionForCondition(builder, aCondition, info);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = builder->ToString(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - CreateSQLForCondition() - %s",
         this, NS_LossyConvertUTF16toASCII(_retval).get()));

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::AddCriterionForCondition(sbISQLSelectBuilder* aBuilder,
                                                        sbRefPtrCondition& aCondition,
                                                        sbIPropertyInfo* aInfo)
{
  NS_ENSURE_ARG_POINTER(aBuilder);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv;

  // Get some stuff about the property
  PRBool isTopLevelProperty = SB_IsTopLevelProperty(aCondition->mPropertyName);
  PRUint32 propertyId;
  nsAutoString columnName;
  nsCOMPtr<sbISQLBuilderCriterion> propertyCriterion;

  if (isTopLevelProperty) {
    rv = SB_GetTopLevelPropertyColumn(aCondition->mPropertyName, columnName);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    columnName.AssignLiteral("obj_sortable");
    rv = mPropertyCache->GetPropertyID(aCondition->mPropertyName, &propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBuilder->CreateMatchCriterionLong(NS_LITERAL_STRING("_c"),
                                            NS_LITERAL_STRING("property_id"),
                                            sbISQLBuilder::MATCH_EQUALS,
                                            propertyId,
                                            getter_AddRefs(propertyCriterion));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoString value;
  rv = aInfo->MakeSortable(aCondition->mLeftValue, value);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString op;
  rv = aCondition->mOperator->GetOperator(op);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is a between operator, construct two conditions for it
  if (op.EqualsLiteral(SB_OPERATOR_BETWEEN)) {
    nsCOMPtr<sbISQLBuilderCriterion> left;
    rv = aBuilder->CreateMatchCriterionString(NS_LITERAL_STRING("_c"),
                                              columnName,
                                              sbISQLBuilder::MATCH_GREATEREQUAL,
                                              value,
                                              getter_AddRefs(left));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString rightValue;
    rv = aInfo->MakeSortable(aCondition->mRightValue, rightValue);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> right;
    rv = aBuilder->CreateMatchCriterionString(NS_LITERAL_STRING("_c"),
                                              columnName,
                                              sbISQLBuilder::MATCH_LESSEQUAL,
                                              rightValue,
                                              getter_AddRefs(right));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = aBuilder->CreateAndCriterion(left, right, getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    // If this isn't a top level propery, constraint by property
    if (!isTopLevelProperty) {
      nsCOMPtr<sbISQLBuilderCriterion> temp;
      rv = aBuilder->CreateAndCriterion(propertyCriterion,
                                        criterion,
                                        getter_AddRefs(temp));
      NS_ENSURE_SUCCESS(rv, rv);
      criterion = temp;
    }

    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // Convert the property operator to a sql builder match condition
  PRInt32 matchType = -1;
  if (op.EqualsLiteral(SB_OPERATOR_EQUALS)) {
    matchType = sbISQLBuilder::MATCH_EQUALS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_NOTEQUALS)) {
    matchType = sbISQLBuilder::MATCH_NOTEQUALS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_GREATER)) {
    matchType = sbISQLBuilder::MATCH_GREATER;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_GREATEREQUAL)) {
    matchType = sbISQLBuilder::MATCH_GREATEREQUAL;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_LESS)) {
    matchType = sbISQLBuilder::MATCH_LESS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_LESSEQUAL)) {
    matchType = sbISQLBuilder::MATCH_LESSEQUAL;
  }

  if (matchType >= 0) {
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = aBuilder->CreateMatchCriterionString(NS_LITERAL_STRING("_c"),
                                              columnName,
                                              matchType,
                                              value,
                                              getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!isTopLevelProperty) {
      nsCOMPtr<sbISQLBuilderCriterion> temp;
      rv = aBuilder->CreateAndCriterion(propertyCriterion,
                                        criterion,
                                        getter_AddRefs(temp));
      NS_ENSURE_SUCCESS(rv, rv);
      criterion = temp;
    }

    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // If were are any of the contains style operators, handle it here
  if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
      op.EqualsLiteral(SB_OPERATOR_ENDSWITH) ||
      op.EqualsLiteral(SB_OPERATOR_BEGINSWITH)) {

    nsAutoString like;
    if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_ENDSWITH)) {
      like.AppendLiteral("%");
    }

    like.Append(value);

    if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_BEGINSWITH)) {
      like.AppendLiteral("%");
    }

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = aBuilder->CreateMatchCriterionString(NS_LITERAL_STRING("_c"),
                                              columnName,
                                              sbISQLBuilder::MATCH_LIKE,
                                              like,
                                              getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!isTopLevelProperty) {
      nsCOMPtr<sbISQLBuilderCriterion> temp;
      rv = aBuilder->CreateAndCriterion(propertyCriterion,
                                        criterion,
                                        getter_AddRefs(temp));
      NS_ENSURE_SUCCESS(rv, rv);
      criterion = temp;
    }

    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // If we get here, we don't know how to handle the supplied operator
  return NS_ERROR_UNEXPECTED;
}

nsresult
sbLocalDatabaseSmartMediaList::CreateQueries()
{
  nsresult rv;

  nsCOMPtr<sbILocalDatabaseMediaItem> ldmi = do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLDeleteBuilder> deleteb = 
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mClearListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("member_media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("ordinal"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLSelectBuilder> select =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);

  rv = select->SetBaseTableName(NS_LITERAL_STRING("temp_smart"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString mediaItemIdStr;
  mediaItemIdStr.AppendInt(mediaItemId);
  rv = select->AddColumn(EmptyString(), mediaItemIdStr);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = select->AddColumn(EmptyString(), NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = select->AddColumn(EmptyString(), NS_LITERAL_STRING("count"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->SetSelect(select);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->ToString(mCopyToListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::ReadConfiguration()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - ReadConfiguration()", this));

  nsresult rv;

  nsAutoLock lock(mConditionsLock);

  sbStringMap map;
  PRBool success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Set up the defaults
  mMatch = sbILocalDatabaseSmartMediaList::MATCH_ANY;
  mItemLimit = sbILocalDatabaseSmartMediaList::LIMIT_NONE;
  mRandomSelection = PR_FALSE;
  mConditions.Clear();

  // If no saved state is available, just return
  nsAutoString state;
  rv = mItem->GetProperty(NS_LITERAL_STRING(STATE_PROPERTY), state);
  if (NS_FAILED(rv)) {
    if (rv == NS_ERROR_ILLEGAL_VALUE)
      return NS_OK;
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Parse the list's properties from the state
  rv = ParseQueryStringIntoHashtable(state, map);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString value;
  nsresult dontCare;

  if (map.Get(NS_LITERAL_STRING("match"), &value)) {
    mMatch = value.ToInteger(&dontCare);
  }

  if (map.Get(NS_LITERAL_STRING("itemLimit"), &value)) {
    mItemLimit = value.ToInteger(&dontCare);
  }

  if (map.Get(NS_LITERAL_STRING("randomSelection"), &value)) {
    mRandomSelection = value.EqualsLiteral("1");
  }

  if (map.Get(NS_LITERAL_STRING("conditionCount"), &value)) {
    PRUint32 count = value.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringMap conditionMap;
    success = conditionMap.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = 0; i < count; i++) {
      nsAutoString key;
      key.AssignLiteral("condition");
      key.AppendInt(i);

      if (map.Get(key, &value)) {
        conditionMap.Clear();
        rv = ParseQueryStringIntoHashtable(value, conditionMap);
        if (NS_FAILED(rv)) {
          NS_WARNING("Could not parse condition state");
          continue;
        }

        // Extract each property for this condition and set them to local
        // variables.
        nsAutoString property;
        nsAutoString leftValue;
        nsAutoString rightValue;
        nsAutoString opString;
        PRUint32 limit = sbILocalDatabaseSmartMediaList::LIMIT_NONE;

        if (conditionMap.Get(NS_LITERAL_STRING("property"), &value)) {
          property = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("leftValue"), &value)) {
          leftValue = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("rightValue"), &value)) {
          rightValue = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("operator"), &value)) {
          opString = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("limit"), &value)) {
          limit = value.ToInteger(&dontCare);
        }

        if (!property.IsEmpty() && !opString.IsEmpty()) {

          // Get the property info for this property.  If the property does
          // not exist, just skip this condition
          nsCOMPtr<sbIPropertyInfo> info;
          rv = mPropMan->GetPropertyInfo(property, getter_AddRefs(info));
          if (NS_FAILED(rv)) {
            if (rv == NS_ERROR_NOT_AVAILABLE) {
              NS_WARNING("Stored property not found");
              continue;
            }
            else {
              NS_ENSURE_SUCCESS(rv, rv);
            }
          }

          // Get the operator.  If this operator is not found, then skip this
          // condition
          nsCOMPtr<sbIPropertyOperator> op;
          rv = info->GetOperator(opString, getter_AddRefs(op));
          NS_ENSURE_SUCCESS(rv, rv);
          if (!op) {
            NS_WARNING("Stored operator not found");
            continue;
          }

          sbRefPtrCondition condition;
          condition = new sbLocalDatabaseSmartMediaListCondition(property,
                                                                 op,
                                                                 leftValue,
                                                                 rightValue,
                                                                 limit);
          NS_ENSURE_TRUE(condition, NS_ERROR_OUT_OF_MEMORY);

          sbRefPtrCondition* victory = mConditions.AppendElement(condition);
          NS_ENSURE_TRUE(victory, NS_ERROR_OUT_OF_MEMORY);
        }
        else {
          NS_WARNING("Can't restore condition, blank property or operator");
        }
      }
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::WriteConfiguration()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - WriteConfiguration()", this));

  nsresult rv;

  PRUint32 count = mConditions.Length();

  sbStringMap map;
  PRBool success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString match;
  match.AppendInt(mMatch);
  success = map.Put(NS_LITERAL_STRING("match"), match);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString itemLimit;
  itemLimit.AppendInt(mItemLimit);
  success = map.Put(NS_LITERAL_STRING("itemLimit"), itemLimit);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString randomSelection;
  randomSelection.AppendLiteral(mRandomSelection ? "1" : "0");
  success = map.Put(NS_LITERAL_STRING("randomSelection"), randomSelection);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString conditionCount;
  conditionCount.AppendInt(count);
  success = map.Put(NS_LITERAL_STRING("conditionCount"), conditionCount);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < count; i++) {
    nsAutoString key;
    key.AssignLiteral("condition");
    key.AppendInt(i);

    nsAutoString value;
    rv = mConditions[i]->ToString(value);
    NS_ENSURE_SUCCESS(rv, rv);

    success = map.Put(key, value);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  nsAutoString state;
  rv = JoinStringMapIntoQueryString(map, state);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mItem->SetProperty(NS_LITERAL_STRING(STATE_PROPERTY), state);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mItem->Write();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbIMediaListListener
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnItemAdded(sbIMediaList* aMediaList,
                                           sbIMediaItem* aMediaItem)
{
  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                   sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Is this notification coming from our library?
  nsCOMPtr<sbILibrary> library;
  nsresult rv = GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isOurLibrary;
  rv = aMediaList->Equals(library, &isOurLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  // Is this notification about this smart media list?
  PRBool isThis;
  rv = aMediaItem->Equals(mItem, &isThis);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this smart media list is being removed from the library, remove the
  // data list
  if (isThis && isOurLibrary) {

    nsCOMPtr<sbIMediaList> list = do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = list->Remove(mList);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                  sbIMediaItem* aMediaItem)
{
  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnItemUpdated(sbIMediaList* aMediaList,
                                             sbIMediaItem* aMediaItem)
{
  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnListCleared(sbIMediaList* aMediaList)
{
  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBatchBegin(sbIMediaList* aMediaList)
{
  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBatchEnd(sbIMediaList* aMediaList)
{
  // Don't care
  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseSmartMediaList)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetHelperForLanguage(PRUint32 language,
                                                    nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::THREADSAFE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

