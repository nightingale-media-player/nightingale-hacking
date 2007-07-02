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
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISQLBuilder.h>
#include <sbLocalDatabaseSchemaInfo.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbStandardOperators.h>
#include <sbStandardProperties.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsINetUtil.h>
#include <nsIUUIDGenerator.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsNetCID.h>
#include <nsServiceManagerUtils.h>
#include <nsVoidArray.h>
#include <prlog.h>
#include <prprf.h>

#define STATE_PROPERTY "http://songbirdnest.com/data/1.0#smartMediaListState"
#define RANDOM_ADD_CHUNK_SIZE 1000;
#define SQL_IN_LIMIT 1000

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
    rv = netUtil->UnescapeString(NS_ConvertUTF16toUTF8(name), 0, unescapedNameUtf8);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString value(nsDependentSubstring(aString, pos + 1, length - pos));
    nsCAutoString unescapedValueUtf8;
    if (pos < length - 1) {
      rv = netUtil->UnescapeString(NS_ConvertUTF16toUTF8(value), 0, unescapedValueUtf8);
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
    static_cast<nsString*>(aUserData);
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
, mMatchType(sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY)
, mLimitType(sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE)
, mLimit(0)
, mSelectDirection(PR_TRUE)
, mRandomSelection(PR_FALSE)
, mLiveUpdate(PR_FALSE)
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

  // Use a weak reference here because we already have a strong reference to
  // the inner list
  rv = libraryList->AddListener(this, PR_TRUE);
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
sbLocalDatabaseSmartMediaList::GetMatchType(PRUint32* aMatchType)
{
  NS_ENSURE_ARG_POINTER(aMatchType);
  nsAutoLock lock(mConditionsLock);

  *aMatchType = mMatchType;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetMatchType(PRUint32 aMatchType)
{
  NS_ENSURE_ARG_RANGE(aMatchType,
    sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY,
    sbILocalDatabaseSmartMediaList::MATCH_TYPE_NONE);

  nsAutoLock lock(mConditionsLock);
  mMatchType = aMatchType;

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
sbLocalDatabaseSmartMediaList::GetLimitType(PRUint32* aLimitType)
{
  NS_ENSURE_ARG_POINTER(aLimitType);
  *aLimitType = mLimitType;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetLimitType(PRUint32 aLimitType)
{
  NS_ENSURE_ARG_RANGE(aLimitType,
    sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE,
    sbILocalDatabaseSmartMediaList::LIMIT_TYPE_BYTES);

  nsAutoLock lock(mConditionsLock);
  mLimitType = aLimitType;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetLimit(PRUint64* aLimit)
{
  NS_ENSURE_ARG_POINTER(aLimit);
  *aLimit = mLimit;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetLimit(PRUint64 aLimit)
{
  nsAutoLock lock(mConditionsLock);
  mLimit = aLimit;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetSelectPropertyName(nsAString& aSelectPropertyName)
{
  aSelectPropertyName = mSelectPropertyName;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetSelectPropertyName(const nsAString& aSelectPropertyName)
{
  nsAutoLock lock(mConditionsLock);
  mSelectPropertyName = aSelectPropertyName;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetSelectDirection(PRBool* aSelectDirection)
{
  NS_ENSURE_ARG_POINTER(aSelectDirection);

  nsAutoLock lock(mConditionsLock);
  *aSelectDirection = mSelectDirection;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetSelectDirection(PRBool aSelectDirection)
{
  nsAutoLock lock(mConditionsLock);
  mSelectDirection = aSelectDirection;

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
sbLocalDatabaseSmartMediaList::GetLiveUpdate(PRBool* aLiveUpdate)
{
  NS_ENSURE_ARG_POINTER(aLiveUpdate);

  nsAutoLock lock(mConditionsLock);
  *aLiveUpdate = mLiveUpdate;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetLiveUpdate(PRBool aLiveUpdate)
{
  nsAutoLock lock(mConditionsLock);
  mLiveUpdate = aLiveUpdate;

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

  nsresult rv;

  // You must either have a match or a limit
  NS_ENSURE_ARG(!(mMatchType == sbILocalDatabaseSmartMediaList::MATCH_TYPE_NONE && 
                  mLimitType == sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE));

  // If we have a limit, either random or the selection property must be set
  if (mLimitType != sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE) {
    if (!(mRandomSelection || !mSelectPropertyName.IsEmpty()))
      return NS_ERROR_INVALID_ARG;
  }

  if (mMatchType == sbILocalDatabaseSmartMediaList::MATCH_TYPE_NONE) {
    if (mRandomSelection) {
      rv = RebuildMatchTypeNoneRandom();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = RebuildMatchTypeNoneNotRandom();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    rv = RebuildMatchTypeAnyAll();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Invalidate our data simple media list
  nsCOMPtr<sbILocalDatabaseSimpleMediaList> ldsml =
    do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ldsml->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::RebuildMatchTypeAnyAll()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - RebuildMatchTypeAnyAll()",
         this));

  nsresult rv;

  // For an any/all smart media list, execute one query per condition and
  // stick the result into a temp table.  Then determine the row limit (if any)
  // and copy the data into the data list
  nsAutoString tempTableName;
  rv = CreateTempTable(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  // The contents of the smart media list will be inserted using a single
  // insert statement with a compound select statemet made up of select
  // statements for each condition.

  // The sql builder does not know how to create compound select statements so
  // we'll do a little string appending here
  nsAutoString sql;
  sql.AssignLiteral("insert into ");
  sql.Append(tempTableName);
  sql.AppendLiteral(" (media_item_id, limitby, selectby) ");

  PRUint32 count = mConditions.Length();
  for (PRUint32 i = 0; i < count; i++) {
    nsAutoString conditionSql;
    rv = CreateSQLForCondition(mConditions[i], conditionSql);
    NS_ENSURE_SUCCESS(rv, rv);

    sql.Append(conditionSql);
    if (i + 1 < count) {
      // Join the select statements with "intersect" if we are doing an a
      // match, otherwise use "union"
      if (mMatchType == sbILocalDatabaseSmartMediaList::MATCH_TYPE_ALL) {
        sql.AppendLiteral(" intersect ");
      }
      else {
        sql.AppendLiteral(" union ");
      }
    }
  }

  rv = ExecuteQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is a random selection query, set the selectby column to random
  // values
  if (mRandomSelection) {
    nsAutoString updateRandom;
    updateRandom.AppendLiteral("update ");
    updateRandom.Append(tempTableName);
    updateRandom.AppendLiteral(" set selectby = random()");
    rv = ExecuteQuery(updateRandom);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Delete the contents of our inner simple media list
  rv = ExecuteQuery(mClearListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString copy;
  rv = GetCopyToListQuery(tempTableName, copy);
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is a limit query, figure out the row limit and add it to the
  // copy query
  if (mLimitType != sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE) {

    // If this is not an item limit, figure out our row limit should be
    PRUint32 rowLimit;
    if (mLimitType == sbILocalDatabaseSmartMediaList::LIMIT_TYPE_ITEMS) {
      rowLimit = mLimit;
    }
    else {
      nsAutoString rollingSql;
      rollingSql.AssignLiteral("select limitby from ");
      rollingSql.Append(tempTableName);
      rollingSql.AppendLiteral(" order by selectby ");
      rollingSql.AppendLiteral(mSelectDirection ? "asc" : "desc");
      rv = GetRollingLimit(rollingSql, 0, &rowLimit);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // If the rolling limit was 0, that means we can select everything and
    // not go over the limit
    if (rowLimit > 0) {
      copy.AppendLiteral(" order by selectby ");
      copy.AppendLiteral(mSelectDirection ? "asc" : "desc");
      copy.AppendLiteral(" limit ");
      copy.AppendInt(rowLimit);
    }

  }

  rv = ExecuteQuery(copy);
  NS_ENSURE_SUCCESS(rv, rv);

  // Drop the temp table
  rv = DropTempTable(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::RebuildMatchTypeNoneNotRandom()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - "
         "RebuildMatchTypeNoneNotRandom()", this));    

  // Must have some kind of limit and be non-random and have a select property
  // to get here
  NS_ENSURE_STATE(mLimitType != sbLocalDatabaseSmartMediaList::LIMIT_TYPE_NONE);
  NS_ENSURE_STATE(!mRandomSelection);
  NS_ENSURE_STATE(!mSelectPropertyName.IsEmpty());

  NS_NAMED_LITERAL_STRING(kMediaItems,      "media_items");
  NS_NAMED_LITERAL_STRING(kMediaItemId,     "media_item_id");
  NS_NAMED_LITERAL_STRING(kMediaItemsAlias, "_mi");
  NS_NAMED_LITERAL_STRING(kLimitAlias,      "_limit");

  nsresult rv;

  // For match type none, the strategy is to do a rolling limit query on a
  // full library query (sorted on the select by attribute) to get the row
  // number to select up to.  Then do a select directly into the data media
  // list

  PRUint32 limitRow;

  // If we don't have an item limit type, use a rolling query to find the
  // limit row
  if (mLimitType != sbLocalDatabaseSmartMediaList::LIMIT_TYPE_ITEMS) {
    nsCOMPtr<sbISQLSelectBuilder> builder =
      do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->SetBaseTableName(kMediaItems);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->SetBaseTableAlias(kMediaItemsAlias);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddSelectColumnAndJoin(builder, kMediaItemsAlias, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddLimitColumnAndJoin(builder, kMediaItemsAlias);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString sql;
    rv = builder->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetRollingLimit(sql, 1, &limitRow);
    NS_ENSURE_SUCCESS(rv, rv);

  }
  else {
    // Otherwise, the limit row is the limit
    limitRow = mLimit;
  }

  nsAutoString tempTableName;
  rv = CreateTempTable(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Select the media item ids from the library into the temp table.  We do
  // this so the rows can get numbered
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(kMediaItems);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableAlias(kMediaItemsAlias);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(kMediaItemsAlias, kMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("'0'"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddSelectColumnAndJoin(builder, kMediaItemsAlias, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // If we found a row limit, set it here
  if (limitRow > 0) {
    rv = builder->SetLimit(limitRow);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoString sql;
  rv = builder->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString insert;
  insert.AssignLiteral("insert into ");
  insert.Append(tempTableName);
  insert.AppendLiteral(" (media_item_id, limitby, selectby) ");
  insert.Append(sql);

  rv = ExecuteQuery(insert);
  NS_ENSURE_SUCCESS(rv, rv);

  // Finaly, clear the data simple media list and insert the result
  rv = ExecuteQuery(mClearListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString copy;
  rv = GetCopyToListQuery(tempTableName, copy);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ExecuteQuery(copy);
  NS_ENSURE_SUCCESS(rv, rv);

  // Drop the temp table
  rv = DropTempTable(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::RebuildMatchTypeNoneRandom()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - "
         "RebuildMatchTypeNoneRandom()", this));

  // Must have some kind of limit and be random and have no select property
  // to get here
  NS_ENSURE_STATE(mLimitType != sbLocalDatabaseSmartMediaList::LIMIT_TYPE_NONE);
  NS_ENSURE_STATE(mRandomSelection);
  NS_ENSURE_STATE(mSelectPropertyName.IsEmpty());

  NS_NAMED_LITERAL_STRING(kMediaItemId, "media_item_id");
  NS_NAMED_LITERAL_STRING(kLimitBy,     "limitby");

  nsresult rv;

  // This is the strangest situation of the bunch.  We need to do a random
  // selection over the entire library and keep going until the limit condition
  // is met.  To do this, we'll create an array representing all of the media
  // item ids in the database, then shuffle it.  Next, we'll add chunks of
  // the shuffled media item ids to the temp table until we reach our limit

  // First, get the min and max media_item_ids from the database
  PRUint32 min;
  PRUint32 max;
  rv = GetMediaItemIdRange(&min, &max);
  if (NS_FAILED(rv)) {
    // If we get NS_ERROR_UNEXPECTED, this means the library is empty.
    // Nothing to do so just return
    if (rv == NS_ERROR_UNEXPECTED) {
      return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // XXXsteve Perhaps for some small item limits we could just generate a
  // handful of non repeating ids rather than go through all of this shuffling
  // stuff.  This might be useful if a common case is to make a smart playlist
  // with a small number of random items on it (perhaps to feed a party shuffle
  // type thing)

  // Create an array with max - min elements and fill it with the concecutive
  // numbers between the twe
  PRUint32 length = max - min + 1;
  sbMediaItemIdArray mediaItemIds(length);

  for (PRUint32 i = 0; i < length; i++) {
    PRUint32* success = mediaItemIds.AppendElement(min + i);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Shuffle the items in the array
  ShuffleArray(mediaItemIds);

  // Make our temp table
  nsAutoString tempTableName;
  rv = CreateTempTable(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the media items into the temp table in batches of the shuffled order.
  // After each add, check the rolling sum to see if we've reached paydirt
  // Note that we can't optmize this in the case of the item limit since we
  // don't really now for sure how many rows we will actually add in each
  // batch since the real media item ids in the database may be non concecutive

  // Prepare our rolling sum query
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), kLimitBy);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString rollingQuery;
  rv = builder->ToString(rollingQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXsteve There is probably an optmization here were we could get the
  // rolling sum of only the records that were just added.  However, this is
  // hard to do with the current database interface because we can't give
  // it a value to start from when counting the rolling sum.

  PRUint32 rowLimit = 0;
  PRUint32 pos = 0;
  while (pos < length) {
    PRUint32 chunkSize = RANDOM_ADD_CHUNK_SIZE;
    if (pos + chunkSize > length) {
      chunkSize = length - pos;
    }

    rv = AddMediaItemsTempTable(tempTableName, mediaItemIds, pos, chunkSize);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mLimitType == sbLocalDatabaseSmartMediaList::LIMIT_TYPE_ITEMS) {
      PRUint32 rowCount;
      rv = GetRowCount(tempTableName, &rowCount);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rowCount >= mLimit) {
        rowLimit = mLimit;
        break;
      }
    }
    else {
      PRUint32 rollingRow;
      rv = GetRollingLimit(rollingQuery, 0, &rollingRow);
      NS_ENSURE_SUCCESS(rv, rv);

      if (rollingRow > 0) {
        rowLimit = rollingRow;
        break;
      }
    }
    pos += chunkSize;
  }

  // Finally clear out the old list and copy the items out of the temp table to
  // the data simple media list
  rv = ExecuteQuery(mClearListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString copy;
  rv = GetCopyToListQuery(tempTableName, copy);
  NS_ENSURE_SUCCESS(rv, rv);

  // If we found a limit, apply it here.  Otherwise we just copy everything.
  if (rowLimit > 0) {
    copy.AppendLiteral(" limit ");
    copy.AppendInt(rowLimit);
  }

  rv = ExecuteQuery(copy);
  NS_ENSURE_SUCCESS(rv, rv);

  // Drop the temp table
  rv = DropTempTable(tempTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::AddMediaItemsTempTable(const nsAutoString& tempTableName,
                                                      sbMediaItemIdArray& aArray,
                                                      PRUint32 aStart,
                                                      PRUint32 aLength)
{
  NS_NAMED_LITERAL_STRING(kMediaItems,      "media_items");
  NS_NAMED_LITERAL_STRING(kMediaItemId,     "media_item_id");
  NS_NAMED_LITERAL_STRING(kMediaItemsAlias, "_mi");
  NS_NAMED_LITERAL_STRING(kMediaListTypeId, "media_list_type_id");

  nsresult rv;

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(kMediaItems);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableAlias(kMediaItemsAlias);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(kMediaItemsAlias, kMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddLimitColumnAndJoin(builder, kMediaItemsAlias);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("''"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Only get media items
  nsCOMPtr<sbISQLBuilderCriterion> nullCriterion;
  rv = builder->CreateMatchCriterionNull(kMediaItemsAlias,
                                         kMediaListTypeId,
                                         sbISQLBuilder::MATCH_EQUALS,
                                         getter_AddRefs(nullCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(nullCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterionIn> criterion;
  rv = builder->CreateMatchCriterionIn(kMediaItemsAlias,
                                       kMediaItemId,
                                       getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString insertStart;
  insertStart.AssignLiteral("insert into ");
  insertStart.Append(tempTableName);
  insertStart.AppendLiteral(" (media_item_id, limitby, selectby) ");

  PRUint32 inNum = 0;
  for (PRUint32 i = 0; i < aLength; i++) {

    rv = criterion->AddLong(aArray[aStart + i]);
    NS_ENSURE_SUCCESS(rv, rv);
    inNum++;

    if (inNum > SQL_IN_LIMIT || i + 1 == aLength) {

      nsAutoString sql;
      rv = builder->ToString(sql);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString insert(insertStart);
      insert.Append(sql);

      rv = ExecuteQuery(insert);
      NS_ENSURE_SUCCESS(rv, rv);

      inNum = 0;
      rv = criterion->Clear();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::GetRollingLimit(const nsAString& aSql,
                                               PRUint32 aRollingLimitColumnIndex,
                                               PRUint32* aRow)
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - GetRollingLimit()", this));

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = mLocalDatabaseLibrary->CreateQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetRollingLimit(mLimit);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetRollingLimitColumnIndex(aRollingLimitColumnIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(aSql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  rv = query->GetRollingLimitResult(aRow);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - GetRollingLimit() - %s = %d",
         this, NS_ConvertUTF16toUTF8(aSql).get(), *aRow));

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::CreateSQLForCondition(sbRefPtrCondition& aCondition,
                                                     nsAString& _retval)
{
  nsresult rv;

  NS_NAMED_LITERAL_STRING(kConditionAlias,     "_c");
  NS_NAMED_LITERAL_STRING(kGuid,               "guid");
  NS_NAMED_LITERAL_STRING(kMediaItemId,        "media_item_id");
  NS_NAMED_LITERAL_STRING(kMediaItems,         "media_items");
  NS_NAMED_LITERAL_STRING(kMediaItemsAlias,    "_mi");
  NS_NAMED_LITERAL_STRING(kMediaListTypeId,    "media_list_type_id");
  NS_NAMED_LITERAL_STRING(kObjSortable,        "obj_sortable");
  NS_NAMED_LITERAL_STRING(kPropertyId,         "property_id");
  NS_NAMED_LITERAL_STRING(kResourceProperties, "resource_properties");

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

  rv = builder->SetBaseTableName(kMediaItems);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString baseAlias;

  // Add condition query
  if (isTopLevelProperty) {
    baseAlias.Assign(kConditionAlias);
    rv = builder->SetBaseTableAlias(baseAlias);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddColumn(baseAlias, kMediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddCriterionForCondition(builder, aCondition, info);
    NS_ENSURE_SUCCESS(rv, rv);

    // Only show media items
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = builder->CreateMatchCriterionNull(baseAlias,
                                           kMediaListTypeId,
                                           sbISQLBuilder::MATCH_EQUALS,
                                           getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    baseAlias.Assign(kMediaItemsAlias);
    rv = builder->SetBaseTableAlias(baseAlias);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddColumn(baseAlias, kMediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddJoin(sbISQLBuilder::JOIN_INNER,
                          kResourceProperties,
                          kConditionAlias,
                          kGuid,
                          baseAlias,
                          kGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddCriterionForCondition(builder, aCondition, info);
    NS_ENSURE_SUCCESS(rv, rv);

    // Only show media items
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = builder->CreateMatchCriterionNull(kMediaItemsAlias,
                                           kMediaListTypeId,
                                           sbISQLBuilder::MATCH_EQUALS,
                                           getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Add the value for the limit
  rv = AddLimitColumnAndJoin(builder, baseAlias);
  NS_ENSURE_SUCCESS(rv, rv);

  // If there is a select property name, pull it into the result of the query
  if (!mSelectPropertyName.IsEmpty()) {
    rv = AddSelectColumnAndJoin(builder, baseAlias, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // Just add a blank placeholder column
    rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("''"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = builder->ToString(_retval);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - CreateSQLForCondition() - %s",
         this, NS_LossyConvertUTF16toASCII(Substring(_retval, 0, 400)).get()));
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - CreateSQLForCondition() - %s",
         this, NS_LossyConvertUTF16toASCII(Substring(_retval, 400, 800)).get()));

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::AddCriterionForCondition(sbISQLSelectBuilder* aBuilder,
                                                        sbRefPtrCondition& aCondition,
                                                        sbIPropertyInfo* aInfo)
{
  NS_ENSURE_ARG_POINTER(aBuilder);
  NS_ENSURE_ARG_POINTER(aInfo);

  NS_NAMED_LITERAL_STRING(kConditionAlias, "_c");
  NS_NAMED_LITERAL_STRING(kGuid,           "guid");
  NS_NAMED_LITERAL_STRING(kObjSortable,    "obj_sortable");
  NS_NAMED_LITERAL_STRING(kPropertyId,     "property_id");

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
    columnName.Assign(kObjSortable);
    rv = mPropertyCache->GetPropertyID(aCondition->mPropertyName, &propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBuilder->CreateMatchCriterionLong(kConditionAlias,
                                            kPropertyId,
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
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
                                              columnName,
                                              sbISQLBuilder::MATCH_GREATEREQUAL,
                                              value,
                                              getter_AddRefs(left));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString rightValue;
    rv = aInfo->MakeSortable(aCondition->mRightValue, rightValue);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> right;
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
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
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
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
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
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

  NS_NAMED_LITERAL_STRING(kSimpleMediaLists, "simple_media_lists");
  NS_NAMED_LITERAL_STRING(kMediaItemId,      "media_item_id");

  nsCOMPtr<sbILocalDatabaseMediaItem> ldmi = do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLDeleteBuilder> deleteb =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);

  rv = deleteb->SetTableName(kSimpleMediaLists);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         kMediaItemId,
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mClearListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::AddSelectColumnAndJoin(sbISQLSelectBuilder* aBuilder,
                                                      const nsAString& aBaseTableAlias,
                                                      PRBool aAddOrderBy)
{
  NS_ENSURE_ARG_POINTER(aBuilder);

  NS_NAMED_LITERAL_STRING(kGuid,               "guid");
  NS_NAMED_LITERAL_STRING(kObjSortable,        "obj_sortable");
  NS_NAMED_LITERAL_STRING(kPropertyId,         "property_id");
  NS_NAMED_LITERAL_STRING(kResourceProperties, "resource_properties");
  NS_NAMED_LITERAL_STRING(kSelectAlias,        "_select");

  nsresult rv;

  if (SB_IsTopLevelProperty(mSelectPropertyName)) {
    // If the select property is a top level property, just add it to the
    // column list off the base table
    nsAutoString columnName;
    rv = SB_GetTopLevelPropertyColumn(mSelectPropertyName, columnName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBuilder->AddColumn(aBaseTableAlias, columnName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aAddOrderBy) {
      rv = aBuilder->AddOrder(aBaseTableAlias, columnName, mSelectDirection);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    // Otherwise, join the property to the base table
    rv = aBuilder->AddColumn(kSelectAlias, kObjSortable);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBuilder->AddJoin(sbISQLBuilder::JOIN_INNER,
                           kResourceProperties,
                           kSelectAlias,
                           kGuid,
                           aBaseTableAlias,
                           kGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyId;
    rv = mPropertyCache->GetPropertyID(mSelectPropertyName, &propertyId);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = aBuilder->CreateMatchCriterionLong(kSelectAlias,
                                            kPropertyId,
                                            sbISQLBuilder::MATCH_EQUALS,
                                            propertyId,
                                            getter_AddRefs(criterion));
    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    if (aAddOrderBy) {
      rv = aBuilder->AddOrder(kSelectAlias, kObjSortable, mSelectDirection);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::AddLimitColumnAndJoin(sbISQLSelectBuilder* aBuilder,
                                                     const nsAString& aBaseTableAlias)
{
  NS_ENSURE_ARG_POINTER(aBuilder);

  NS_NAMED_LITERAL_STRING(kContentLength,      "content_length");
  NS_NAMED_LITERAL_STRING(kGuid,               "guid");
  NS_NAMED_LITERAL_STRING(kLimitAlias,         "_limit");
  NS_NAMED_LITERAL_STRING(kObjSortable,        "obj_sortable");
  NS_NAMED_LITERAL_STRING(kPropertyId,         "property_id");
  NS_NAMED_LITERAL_STRING(kResourceProperties, "resource_properties");

  nsresult rv;

  switch(mLimitType) {
    case sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE:
    case sbILocalDatabaseSmartMediaList::LIMIT_TYPE_ITEMS:
      // For these limit types, we don't need any data so add a placeholder
      rv = aBuilder->AddColumn(EmptyString(), NS_LITERAL_STRING("0"));
      NS_ENSURE_SUCCESS(rv, rv);
    break;
    case sbILocalDatabaseSmartMediaList::LIMIT_TYPE_USECS:
      {
        // For microseconds limits, we need to join the duration property
        rv = aBuilder->AddColumn(kLimitAlias, kObjSortable);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aBuilder->AddJoin(sbISQLBuilder::JOIN_INNER,
                               kResourceProperties,
                               kLimitAlias,
                               kGuid,
                               aBaseTableAlias,
                               kGuid);
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 propertyId;
        rv = mPropertyCache->GetPropertyID(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
                                           &propertyId);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbISQLBuilderCriterion> criterion;
        rv = aBuilder->CreateMatchCriterionLong(kLimitAlias,
                                                kPropertyId,
                                                sbISQLBuilder::MATCH_EQUALS,
                                                propertyId,
                                                getter_AddRefs(criterion));
        rv = aBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    break;
    case sbILocalDatabaseSmartMediaList::LIMIT_TYPE_BYTES:
      // For bytes, we use the value of the content_length column
      rv = aBuilder->AddColumn(aBaseTableAlias, kContentLength);
      NS_ENSURE_SUCCESS(rv, rv);
    break;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::GetCopyToListQuery(const nsAString& aTempTableName,
                                                  nsAString& aSql)
{
  nsresult rv;

  nsCOMPtr<sbILocalDatabaseMediaItem> ldmi = do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldmi->GetMediaItemId(&mediaItemId);
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

  rv = select->SetBaseTableName(aTempTableName);
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

  rv = insert->ToString(aSql);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::CreateTempTable(nsAString& aName)
{
  nsresult rv;

  rv = MakeTempTableName(aName);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;
  sql.AssignLiteral("create table ");
  sql.Append(aName);
  sql.AppendLiteral(" (media_item_id integer unique, limitby integer, selectby text, count integer primary key autoincrement)");

  rv = ExecuteQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::DropTempTable(const nsAString& aName)
{
  nsresult rv;

  nsAutoString sql;
  sql.AssignLiteral("drop table ");
  sql.Append(aName);

  rv = ExecuteQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::ExecuteQuery(const nsAString& aSql)
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - ExecuteQuery() - %s",
         this, NS_LossyConvertUTF16toASCII(Substring(aSql, 0, 400)).get()));
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - ExecuteQuery() - %s",
         this, NS_LossyConvertUTF16toASCII(Substring(aSql, 400, 800)).get()));

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = mLocalDatabaseLibrary->CreateQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(aSql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}


nsresult
sbLocalDatabaseSmartMediaList::MakeTempTableName(nsAString& aName)
{
  nsresult rv;

  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid(NS_ConvertUTF8toUTF16(id.ToString()));

  nsAutoString stripped;

  // Extract all the numbers from the guid
  // 01234567890123456789012345789012345678
  // {5ee2439a-bb40-47d6-938b-de24a6c72815}
  stripped.Append(Substring(guid, 1, 8));
  stripped.Append(Substring(guid, 10, 4));
  stripped.Append(Substring(guid, 15, 4));
  stripped.Append(Substring(guid, 20, 4));
  stripped.Append(Substring(guid, 25, 12));

  nsAutoString tempTableName;
  tempTableName.AssignLiteral("temp_smart_");
  tempTableName.Append(stripped);

  aName = tempTableName;
  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::GetMediaItemIdRange(PRUint32* aMin,
                                                   PRUint32* aMax)
{
  nsAutoString sql;
  sql.AssignLiteral("select min(media_item_id), max(media_item_id) from media_items");

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = mLocalDatabaseLibrary->CreateQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount != 1) {
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoString temp;
  rv = result->GetRowCell(0, 0, temp);
  NS_ENSURE_SUCCESS(rv, rv);

  *aMin = temp.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = result->GetRowCell(0, 1, temp);
  NS_ENSURE_SUCCESS(rv, rv);

  *aMax = temp.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::GetRowCount(const nsAString& aTableName,
                                           PRUint32* _retval)
{
  nsAutoString sql;
  sql.AssignLiteral("select count(1) from ");
  sql.Append(aTableName);

  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - GetRowCount() - %s",
         this, NS_LossyConvertUTF16toASCII(sql).get()));

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = mLocalDatabaseLibrary->CreateQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount != 1) {
    return NS_ERROR_UNEXPECTED;
  }

  nsAutoString temp;
  rv = result->GetRowCell(0, 0, temp);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = temp.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*
 * \breif Shuffle the contents of the array using Fisher-Yates shuffle
 *        http://www.nist.gov/dads/HTML/fisherYatesShuffle.html
 */
void
sbLocalDatabaseSmartMediaList::ShuffleArray(sbMediaItemIdArray& aArray)
{
  PRUint32 n = aArray.Length();
  if (n > 1) {
    PRUint32 i;
	  for (i = 0; i < n - 1; i++) {
      PRUint32 j = i + rand() / (RAND_MAX / (n - i) + 1);
      PRUint32 t = aArray[j];
      aArray[j] = aArray[i];
      aArray[i] = t;
    }
  }
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
  mMatchType = sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY;
  mLimitType = sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE;
  mLimit = 0;
  mSelectPropertyName.Truncate();
  mSelectDirection = PR_TRUE;
  mRandomSelection = PR_FALSE;
  mLiveUpdate = PR_FALSE;
  mConditions.Clear();

  nsAutoString state;
  rv = mItem->GetProperty(NS_LITERAL_STRING(STATE_PROPERTY), state);
  NS_ENSURE_SUCCESS(rv, rv);

  // If no saved state is available, just return
  if (state.IsEmpty()) {
    return NS_OK;
  }

  // Parse the list's properties from the state
  rv = ParseQueryStringIntoHashtable(state, map);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString value;
  nsresult dontCare;

  if (map.Get(NS_LITERAL_STRING("matchType"), &value)) {
    mMatchType = value.ToInteger(&dontCare);
    if (!(mMatchType >= sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY &&
          mMatchType <= sbILocalDatabaseSmartMediaList::MATCH_TYPE_NONE)) {
      mMatchType = sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY;
    }
  }

  if (map.Get(NS_LITERAL_STRING("limitType"), &value)) {
    mLimitType = value.ToInteger(&dontCare);
    if (!(mLimitType >= sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE &&
          mLimitType <= sbILocalDatabaseSmartMediaList::LIMIT_TYPE_BYTES)) {
      mLimitType = sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE;
    }
  }

  if (map.Get(NS_LITERAL_STRING("limit"), &value)) {
    // Can't use ToInteger here since it is a 64 bit value
    PR_sscanf(NS_LossyConvertUTF16toASCII(value).get(), "%llu", &mLimit);
  }

  if (map.Get(NS_LITERAL_STRING("selectPropertyName"), &value)) {
    mSelectPropertyName = value;
  }

  if (map.Get(NS_LITERAL_STRING("selectDirection"), &value)) {
    mSelectDirection = value.EqualsLiteral("1");
  }

  if (map.Get(NS_LITERAL_STRING("randomSelection"), &value)) {
    mRandomSelection = value.EqualsLiteral("1");
  }

  if (map.Get(NS_LITERAL_STRING("liveUpdate"), &value)) {
    mLiveUpdate = value.EqualsLiteral("1");
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
        PRUint32 limit = sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE;

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
  match.AppendInt(mMatchType);
  success = map.Put(NS_LITERAL_STRING("matchType"), match);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString limitType;
  limitType.AppendInt(mLimitType);
  success = map.Put(NS_LITERAL_STRING("limitType"), limitType);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Can't use AppendInt here because limit is a 64 bit value
  nsAutoString limit;
  char buf[32];
  PR_snprintf(buf, sizeof(buf), "%llu", mLimit);
  limit.Append(NS_ConvertASCIItoUTF16(buf));
  success = map.Put(NS_LITERAL_STRING("limit"), limit);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("selectPropertyName"),
                    mSelectPropertyName);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString selectDirection;
  selectDirection.AppendLiteral(mSelectDirection ? "1" : "0");
  success = map.Put(NS_LITERAL_STRING("selectDirection"), selectDirection);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString randomSelection;
  randomSelection.AppendLiteral(mRandomSelection ? "1" : "0");
  success = map.Put(NS_LITERAL_STRING("randomSelection"), randomSelection);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString liveUpdate;
  liveUpdate.AppendLiteral(mLiveUpdate ? "1" : "0");
  success = map.Put(NS_LITERAL_STRING("liveUpdate"), liveUpdate);
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
                                             sbIMediaItem* aMediaItem,
                                             sbIPropertyArray* aProperties)
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

