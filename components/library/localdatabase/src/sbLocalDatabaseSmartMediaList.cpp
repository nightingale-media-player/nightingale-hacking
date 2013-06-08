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
#include <sbIPropertyInfo.h>
#include <sbIPropertyManager.h>
#include <sbISQLBuilder.h>
#include <sbLocalDatabaseSchemaInfo.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbStandardOperators.h>
#include <sbStandardProperties.h>
#include <sbILibraryManager.h>
#include <sbDummyProperties.h>
#include <sbStringUtils.h>
#include <nsAutoPtr.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsINetUtil.h>
#include <nsIObserverService.h>
#include <nsIUUIDGenerator.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsNetCID.h>
#include <nsServiceManagerUtils.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsVoidArray.h>
#include <prlog.h>
#include <prprf.h>
#include <prtime.h>
#include <mozilla/Mutex.h>
#include <mozilla/Monitor.h>

#define RANDOM_ADD_CHUNK_SIZE 1000;
#define SQL_IN_LIMIT 1000

#define ONEDAY (1000*60*60*24)
#define ONE_MS 1

static const char *gsFmtRadix10 = "%lld";

static char const LIBRARY_MANAGER_BEFORE_SHUTDOWN[] = "songbird-library-manager-before-shutdown";
static char const OBSERVER_SERVICE_CONTRACT_ID[] = "@mozilla.org/observer-service;1";

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

