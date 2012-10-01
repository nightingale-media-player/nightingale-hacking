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

/** 
* \file  DatabaseQuery.h
* \brief Songbird Database Object Definition.
*/

#ifndef __DATABASE_QUERY_H__
#define __DATABASE_QUERY_H__

// FORWARD REFERENCES =========================================================
class CDatabaseQuery;

// INCLUDES ===================================================================
#include <string>
#include <vector>
#include <deque>
#include <set>

#include <sqlite3.h>

#include <prlock.h>
#include <prmon.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsTArray.h>
#include <nsStringGlue.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsHashKeys.h>

#include "sbIDatabaseQuery.h"
#include "sbIDatabasePreparedStatement.h"
#include "DatabaseResult.h"

// DEFINES ====================================================================
#define SONGBIRD_DATABASEQUERY_CONTRACTID                 \
  "@songbirdnest.com/Songbird/DatabaseQuery;1"
#define SONGBIRD_DATABASEQUERY_CLASSNAME                  \
  "Songbird Database Query Interface"
#define SONGBIRD_DATABASEQUERY_CID                        \
{ /* 377a6592-64bd-4a6b-a941-d488abb5a8aa */              \
  0x377a6592,                                             \
  0x64bd,                                                 \
  0x4a6b,                                                 \
  {0xa9, 0x41, 0xd4, 0x88, 0xab, 0xb5, 0xa8, 0xaa}        \
}
// CLASSES ====================================================================
typedef enum {
  ISNULL,
  UTF8STRING,
  STRING,
  DOUBLE,
  INTEGER32,
  INTEGER64
} ParameterType;

struct CQueryParameter
{
  CQueryParameter() :
    type(ISNULL),
    doubleValue(0),
    int32Value(0),
    int64Value(0) {
  }

  ~CQueryParameter() {
  }

  CQueryParameter(const CQueryParameter &queryParameter) :
    type(queryParameter.type),
    utf8StringValue(queryParameter.utf8StringValue),
    stringValue(queryParameter.stringValue),
    doubleValue(queryParameter.doubleValue),
    int32Value(queryParameter.int32Value),
    int64Value(queryParameter.int64Value) {
  }
  
  ParameterType type;
  nsCString utf8StringValue;
  nsString stringValue;
  double doubleValue;
  PRInt32 int32Value;
  PRInt64 int64Value;
};

typedef std::vector< CQueryParameter > bindParameterArray_t;

class CDatabaseEngine;
class QueryProcessorThread;
class nsIEventTarget;
class nsIURI;
class sbIDatabaseEngine;

class CDatabaseQuery : public sbIDatabaseQuery,
                       public nsIClassInfo
{
friend class CDatabaseEngine;
friend class QueryProcessorThread;

public:
  CDatabaseQuery();
  virtual ~CDatabaseQuery();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASEQUERY
  NS_DECL_NSICLASSINFO

  nsresult Init();

  /**
   * Additional accessor to get the database location as a 
   * string spec.  This is used when an nsIURI isn't needed
   * and/or the caller is off of the main thread.
   */
  nsresult GetDatabaseLocation(nsACString& aURISpec);

protected:
  CDatabaseResult* GetResultObject();
  void SetResultObject(CDatabaseResult *aResultObject);

  nsresult PopQuery(sbIDatabasePreparedStatement **_retval);
  bindParameterArray_t* GetQueryParameters(PRUint32 aQueryIndex);
  bindParameterArray_t* PopQueryParameters();

  PRLock *m_pLock;

  nsCString m_LocationURIString;

  PRBool m_IsAborting;
  PRBool m_IsExecuting;
  PRBool m_AsyncQuery;

  PRUint32 m_CurrentQuery;
  PRInt32 m_LastError;

  nsRefPtr<CDatabaseResult> m_QueryResult;
  nsString m_DatabaseGUID;
  std::deque< nsCOMPtr<sbIDatabasePreparedStatement> > m_DatabaseQueryList;

  PRMonitor* m_pQueryRunningMonitor;
  PRBool m_QueryHasCompleted;

  nsInterfaceHashtableMT<nsISupportsHashKey, sbIDatabaseSimpleQueryCallback> m_CallbackList;
  std::deque< bindParameterArray_t > m_BindParameters;

  PRUint64 m_RollingLimit;
  PRUint32 m_RollingLimitColumnIndex;
  PRUint32 m_RollingLimitResult;

  nsCOMPtr<sbIDatabaseEngine> mDatabaseEngine;

private:
  nsresult EnsureLastQueryParameter(PRUint32 aParamIndex);
};

#endif // __DATABASE_QUERY_H__