static nsresult
ParseAndAddChunk(const nsAString& aString,
                 sbStringMap& aMap)
{
  nsresult rv;
  nsCOMPtr<nsINetUtil> netUtil = do_GetService(NS_IOSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  static const PRUnichar sEquals = '=';

  PRInt32 length = aString.Length();

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

static nsresult ParseQueryStringIntoHashtable(const nsAString& aString,
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

static nsresult
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

sbLocalDatabaseSmartMediaListCondition::sbLocalDatabaseSmartMediaListCondition(const nsAString& aPropertyID,
                                                                               const nsAString& aOperatorString,
                                                                               const nsAString& aLeftValue,
                                                                               const nsAString& aRightValue,
                                                                               const nsAString& aDisplayUnit)
: mLock(nsnull)
, mPropertyID(aPropertyID)
, mOperatorString(aOperatorString)
, mLeftValue(aLeftValue)
, mRightValue(aRightValue)
, mDisplayUnit(aDisplayUnit)
{

}

sbLocalDatabaseSmartMediaListCondition::~sbLocalDatabaseSmartMediaListCondition()
{

}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetPropertyID(nsAString& aPropertyID)
{
  mozilla::MutexAutoLock lock(mLock);
  aPropertyID = mPropertyID;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetOperator(sbIPropertyOperator** aOperator)
{
  NS_ENSURE_ARG_POINTER(aOperator);

  mozilla::MutexAutoLock lock(mLock);
  
  if (!mOperator) {
    nsresult rv;
    // Get the property manager
    nsCOMPtr<sbIPropertyManager> propMan;
    propMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Get the property info for this property. 
    nsCOMPtr<sbIPropertyInfo> info;
    rv = propMan->GetPropertyInfo(mPropertyID, getter_AddRefs(info));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the operator.
    rv = info->GetOperator(mOperatorString, getter_AddRefs(mOperator));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aOperator = mOperator;
  NS_IF_ADDREF(*aOperator);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetLeftValue(nsAString& aLeftValue)
{
  mozilla::MutexAutoLock lock(mLock);
  aLeftValue = mLeftValue;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetRightValue(nsAString& aRightValue)
{
  mozilla::MutexAutoLock lock(mLock);
  aRightValue = mRightValue;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaListCondition::GetDisplayUnit(nsAString& aDisplayUnit)
{
  mozilla::MutexAutoLock lock(mLock);
  aDisplayUnit = mDisplayUnit;

  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaListCondition::ToString(nsAString& _retval)
{
  mozilla::MutexAutoLock lock(mLock);

  nsresult rv;

  sbStringMap map;
  PRBool success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("property"), mPropertyID);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("operator"), mOperatorString);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("leftValue"), mLeftValue);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("rightValue"), mRightValue);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = map.Put(NS_LITERAL_STRING("displayUnit"), mDisplayUnit);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  rv = JoinStringMapIntoQueryString(map, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//==============================================================================
// sbLocalDatabaseSmartMediaList
//==============================================================================


NS_IMPL_THREADSAFE_ISUPPORTS9(sbLocalDatabaseSmartMediaList,
                              nsIClassInfo,
                              nsISupportsWeakReference,
                              sbILibraryResource,
                              sbILocalDatabaseSmartMediaList,
                              sbILocalDatabaseMediaItem,
                              sbIMediaItem,
                              sbIMediaList,
                              sbIMediaListListener,
                              nsIObserver);

NS_IMPL_CI_INTERFACE_GETTER8(sbLocalDatabaseSmartMediaList,
                             nsIClassInfo,
                             nsISupportsWeakReference,
                             sbILibraryResource,
                             sbILocalDatabaseSmartMediaList,
                             sbIMediaItem,
                             sbIMediaList,
                             sbIMediaListListener,
                             nsIObserver);

sbLocalDatabaseSmartMediaList::sbLocalDatabaseSmartMediaList()
: mInnerMonitor(nsnull)
, mConditionsMonitor(nsnull)
, mMatchType(sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY)
, mLimitType(sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE)
, mLimit(0)
, mSelectDirection(PR_TRUE)
, mRandomSelection(PR_FALSE)
, mAutoUpdateMonitor(nsnull)
, mAutoUpdate(false)
, mNotExistsMode(sbILocalDatabaseSmartMediaList::NOTEXISTS_ASZERO)
, mListenersMonitor(nsnull)
, mSourceMonitor(nsnull)
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
  if (mItem) {
    /* use a single-iteration loop to allow breaking out easier */
    nsCOMPtr<sbILibrary> library;
    nsresult rv = mItem->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, /* void */);

    nsCOMPtr<sbIMediaList> libraryList = do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);

    (void) libraryList->RemoveListener(this);

    // Unregister ourselves as we're done
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(OBSERVER_SERVICE_CONTRACT_ID, &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);
    
    rv = observerService->RemoveObserver(this, LIBRARY_MANAGER_BEFORE_SHUTDOWN);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
}

/**
 * Utility class to suppress and automatically unsuppress notifications
 */
class sbAutoSuppressor
{
public:
  /**
   * Initialize and suppress
   */
  sbAutoSuppressor(sbIMediaItem * aItem) : mItem(do_QueryInterface(aItem)) 
  {
    Suppress();
  }
  /**
   * Allow notifications to occur
   */
  ~sbAutoSuppressor() 
  {
    Unsuppress();    
  }
  /**
   * Suppresses notifications
   */
  void Suppress()
  {
    if (mItem)
      mItem->SetSuppressNotifications(PR_TRUE);
  }
  /**
   * Allow notificatiosn
   */
  void Unsuppress()
  {
    if (mItem)
      mItem->SetSuppressNotifications(PR_FALSE);
  }
private:
  nsCOMPtr<sbILocalDatabaseMediaItem> mItem;
  // Disallow copying and assignment
  sbAutoSuppressor(sbAutoSuppressor const &);
  sbAutoSuppressor & operator =(sbAutoSuppressor const *&);
};

nsresult
sbLocalDatabaseSmartMediaList::Init(sbIMediaItem *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  mItem = aItem;
  
  nsresult rv;
  mLocalDBItem = do_QueryInterface(mItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsAutoString storageGuid;
  rv = mItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
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

  // Supress notifications until we're intialized
  sbAutoSuppressor suppressor(mediaItem);
  
  // let the inner list know about us
  nsAutoString guid;
  rv = GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> mi =
    do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mi->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_OUTERGUID), guid);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register for library shutdown
  nsCOMPtr<nsIObserverService> observerService =
      do_GetService(OBSERVER_SERVICE_CONTRACT_ID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = observerService->AddObserver(this, LIBRARY_MANAGER_BEFORE_SHUTDOWN, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register self for "before delete" so we can delete our storage list
  nsCOMPtr<sbIMediaList> libraryList = do_QueryInterface(library, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use a weak reference here because we already have a strong reference to
  // the inner list
  rv = libraryList->AddListener(this,
                                PR_TRUE,
                                sbIMediaList::LISTENER_FLAGS_ALL,
                                nsnull);
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
sbLocalDatabaseSmartMediaList::GetListContentType(PRUint16* aContentType)
{
  NS_ENSURE_ARG_POINTER(aContentType);
  nsresult rv;

  PRUint32 conditionLength = mConditions.Length();

  // No condition available. Smart playlist not ready.
  if (!conditionLength)
    return NS_ERROR_NOT_AVAILABLE;

  nsCOMPtr<sbILocalDatabaseSmartMediaListCondition> condition;
  nsCOMPtr<sbIPropertyOperator> propertyOperator;
  nsString propertyID, leftValue, operatorStr;
  for (PRUint32 i = 0; i < conditionLength; ++i) {
    condition = mConditions[i];

    rv = condition->GetPropertyID(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);
    // Deal with smart playlists with condition "mediatype".
    if (propertyID.EqualsLiteral(SB_PROPERTY_CONTENTTYPE)) {
      rv = condition->GetOperator(getter_AddRefs(propertyOperator));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = propertyOperator->GetOperator(operatorStr);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = condition->GetLeftValue(leftValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // XXX Alfred: Update the function whenever any content type other than
      //     audio or video is added in the future.

      // mediatype is video, or mediatype is not audio
      if ((operatorStr.EqualsLiteral(SB_OPERATOR_EQUALS) &&
           leftValue.EqualsLiteral("video")) ||
          (operatorStr.EqualsLiteral(SB_OPERATOR_NOTEQUALS) &&
           leftValue.EqualsLiteral("audio"))) {
        *aContentType = sbIMediaList::CONTENTTYPE_VIDEO;
        return NS_OK;
      }
      // mediatype is audio, or mediatype is not video
      else if ((operatorStr.EqualsLiteral(SB_OPERATOR_EQUALS) &&
                leftValue.EqualsLiteral("audio")) ||
               (operatorStr.EqualsLiteral(SB_OPERATOR_NOTEQUALS) &&
                leftValue.EqualsLiteral("video"))) {
        *aContentType = sbIMediaList::CONTENTTYPE_AUDIO;
        return NS_OK;
      }
    }
  }

  mList->GetListContentType(aContentType);
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
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);

  *aMatchType = mMatchType;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetMatchType(PRUint32 aMatchType)
{
  NS_ENSURE_ARG_RANGE(aMatchType,
    sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY,
    sbILocalDatabaseSmartMediaList::MATCH_TYPE_NONE);

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  mMatchType = aMatchType;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetConditionCount(PRUint32* aConditionCount)
{
  NS_ENSURE_ARG_POINTER(aConditionCount);

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
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

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
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
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  mLimit = aLimit;
  
  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetSelectPropertyID(nsAString& aSelectPropertyID)
{
  aSelectPropertyID = mSelectPropertyID;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetSelectPropertyID(const nsAString& aSelectPropertyID)
{
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  mSelectPropertyID = aSelectPropertyID;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetSelectDirection(PRBool* aSelectDirection)
{
  NS_ENSURE_ARG_POINTER(aSelectDirection);

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  *aSelectDirection = mSelectDirection;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetSelectDirection(PRBool aSelectDirection)
{
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  mSelectDirection = aSelectDirection;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetRandomSelection(PRBool* aRandomSelection)
{
  NS_ENSURE_ARG_POINTER(aRandomSelection);

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  *aRandomSelection = mRandomSelection;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetRandomSelection(PRBool aRandomSelection)
{
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  mRandomSelection = aRandomSelection;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetAutoUpdate(PRBool* aAutoUpdate)
{
  NS_ENSURE_ARG_POINTER(aAutoUpdate);

  mozilla::MonitorAutoLock monitor(mAutoUpdateMonitor);
  *aAutoUpdate = mAutoUpdate;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetAutoUpdate(PRBool aAutoUpdate)
{
  mozilla::MonitorAutoLock monitor(mAutoUpdateMonitor);
  mAutoUpdate = aAutoUpdate;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetNotExistsMode(PRUint32* aNotExistsMode)
{
  NS_ENSURE_ARG_POINTER(aNotExistsMode);

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  *aNotExistsMode = mNotExistsMode;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetNotExistsMode(PRUint32 aNotExistsMode)
{
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  mNotExistsMode = aNotExistsMode;

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::AppendCondition(const nsAString& aPropertyID,
                                               sbIPropertyOperator* aOperator,
                                               const nsAString& aLeftValue,
                                               const nsAString& aRightValue,
                                               const nsAString& aDisplayUnit,
                                               sbILocalDatabaseSmartMediaListCondition** _retval)
{
  NS_ENSURE_ARG_POINTER(aOperator);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_ARG(aPropertyID.Length() > 1);

  // Make sure the right value is void if the operator is anything else but
  // between, and that both are void if the operator is istrue, isfalse,
  // isset, or isnotset
  nsAutoString op;
  nsresult rv = aOperator->GetOperator(op);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXsteve IsEmpty should really be IsVoid
  if (!op.EqualsLiteral(SB_OPERATOR_BETWEEN) &&
      !op.EqualsLiteral(SB_OPERATOR_BETWEENDATES)) {
    if (!aRightValue.IsEmpty())
      return NS_ERROR_ILLEGAL_VALUE;
  } else {
    if (aRightValue.IsEmpty())
      return NS_ERROR_ILLEGAL_VALUE;
  }

  if ((op.EqualsLiteral(SB_OPERATOR_ISTRUE) ||
       op.EqualsLiteral(SB_OPERATOR_ISFALSE) ||
       op.EqualsLiteral(SB_OPERATOR_ISSET) ||
       op.EqualsLiteral(SB_OPERATOR_ISNOTSET)) &&
      !aLeftValue.IsEmpty()) {
    return NS_ERROR_ILLEGAL_VALUE;
  };

  sbRefPtrCondition condition;
  condition = new sbLocalDatabaseSmartMediaListCondition(aPropertyID,
                                                         op,
                                                         aLeftValue,
                                                         aRightValue,
                                                         aDisplayUnit);
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
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
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

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);
  PRUint32 count = mConditions.Length();

  NS_ENSURE_ARG(aConditionIndex < count);

  *_retval = mConditions[aConditionIndex];
  NS_ADDREF(*_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::ClearConditions()
{
  mozilla::MonitorAutoLock monitor(mConditionsMonitor);

  mConditions.Clear();

  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::Rebuild()
{
  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - Rebuild()", this));

  nsresult rv;
  {
	mozilla::MonitorAutoLock monitor(mConditionsMonitor);
	mozilla::MonitorAutoLock monitor2(mSourceMonitor);

    // You must either have a match or a limit
    NS_ENSURE_ARG(!(mMatchType == sbILocalDatabaseSmartMediaList::MATCH_TYPE_NONE &&
                    mLimitType == sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE));

    // If we have a limit, either random or the selection property must be set
    if (mLimitType != sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE) {
      if (!(mRandomSelection || !mSelectPropertyID.IsEmpty()))
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
  }
  // Notify our inner list that its content changed
  nsCOMPtr<sbILocalDatabaseSimpleMediaList> ldsml =
    do_QueryInterface(mList, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ldsml->NotifyContentChanged();
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::MonitorAutoLock monitor(mListenersMonitor);
  for (PRInt32 i=0;i<mListeners.Count();i++)
    mListeners.ObjectAt(i)->OnRebuild(this);

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
  // insert statement with a compound select statement made up of select
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
    rv = CreateSQLForCondition(mConditions[i], i == count-1, conditionSql);
    NS_ENSURE_SUCCESS(rv, rv);

    sql.Append(conditionSql);
    if (i + 1 < count) {
      // Join the select statements with "intersect" if we are doing an ALL
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

    // If this is not an item limit, figure out what our row limit should be
    PRUint32 rowLimit;
    if (mLimitType == sbILocalDatabaseSmartMediaList::LIMIT_TYPE_ITEMS) {
      // rolling limit value is 64 bits (since it accommodates for tiny units
      // like bits), but rolling queries are limited to 2^32 rows of results.
      // Since this is a limit by number of items, we have to cast mLimit down
      // to 32 bits. Avoid a warning here by explicitly masking & casting.
      rowLimit = (PRUint32)(mLimit & 0xFFFFFFFF);
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
  NS_ENSURE_STATE(!mSelectPropertyID.IsEmpty());

  NS_NAMED_LITERAL_STRING(kMediaItems,      "media_items");
  NS_NAMED_LITERAL_STRING(kMediaItemId,     "media_item_id");
  NS_NAMED_LITERAL_STRING(kMediaItemsAlias, "_mi");
  NS_NAMED_LITERAL_STRING(kLimitAlias,      "_limit");
  NS_NAMED_LITERAL_STRING(kMediaListTypeId, "media_list_type_id");
  
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

    // Only get media items
    nsCOMPtr<sbISQLBuilderCriterion> nullCriterion;
    rv = builder->CreateMatchCriterionNull(kMediaItemsAlias,
                                           kMediaListTypeId,
                                           sbISQLBuilder::MATCH_EQUALS,
                                           getter_AddRefs(nullCriterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(nullCriterion);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString sql;
    rv = builder->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = GetRollingLimit(sql, 1, &limitRow);
    NS_ENSURE_SUCCESS(rv, rv);

  }
  else {
    // Otherwise, the limit row is the limit
    // rolling limit value is 64 bits (since it accommodates for tiny units
    // like bits), but rolling queries are limited to 2^32 rows of results.
    // Since this is a limit by number of items, we have to cast mLimit down
    // to 32 bits. Avoid a warning here by explicitly masking & casting.
    limitRow = (PRUint32)(mLimit & 0xFFFFFFFF);
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

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("0"));
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
  NS_ENSURE_STATE(mSelectPropertyID.IsEmpty());

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
        // rolling limit value is 64 bits (since it accommodates for tiny units
        // like bits), but rolling queries are limited to 2^32 rows of results.
        // Since this is a limit by number of items, we have to cast mLimit down
        // to 32 bits. Avoid a warning here by explicitly masking & casting.
        rowLimit = (PRUint32)(mLimit & 0xFFFFFFFF);
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

      // we need the results in random order, otherwise a limit will always
      // pick items from the top
      rv = builder->AddRandomOrder();
      NS_ENSURE_SUCCESS(rv, rv);

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
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  rv = query->GetRollingLimitResult(aRow);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbLocalDatabaseSmartMediaList[0x%.8x] - GetRollingLimit() - %s = %d",
         this, NS_ConvertUTF16toUTF8(aSql).get(), *aRow));

  return NS_OK;
}
#include <stdio.h>
nsresult
sbLocalDatabaseSmartMediaList::CreateSQLForCondition(sbRefPtrCondition& aCondition,
                                                     PRBool aAddOrderBy,
                                                     nsAString& _retval)
{
  nsresult rv;

  NS_NAMED_LITERAL_STRING(kConditionAlias,     "_c");
  NS_NAMED_LITERAL_STRING(kMediaItemId,        "media_item_id");
  NS_NAMED_LITERAL_STRING(kMemberMediaItemId,  "member_media_item_id");
  NS_NAMED_LITERAL_STRING(kMediaItems,         "media_items");
  NS_NAMED_LITERAL_STRING(kMediaLists,         "simple_media_lists");
  NS_NAMED_LITERAL_STRING(kMediaItemsAlias,    "_mi");
  NS_NAMED_LITERAL_STRING(kMediaListTypeId,    "media_list_type_id");
  NS_NAMED_LITERAL_STRING(kPropertyId,         "property_id");
  NS_NAMED_LITERAL_STRING(kResourceProperties, "resource_properties");

  // Get the property info for this property.  If the property info is not
  // found, return not available
  nsCOMPtr<sbIPropertyInfo> info;
  rv = mPropMan->GetPropertyInfo(aCondition->mPropertyID,
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

  PRBool isTopLevelProperty = SB_IsTopLevelProperty(aCondition->mPropertyID);

  rv = builder->SetBaseTableName(kMediaItems);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString mediaColumn;
  nsAutoString joinTable;
  PRBool bIsMediaListMatch;
  
  bIsMediaListMatch = aCondition->mPropertyID.EqualsLiteral(SB_DUMMYPROPERTY_SMARTMEDIALIST_PLAYLIST);
  
  // use a different refptr so we can reassign it without modifying the actual
  // condition
  sbRefPtrCondition ruleCondition = aCondition;

  if (bIsMediaListMatch) {
    if (ruleCondition->mLeftValue.IsEmpty()) {
      // This condition is of the form "Playlist Is|IsNot Library". This is a
      // very special case, because although we could tweak the query to match
      // any track that is in no playlist at all (for "Is") or in at least one
      // playlist (for "IsNot"), that is not actually what we want because
      // every other update would add the tracks to the result list, and each
      // subsequent one would empty the list. What we want is "Is Library" to
      // match everything, and "IsNot Library" to match nothing at all. Yes it
      // means that both rules are useless, but they're only valid so that the
      // default state for a new playlist rule is a valid one (see bug 12046).
      // To this end, we'll construct a dummy rule here, that either matches
      // everything or nothing depending on the operator used, it will be of
      // the form "guid Is|IsNot ''".

      nsCOMPtr<sbIPropertyOperator> opObj;
      rv = ruleCondition->GetOperator(getter_AddRefs(opObj));
      NS_ENSURE_SUCCESS(rv, rv);
      
      nsAutoString op;
      rv = opObj->GetOperator(op);
      NS_ENSURE_SUCCESS(rv, rv);
      
      nsAutoString newOp;
      if (op.EqualsLiteral(SB_OPERATOR_EQUALS))
        newOp = NS_LITERAL_STRING(SB_OPERATOR_ISSET);
      else
        newOp = NS_LITERAL_STRING(SB_OPERATOR_ISNOTSET);

      ruleCondition = new 
        sbLocalDatabaseSmartMediaListCondition(NS_LITERAL_STRING(SB_PROPERTY_GUID),
                                               newOp,
                                               NS_LITERAL_STRING(""),
                                               NS_LITERAL_STRING(""),
                                               NS_LITERAL_STRING(""));
      bIsMediaListMatch = PR_FALSE;
      isTopLevelProperty = PR_TRUE;
    } else {
      // Otherwise, use the playlist table to join
      mediaColumn = kMemberMediaItemId;
      joinTable = kMediaLists;
    }
  } else {
    mediaColumn = kMediaItemId;
    joinTable = kResourceProperties;
  }

  nsAutoString baseAlias;

  // Add condition query
  if (isTopLevelProperty) {
    baseAlias.Assign(kConditionAlias);
    rv = builder->SetBaseTableAlias(baseAlias);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddColumn(baseAlias, kMediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = AddCriterionForCondition(builder, ruleCondition, info);
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
    
    builder->SetDistinct(PR_TRUE);

    rv = builder->AddColumn(baseAlias, kMediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Create a left outer join on two condition: media_item_id matching
    // the left table, and property_id matching the rule property id, so that
    // non-existing properties end up in the result set as null values
    
    // media_item_id match with left table media_item_id
    nsCOMPtr<sbISQLBuilderCriterion> criterion_media_item;
    rv = builder->CreateMatchCriterionTable(baseAlias,
                                            kMediaItemId,
                                            sbISQLBuilder::MATCH_EQUALS,
                                            kConditionAlias,
                                            mediaColumn,
                                            getter_AddRefs(criterion_media_item));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 longVal;
    nsAutoString colId;
    if (bIsMediaListMatch) {
      rv = MediaListGuidToDB(ruleCondition->mLeftValue, longVal);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (longVal == (PRUint32)-1) 
        return NS_ERROR_UNEXPECTED;
      
      colId = kMediaItemId;
    } else {
      // get property db id for rule property
      rv = mPropertyCache->GetPropertyDBID(ruleCondition->mPropertyID, &longVal);
      NS_ENSURE_SUCCESS(rv, rv);
      
      colId = kPropertyId;
    }

    nsCOMPtr<sbISQLBuilderCriterion> join_criterion;
    
    // property_id match with property db id
    nsCOMPtr<sbISQLBuilderCriterion> criterion_property_id;
    rv = builder->CreateMatchCriterionLong(kConditionAlias,
                                           colId,
                                           sbISQLBuilder::MATCH_EQUALS,
                                           longVal,
                                           getter_AddRefs(criterion_property_id));
    NS_ENSURE_SUCCESS(rv, rv);

    // combine the two conditions with AND
    rv = builder->CreateAndCriterion(criterion_media_item, 
                                     criterion_property_id, 
                                     getter_AddRefs(join_criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    // create a left outer join with these conditions, so we get null values
    // for items that do not have the property we're looking for.
    rv = builder->AddJoinWithCriterion(sbISQLBuilder::JOIN_LEFT_OUTER,
                                       joinTable,
                                       kConditionAlias,
                                       join_criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    // add conditions for this rule
    rv = AddCriterionForCondition(builder, ruleCondition, info);
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

  // If there is a limit on a select property id, pull it into the result of
  // the query, and sort the results by that property
  if (mLimit != sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE &&
      !mSelectPropertyID.IsEmpty()) {
    // only add the sort to the last condition, because it is invalid (and
    // would be quite unnecessary anyway) to sort before an intersect/union
    rv = AddSelectColumnAndJoin(builder, baseAlias, aAddOrderBy);
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
sbLocalDatabaseSmartMediaList::ScanfInt64(nsAString &aString, PRInt64 *aRetVal) {
  PRTime value = 0;
  NS_ConvertUTF16toUTF8 narrow(aString);

  if (PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    return NS_ERROR_INVALID_ARG;
  }
  *aRetVal = value;
  return NS_OK;
}

PRInt64
sbLocalDatabaseSmartMediaList::ScanfInt64d(nsAString &aString) {
  PRTime value = 0;
  NS_ConvertUTF16toUTF8 narrow(aString);

  if (PR_sscanf(narrow.get(), gsFmtRadix10, &value) != 1) {
    return 0;
  }
  return value;
}

void
sbLocalDatabaseSmartMediaList::SPrintfInt64(nsAString &aString, 
                                            PRInt64 aValue) {
  char out[32] = {0};
  if (PR_snprintf(out, 32, gsFmtRadix10, aValue) == (PRUint32)-1) {
    aString = NS_LITERAL_STRING("0");
  }
  NS_ConvertUTF8toUTF16 wide(out);
  aString = wide;
}

PRInt64
sbLocalDatabaseSmartMediaList::StripTime(PRInt64 aDateTime) {
  PRExplodedTime explodedTime = {0};
  PR_ExplodeTime(aDateTime * PR_USEC_PER_MSEC, PR_LocalTimeParameters, &explodedTime);
  explodedTime.tm_usec = 0;
  explodedTime.tm_sec = 0;
  explodedTime.tm_min = 0;
  explodedTime.tm_hour = 0;
  return PR_ImplodeTime(&explodedTime) / PR_USEC_PER_MSEC;
}

nsresult
sbLocalDatabaseSmartMediaList::AddCriterionForCondition(sbISQLSelectBuilder* aBuilder,
                                                        sbRefPtrCondition& aCondition,
                                                        sbIPropertyInfo* aInfo)
{
  NS_ENSURE_ARG_POINTER(aBuilder);
  NS_ENSURE_ARG_POINTER(aInfo);

  NS_NAMED_LITERAL_STRING(kConditionAlias, "_c");
  NS_NAMED_LITERAL_STRING(kObjSearchable,  "obj_searchable");
  NS_NAMED_LITERAL_STRING(kMediaItem,      "media_item_id");

  nsresult rv;

  // Get some stuff about the property
  PRBool isTopLevelProperty = SB_IsTopLevelProperty(aCondition->mPropertyID);
  nsAutoString columnName;

  PRBool bNeedOrIsNull;
  rv = GetConditionNeedsNull(aCondition, aInfo, bNeedOrIsNull);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isTopLevelProperty) {
    rv = SB_GetTopLevelPropertyColumn(aCondition->mPropertyID, columnName);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    columnName.Assign(kObjSearchable);
  }

  nsCOMPtr<sbIPropertyOperator> opObj;
  rv = aCondition->GetOperator(getter_AddRefs(opObj));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString op;
  rv = opObj->GetOperator(op);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString value;
  nsAutoString leftValue;
  nsAutoString rightValue;
  
  if (op.EqualsLiteral(SB_OPERATOR_ISTRUE) ||
      op.EqualsLiteral(SB_OPERATOR_ISFALSE)) {
    leftValue = NS_LITERAL_STRING("1");
  } else if (op.EqualsLiteral(SB_OPERATOR_ISSET) ||
             op.EqualsLiteral(SB_OPERATOR_ISNOTSET)) {
    leftValue = NS_LITERAL_STRING("");
  } else if (op.EqualsLiteral(SB_OPERATOR_INTHELAST) ||
             op.EqualsLiteral(SB_OPERATOR_NOTINTHELAST)) {

    PRTime timeValue = 0;
    NS_ConvertUTF16toUTF8 narrow(aCondition->mLeftValue);

    if(PR_sscanf(narrow.get(), gsFmtRadix10, &timeValue) != 1) {
      return NS_ERROR_INVALID_ARG;
    }

    char out[32] = {0};
    
    PRTime now = PR_Now()/PR_USEC_PER_MSEC;
    PRTime when = now - timeValue;
    if(PR_snprintf(out, 32, gsFmtRadix10, when) == (PRUint32)-1) {
      return NS_ERROR_FAILURE;
    }
    NS_ConvertUTF8toUTF16 wide(out);
    leftValue = wide;
  } else {
    leftValue = aCondition->mLeftValue;
  }

  PRBool bNumericCondition = PR_FALSE;
  if (aCondition->mPropertyID.EqualsLiteral(SB_DUMMYPROPERTY_SMARTMEDIALIST_PLAYLIST)) {
    columnName.Assign(kMediaItem);
    
    PRUint32 id;
    rv = MediaListGuidToDB(leftValue, id);
    NS_ENSURE_SUCCESS(rv, rv);
    
    leftValue.Truncate();
    leftValue.AppendInt(id);
    
    bNumericCondition = PR_TRUE;
  }

  PRBool invertRange = PR_FALSE;
  
  rightValue = aCondition->mRightValue;
  
  if (op.EqualsLiteral(SB_OPERATOR_BETWEENDATES)) {
    // if we are matching date only (and not time), both values for the range
    // needs to be parsed, and stripped of time, then the right value should be
    // added 23:59:59.9999.
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue)));
    SPrintfInt64(rightValue, StripTime(ScanfInt64d(rightValue))+(ONEDAY-ONE_MS));
    op = NS_LITERAL_STRING(SB_OPERATOR_BETWEEN);
  } else if (op.EqualsLiteral(SB_OPERATOR_ONDATE)) {
    // If we are matching date only (and not time), the date must be stripped of
    // its time component, then we need to turn the condition into a range
    // of [date,date+23:59:59.9999]
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue)));
    SPrintfInt64(rightValue, StripTime(ScanfInt64d(leftValue))+(ONEDAY-ONE_MS));
    op = NS_LITERAL_STRING(SB_OPERATOR_BETWEEN);
  } else if (op.EqualsLiteral(SB_OPERATOR_NOTONDATE)) {
    // If we are matching date only (and not time), the date must be stripped of
    // its time component, then we need to turn the condition into an inverted
    // range of [date, date+23:59:59.9999]
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue)));
    SPrintfInt64(rightValue, StripTime(ScanfInt64d(leftValue))+(ONEDAY-ONE_MS));
    op = NS_LITERAL_STRING(SB_OPERATOR_BETWEEN);
    invertRange = PR_TRUE;
  } else if (op.EqualsLiteral(SB_OPERATOR_BEFOREDATE)) {
    // If we are matching date only (and not time), the date must be stripped of
    // its time component.
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue)));
    op = NS_LITERAL_STRING(SB_OPERATOR_LESS);
  } else if (op.EqualsLiteral(SB_OPERATOR_BEFOREORONDATE)) {
    // If we are matching date only (and not time), the date must be stripped of
    // its time component, and then added 23:59:59.9999.
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue))+(ONEDAY-ONE_MS));
    op = NS_LITERAL_STRING(SB_OPERATOR_LESSEQUAL);
  } else if (op.EqualsLiteral(SB_OPERATOR_AFTERDATE)) {
    // If we are matching date only (and not time), the date must be stripped of
    // its time component, and then added 23:59:59.9999.
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue))+(ONEDAY-ONE_MS));
    op = NS_LITERAL_STRING(SB_OPERATOR_GREATER);
  } else if (op.EqualsLiteral(SB_OPERATOR_AFTERORONDATE)) {
    // If we are matching date only (and not time), the date must be stripped of
    // its time component.
    SPrintfInt64(leftValue, StripTime(ScanfInt64d(leftValue)));
    op = NS_LITERAL_STRING(SB_OPERATOR_GREATEREQUAL);
  }
  
  if (!leftValue.IsEmpty()) {
    rv = aInfo->MakeSearchable(leftValue, value);
    // MakeSearchable may fail if the value fails to validate, but since a smart
    // playlist may look for substrings instead of full valid values, it is not
    // fatal to fail to make searchable, when that fails we just use the value
    // that we were given as is and escape it properly.
    if (NS_FAILED(rv)) {
      nsCOMPtr<nsINetUtil> netUtil =
        do_CreateInstance("@mozilla.org/network/util;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString spec;
      rv = netUtil->EscapeString(NS_ConvertUTF16toUTF8(leftValue),
                                 nsINetUtil::ESCAPE_URL_PATH,
                                 spec);
      NS_ENSURE_SUCCESS(rv, rv);

      value = NS_ConvertUTF8toUTF16(spec);
    }
  }

  // If this is a between operator, construct two conditions for it
  if (op.EqualsLiteral(SB_OPERATOR_BETWEEN)) {
    nsCOMPtr<sbISQLBuilderCriterion> left;
    PRUint32 matchType;
    if (invertRange)
      matchType = sbISQLBuilder::MATCH_LESS;
    else
      matchType = sbISQLBuilder::MATCH_GREATEREQUAL;
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
                                              columnName,
                                              matchType,
                                              value,
                                              getter_AddRefs(left));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString rvalue;
    rv = aInfo->MakeSearchable(rightValue, rvalue);
    // MakeSearchable may fail if the value fails to validate, but since a smart
    // playlist may look for substrings instead of full valid values, it is not
    // fatal to fail to make searchable, when that fails we just use the value
    // that we were given as is.
    if (NS_FAILED(rv)) {
      rvalue = rightValue;
    }

    nsCOMPtr<sbISQLBuilderCriterion> right;
    if (invertRange)
      matchType = sbISQLBuilder::MATCH_GREATER;
    else
      matchType = sbISQLBuilder::MATCH_LESSEQUAL;
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
                                              columnName,
                                              matchType,
                                              rvalue,
                                              getter_AddRefs(right));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    if (invertRange)
      rv = aBuilder->CreateOrCriterion(left, right, getter_AddRefs(criterion));
    else
      rv = aBuilder->CreateAndCriterion(left, right, getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    if (bNeedOrIsNull) {
      nsCOMPtr<sbISQLBuilderCriterion> orIsNull;
      rv = aBuilder->CreateMatchCriterionNull(kConditionAlias,
                                              columnName,
                                              sbISQLBuilder::MATCH_EQUALS,
                                              getter_AddRefs(orIsNull));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbISQLBuilderCriterion> criterionOrIsNull;
      rv = aBuilder->CreateOrCriterion(criterion, 
                                       orIsNull, 
                                       getter_AddRefs(criterionOrIsNull));
      NS_ENSURE_SUCCESS(rv, rv);
      criterion = criterionOrIsNull;
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
  else if (op.EqualsLiteral(SB_OPERATOR_ISTRUE)) {
    matchType = sbISQLBuilder::MATCH_EQUALS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_ISFALSE)) {
    matchType = sbISQLBuilder::MATCH_NOTEQUALS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_INTHELAST)) {
    matchType = sbISQLBuilder::MATCH_GREATEREQUAL;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_NOTINTHELAST)) {
    matchType = sbISQLBuilder::MATCH_LESS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_ISSET)) {
    matchType = sbISQLBuilder::MATCH_NOTEQUALS;
  }
  else if (op.EqualsLiteral(SB_OPERATOR_ISNOTSET)) {
    matchType = sbISQLBuilder::MATCH_EQUALS;
  }
  
  if (matchType >= 0) {
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    if (bNumericCondition) {
      PRInt64 numericValue;
      rv = ScanfInt64(value, &numericValue);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aBuilder->CreateMatchCriterionLongLong(kConditionAlias,
                                                  columnName,
                                                  matchType,
                                                  numericValue,
                                                  getter_AddRefs(criterion));
    } else {
      rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
                                                columnName,
                                                matchType,
                                                value,
                                                getter_AddRefs(criterion));
    }
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (bNeedOrIsNull) {
      nsCOMPtr<sbISQLBuilderCriterion> orIsNull;
      rv = aBuilder->CreateMatchCriterionNull(kConditionAlias,
                                              columnName,
                                              sbISQLBuilder::MATCH_EQUALS,
                                              getter_AddRefs(orIsNull));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbISQLBuilderCriterion> criterionOrIsNull;
      rv = aBuilder->CreateOrCriterion(criterion, 
                                       orIsNull, 
                                       getter_AddRefs(criterionOrIsNull));
      NS_ENSURE_SUCCESS(rv, rv);
      criterion = criterionOrIsNull;
    }

    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // If were are any of the contains style operators, handle it here
  if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
      op.EqualsLiteral(SB_OPERATOR_NOTCONTAINS) ||
      op.EqualsLiteral(SB_OPERATOR_ENDSWITH) ||
      op.EqualsLiteral(SB_OPERATOR_NOTENDSWITH) ||
      op.EqualsLiteral(SB_OPERATOR_BEGINSWITH) ||
      op.EqualsLiteral(SB_OPERATOR_NOTBEGINSWITH)) {

    nsAutoString like;
    if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_NOTCONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_ENDSWITH) ||
        op.EqualsLiteral(SB_OPERATOR_NOTENDSWITH)) {
      like.AppendLiteral("%");
    }

    like.Append(value);

    if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_NOTCONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_BEGINSWITH) ||
        op.EqualsLiteral(SB_OPERATOR_NOTBEGINSWITH)) {
      like.AppendLiteral("%");
    }

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    PRUint32 match;
    if (op.EqualsLiteral(SB_OPERATOR_CONTAINS) ||
        op.EqualsLiteral(SB_OPERATOR_BEGINSWITH) ||
        op.EqualsLiteral(SB_OPERATOR_ENDSWITH)) {
      match = sbISQLBuilder::MATCH_LIKE;
    } else {
      match = sbISQLBuilder::MATCH_NOTLIKE;
    }
        
    rv = aBuilder->CreateMatchCriterionString(kConditionAlias,
                                              columnName,
                                              match,
                                              like,
                                              getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    if (bNeedOrIsNull) {
      nsCOMPtr<sbISQLBuilderCriterion> orIsNull;
      rv = aBuilder->CreateMatchCriterionNull(kConditionAlias,
                                              columnName,
                                              sbISQLBuilder::MATCH_EQUALS,
                                              getter_AddRefs(orIsNull));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbISQLBuilderCriterion> criterionOrIsNull;
      rv = aBuilder->CreateOrCriterion(criterion, 
                                       orIsNull, 
                                       getter_AddRefs(criterionOrIsNull));
      NS_ENSURE_SUCCESS(rv, rv);
      criterion = criterionOrIsNull;
    }

    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  // If we get here, we don't know how to handle the supplied operator
  return NS_ERROR_UNEXPECTED;
}

nsresult sbLocalDatabaseSmartMediaList::MediaListGuidToDB(nsAString &val, PRUint32 &v) 
{
  nsCOMPtr<sbILibraryManager> libraryManager;
  nsresult rv;
  
  nsAutoString guid;
  guid = val;
  
  libraryManager = 
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbILibrary> library;
  if (!mSourceLibraryGuid.IsEmpty()) {
    rv = libraryManager->GetLibrary(mSourceLibraryGuid, getter_AddRefs(library));
  } else {
    rv = libraryManager->GetMainLibrary(getter_AddRefs(library));
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(val, getter_AddRefs(item));
  if (rv != NS_OK) {
    v = (PRUint32)-1;
  } else {
    nsAutoString storageGuid;
    rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_STORAGEGUID),
                           storageGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!storageGuid.IsEmpty()) {
      nsCOMPtr<sbIMediaItem> storageItem;
      rv = library->GetMediaItem(storageGuid, getter_AddRefs(storageItem));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = storageItem->GetGuid(guid);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    PRUint32 id;

    nsCOMPtr<sbILocalDatabaseLibrary> locallibrary =
      do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = locallibrary->GetMediaItemIdForGuid(guid, &id);
    NS_ENSURE_SUCCESS(rv, rv);

    v = id;
  }
      
  return NS_OK;
}

nsresult
sbLocalDatabaseSmartMediaList::GetConditionNeedsNull(sbRefPtrCondition& aCondition,
                                                     sbIPropertyInfo* aInfo,
                                                     PRBool &bNeedIsNull)
{
  nsresult rv;
  
  if (mNotExistsMode == sbILocalDatabaseSmartMediaList::NOTEXISTS_ASNULL) {
    bNeedIsNull = PR_FALSE;
    return NS_OK;
  }
  
  nsCOMPtr<sbIPropertyOperator> opObj;
  rv = aCondition->GetOperator(getter_AddRefs(opObj));
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(opObj, NS_ERROR_FAILURE);

  nsAutoString op;
  rv = opObj->GetOperator(op);
  NS_ENSURE_SUCCESS(rv, rv);

  if (op.EqualsLiteral(SB_OPERATOR_ISFALSE)) {
    bNeedIsNull = PR_TRUE;
    return NS_OK;
  }
  if (op.EqualsLiteral(SB_OPERATOR_ISTRUE)){
    bNeedIsNull = PR_FALSE;
    return NS_OK;
  }
  if (op.EqualsLiteral(SB_OPERATOR_ISSET)) {
    bNeedIsNull = PR_FALSE;
    return NS_OK;
  }
  if (op.EqualsLiteral(SB_OPERATOR_ISNOTSET)){
    bNeedIsNull = PR_TRUE;
    return NS_OK;
  }

  nsAutoString leftValue, value;
  leftValue = aCondition->mLeftValue;
  if (!leftValue.IsEmpty()) {
    rv = aInfo->MakeSearchable(leftValue, value);
    // MakeSearchable may fail if the value fails to validate, but since a smart
    // playlist may look for substrings instead of full valid values, it is not
    // fatal to fail to make searchable, when that fails we just use the value
    // that we were given as is.
    if (NS_FAILED(rv)) {
      value = leftValue;
    }
  }
  
  PRBool bIsEmpty = value.IsEmpty();

  PRInt64 fValue;
  PRBool bIsNumber = PR_TRUE;
  rv = ScanfInt64(value, &fValue);
  if (rv != NS_OK) {
    fValue = 0;
    bIsNumber = PR_FALSE;
  }

  if (op.EqualsLiteral(SB_OPERATOR_EQUALS) ||
      op.EqualsLiteral(SB_OPERATOR_ONDATE)) {
    if ((bIsNumber && fValue == 0) || 
        (!bIsNumber && bIsEmpty)) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }
  
  if (op.EqualsLiteral(SB_OPERATOR_NOTEQUALS) ||
      op.EqualsLiteral(SB_OPERATOR_NOTONDATE)) {
    if ((bIsNumber && fValue != 0) || 
        (!bIsNumber && !bIsEmpty)) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }

  if (op.EqualsLiteral(SB_OPERATOR_GREATER)) {
    if (fValue < 0) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }

  if (op.EqualsLiteral(SB_OPERATOR_GREATEREQUAL) ||
      op.EqualsLiteral(SB_OPERATOR_BETWEEN)) {
    if (fValue <= 0) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }

  if (op.EqualsLiteral(SB_OPERATOR_LESS)) {
    if (fValue > 0) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }

  if (op.EqualsLiteral(SB_OPERATOR_LESSEQUAL)) {
    if (fValue >= 0) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }
  
  if (op.EqualsLiteral(SB_OPERATOR_NOTCONTAINS) ||
      op.EqualsLiteral(SB_OPERATOR_NOTBEGINSWITH) ||
      op.EqualsLiteral(SB_OPERATOR_NOTENDSWITH) ||
      op.EqualsLiteral(SB_OPERATOR_NOTINTHELAST) ||
      op.EqualsLiteral(SB_OPERATOR_NOTONDATE)) {
    if (!bIsEmpty) {
      bNeedIsNull = PR_TRUE;
      return NS_OK;
    }
  }

  bNeedIsNull = PR_FALSE;
  return NS_OK;
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

  NS_NAMED_LITERAL_STRING(kObjSortable,        "obj_sortable");
  NS_NAMED_LITERAL_STRING(kPropertyId,         "property_id");
  NS_NAMED_LITERAL_STRING(kMediaItemId,        "media_item_id");
  NS_NAMED_LITERAL_STRING(kResourceProperties, "resource_properties");
  NS_NAMED_LITERAL_STRING(kSelectAlias,        "_select");

  nsresult rv;

  if (SB_IsTopLevelProperty(mSelectPropertyID)) {
    // If the select property is a top level property, just add it to the
    // column list off the base table
    nsAutoString columnName;
    rv = SB_GetTopLevelPropertyColumn(mSelectPropertyID, columnName);
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

    // get property db id for rule property 
    PRUint32 propertyDBID;
    rv = mPropertyCache->GetPropertyDBID(mSelectPropertyID, &propertyDBID);
    NS_ENSURE_SUCCESS(rv, rv);

    // media_item_id match with left table media_item_id
    nsCOMPtr<sbISQLBuilderCriterion> criterion_media_item;
    rv = aBuilder->CreateMatchCriterionTable(aBaseTableAlias,
                                             kMediaItemId,
                                             sbISQLBuilder::MATCH_EQUALS,
                                             kSelectAlias,
                                             kMediaItemId,
                                             getter_AddRefs(criterion_media_item)); 
    NS_ENSURE_SUCCESS(rv, rv);

 
    // select.property_id match with select property db id
    nsCOMPtr<sbISQLBuilderCriterion> criterion_property_id;
    rv = aBuilder->CreateMatchCriterionLong(kSelectAlias,
                                            kPropertyId,
                                            sbISQLBuilder::MATCH_EQUALS,
                                            propertyDBID,
                                            getter_AddRefs(criterion_property_id));
    NS_ENSURE_SUCCESS(rv, rv);

    // combine the two conditions with AND
    nsCOMPtr<sbISQLBuilderCriterion> join_criterion;
    rv = aBuilder->CreateAndCriterion(criterion_media_item, 
                                      criterion_property_id, 
                                      getter_AddRefs(join_criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    // create a left outer join with these conditions, so we get null values
    // for items that do not have the property we're looking for.
    rv = aBuilder->AddJoinWithCriterion(sbISQLBuilder::JOIN_LEFT_OUTER,
                                        kResourceProperties,
                                        kSelectAlias,
                                        join_criterion);
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
  NS_NAMED_LITERAL_STRING(kLimitAlias,         "_limit");
  NS_NAMED_LITERAL_STRING(kObjSortable,        "obj_sortable");
  NS_NAMED_LITERAL_STRING(kPropertyId,         "property_id");
  NS_NAMED_LITERAL_STRING(kMediaItemId,        "media_item_id");
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
                               kMediaItemId,
                               aBaseTableAlias,
                               kMediaItemId);
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 propertyId;
        rv = mPropertyCache->GetPropertyDBID(NS_LITERAL_STRING(SB_PROPERTY_DURATION),
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
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

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

  char guidChars[NSID_LENGTH];
  id.ToProvidedString(guidChars);

  nsString guid(NS_ConvertASCIItoUTF16(nsDependentCString(guidChars,
                                                          NSID_LENGTH - 1)));

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
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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

  mozilla::MonitorAutoLock monitor(mConditionsMonitor);

  sbStringMap map;
  PRBool success = map.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  // Set up the defaults
  mMatchType = sbILocalDatabaseSmartMediaList::MATCH_TYPE_ANY;
  mLimitType = sbILocalDatabaseSmartMediaList::LIMIT_TYPE_NONE;
  mLimit = 0;
  mSelectPropertyID.Truncate();
  mSelectDirection = PR_TRUE;
  mRandomSelection = PR_FALSE;
  mAutoUpdate = false;
  mConditions.Clear();

  nsAutoString state;
  rv = mItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_SMARTMEDIALIST_STATE), 
                          state);
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

  if (map.Get(NS_LITERAL_STRING("selectPropertyID"), &value)) {
    mSelectPropertyID = value;
  }

  if (map.Get(NS_LITERAL_STRING("selectDirection"), &value)) {
    mSelectDirection = value.EqualsLiteral("1");
  }

  if (map.Get(NS_LITERAL_STRING("randomSelection"), &value)) {
    mRandomSelection = value.EqualsLiteral("1");
  }

  if (map.Get(NS_LITERAL_STRING("autoUpdate"), &value)) {
    PR_sscanf(NS_LossyConvertUTF16toASCII(value).get(), "%d", &mAutoUpdate);
  }

  map.Get(NS_LITERAL_STRING("sourceLibraryGuid"), &mSourceLibraryGuid);

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
        nsAutoString displayUnit;
        nsAutoString opString;

        if (conditionMap.Get(NS_LITERAL_STRING("property"), &value)) {
          property = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("leftValue"), &value)) {
          leftValue = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("rightValue"), &value)) {
          rightValue = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("displayUnit"), &value)) {
          displayUnit = value;
        }

        if (conditionMap.Get(NS_LITERAL_STRING("operator"), &value)) {
          opString = value;
        }

        if (!property.IsEmpty() && !opString.IsEmpty()) {

          sbRefPtrCondition condition;
          condition = new sbLocalDatabaseSmartMediaListCondition(property,
                                                                 opString,
                                                                 leftValue,
                                                                 rightValue,
                                                                 displayUnit);
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

  success = map.Put(NS_LITERAL_STRING("selectPropertyID"),
                    mSelectPropertyID);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString selectDirection;
  selectDirection.AppendLiteral(mSelectDirection ? "1" : "0");
  success = map.Put(NS_LITERAL_STRING("selectDirection"), selectDirection);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString randomSelection;
  randomSelection.AppendLiteral(mRandomSelection ? "1" : "0");
  success = map.Put(NS_LITERAL_STRING("randomSelection"), randomSelection);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString autoUpdate;
  autoUpdate.AppendInt(mAutoUpdate);
  success = map.Put(NS_LITERAL_STRING("autoUpdate"), autoUpdate);
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

  success = map.Put(NS_LITERAL_STRING("sourceLibraryGuid"), mSourceLibraryGuid);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsAutoString state;
  rv = JoinStringMapIntoQueryString(map, state);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_SMARTMEDIALIST_STATE), 
                          state);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the 'last updated' property when the smart media list rules have been
  // changed - this is more in line with the semantics than simply when the
  // contents of the list have changed (due to it being recomputed, for example)
  sbAutoString now((PRUint64)(PR_Now()/PR_MSEC_PER_SEC));
  rv = mItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_UPDATED),
                          now);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbIMediaListListener
NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnItemAdded(sbIMediaList* aMediaList,
                                           sbIMediaItem* aMediaItem,
                                           PRUint32 aIndex,
                                           PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                   sbIMediaItem* aMediaItem,
                                                   PRUint32 aIndex,
                                                   PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

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
    
    nsCOMPtr<sbILocalDatabaseSimpleMediaList> ldsml =
      do_QueryInterface(mList, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                  sbIMediaItem* aMediaItem,
                                                  PRUint32 aIndex,
                                                  PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnItemUpdated(sbIMediaList* aMediaList,
                                             sbIMediaItem* aMediaItem,
                                             sbIPropertyArray* aProperties,
                                             PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnItemMoved(sbIMediaList* aMediaList,
                                           PRUint32 aFromIndex,
                                           PRUint32 aToIndex,
                                           PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBeforeListCleared(sbIMediaList* aMediaList,
                                                   PRBool aExcludeLists,
                                                   PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnListCleared(sbIMediaList* aMediaList,
                                             PRBool aExcludeLists,
                                             PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care
  *aNoMoreForBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBatchBegin(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::OnBatchEnd(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  // Don't care
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetName(const nsAString& aName)
{
  if (!mList) return NS_ERROR_NULL_POINTER;
  
  // Create a property array to hold the changed property
  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // read the old property
  nsAutoString oldName;
  rv = mList->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME), oldName);
  NS_ENSURE_SUCCESS(rv, rv);

  // store it in the array
  rv = properties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME), oldName);
  NS_ENSURE_SUCCESS(rv, rv);

  // set the name to the inner list
  mList->SetName(aName);

  // notify that the name property changed for this list as well. this is so
  // listeners that are watching for list name changes also get notified about
  // us, and not just our inner list, since there is no way to go from the
  // inner list to the outer one.
  rv = mLocalDatabaseLibrary->NotifyListenersItemUpdated(this, properties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetName(nsAString& aName)
{
  if (!mList) return NS_ERROR_NULL_POINTER;

  // always return the name from the inner list, it is doing the localization
  return mList->GetName(aName);
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

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::AddSmartMediaListListener(sbILocalDatabaseSmartMediaListListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  mozilla::MonitorAutoLock monitor(mListenersMonitor);

  // Do not add duplicate listener.
  if (mListeners.IndexOfObject(aListener) == -1)
    mListeners.AppendObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::RemoveSmartMediaListListener(sbILocalDatabaseSmartMediaListListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  mozilla::MonitorAutoLock monitor(mListenersMonitor);
  mListeners.RemoveObject(aListener);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::SetSourceLibraryGuid(const nsAString& aGUID) {

  mozilla::MonitorAutoLock monitor(mSourceMonitor);

  mSourceLibraryGuid = aGUID;
  
  nsresult rv = WriteConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::GetSourceLibraryGuid(nsAString &retVal) {

  mozilla::MonitorAutoLock monitor(mSourceMonitor);

  nsString libraryguid = mSourceLibraryGuid;

  if (libraryguid.IsEmpty()) {
    nsCOMPtr<sbILibraryManager> libraryManager;
    nsresult rv;
    
    libraryManager = 
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<sbILibrary> mainLib;
    rv = libraryManager->GetMainLibrary(getter_AddRefs(mainLib));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mainLib->GetGuid(libraryguid);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  retVal = libraryguid;
  
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSmartMediaList::Observe(nsISupports *aObject,
                                       const char *aTopic,
                                       const PRUnichar *aData) {
  TRACE(("%s: observing %s", __FUNCTION__, aTopic));
  if (strcmp(aTopic, LIBRARY_MANAGER_BEFORE_SHUTDOWN) == 0) {
    // Unregister ourselves from the library so we don't leak in odd cases
    // See bug 14896 for more information
    nsCOMPtr<sbILibrary> library;
    nsresult rv = mItem->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaList> libraryList = do_QueryInterface(library, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryList->RemoveListener(this);
    // Unregister ourselves as we're done
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService(OBSERVER_SERVICE_CONTRACT_ID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = observerService->RemoveObserver(this, LIBRARY_MANAGER_BEFORE_SHUTDOWN);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Problem removing shutdown notification observer");
  }
  return NS_OK;
}
