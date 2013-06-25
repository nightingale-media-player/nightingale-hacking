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

/**
 * \file DatabaseEngine.cpp
 * \brief
 */

// INCLUDES ===================================================================
#include "DatabaseEngine.h"
#include "DatabasePreparedStatement.h"

#include <nsCOMPtr.h>
#include <nsIAppStartup.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsStringGlue.h>
#include <nsIObserverService.h>
#include <nsISimpleEnumerator.h>
#include <nsDirectoryServiceDefs.h>
#include <nsAppDirectoryServiceDefs.h>
#include <nsDirectoryServiceUtils.h>
#include <nsUnicharUtils.h>
#include <nsIURI.h>
#include <nsNetUtil.h>
#include <sbLockUtils.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch.h>
#include <nsToolkitCompsCID.h>
#include <mozilla/Mutex.h>
#include <mozilla/ReentrantMonitor.h>
#include <VideoUtils.h>
#include <prsystem.h>

#include <vector>
#include <algorithm>
#include <prmem.h>
#include <prtypes.h>
#include <assert.h>
#include <prprf.h>
#include <locale.h>

#include <nsIScriptError.h>
#include <nsIConsoleService.h>
#include <nsIConsoleMessage.h>

#include <nsCOMArray.h>

#include <sbIMetrics.h>
#include <sbIPrompter.h>
#include <sbMemoryUtils.h>
#include <sbStringBundle.h>
#include <sbProxiedComponentManager.h>
#include <sbDebugUtils.h>

#if defined(_WIN32)
  #include <windows.h>
  #include <direct.h>
#else
// on UNIX, strnicmp is called strncasecmp
#define strnicmp strncasecmp
#endif

//Sometimes min is not defined.
#if !defined(min)
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define USE_SQLITE_FULL_DISK_CACHING
#define USE_SQLITE_READ_UNCOMMITTED
#define USE_SQLITE_MEMORY_TEMP_STORE
#define USE_SQLITE_BUSY_TIMEOUT
// Can not use FTS with shared cache enabled
//#define USE_SQLITE_SHARED_CACHE

///////////////////////////////////////////////////////////////////////////////
// Prefs used to control memory usage.  
//
// All prefs require a restart to take effect, and pageSize
// can only be changed before databases have been created.
//
// See http://www.sqlite.org/malloc.html for details
// 
// Also note that page cache and page size can be specified on a 
// per db basis with keys like
//  songbird.dbengine.main@library.songbirdnest.com.cacheSize

#define PREF_BRANCH_BASE                      "songbird.dbengine."
#define PREF_DB_PAGE_SIZE                     "pageSize"
#define PREF_DB_CACHE_SIZE                    "cacheSize"
#define PREF_DB_PREALLOCCACHE_SIZE            "preAllocCacheSize"
#define PREF_DB_PREALLOCSCRATCH_SIZE          "preAllocScratchSize"
#define PREF_DB_SOFT_LIMIT                    "softHeapLimit"

// These constants come from sbLocalDatabaseLibraryLoader.cpp
// Do not change these constants unless you are changing them in 
// sbLocalDatabaseLibraryLoader.cpp!
#define PREF_SCAN_COMPLETE                    "songbird.firstrun.scancomplete"
#define PREF_BRANCH_LIBRARY_LOADER            "songbird.library.loader."
#define PREF_MAIN_LIBRARY                     "songbird.library.main"
#define PREF_WEB_LIBRARY                      "songbird.library.web"
#define PREF_DOWNLOAD_LIST                    "songbird.library.download"
#define PREF_PLAYQUEUE_LIBRARY                "songbird.library.playqueue"

// These are also from sbLocalDatabaseLibraryLoader.cpp.
#define PREF_LOADER_DBGUID                    "databaseGUID"
#define PREF_LOADER_DBLOCATION                "databaseLocation"

// These are also from sbLocalDatabaseLibraryLoader.cpp.
#define DBENGINE_GUID_MAIN_LIBRARY            "main@library.songbirdnest.com"
#define DBENGINE_GUID_WEB_LIBRARY             "web@library.songbirdnest.com"
#define DBENGINE_GUID_PLAYQUEUE_LIBRARY       "playqueue@library.songbirdnest.com"

// Unless prefs state otherwise, allow 250 Mbytes
// of page cache.  WOW!
#define DEFAULT_PAGE_SIZE             16384
#define DEFAULT_CACHE_SIZE            16000

// pre-allocated for caching
#define DEFAULT_PREALLOCCACHE_SIZE    0
// pre-allocated for scratch memory
#define DEFAULT_PREALLOCSCRATCH_SIZE  0

#define SQLITE_MAX_RETRIES            666
#define MAX_BUSY_RETRY_CLOSE_DB       10

// Uncomment or define this to enable perf logging.
//#define USE_PERF_LOGGING 1

// Idle Service Timeout we use before checking if we should run ANALYZE.
#define IDLE_SERVICE_TIMEOUT    (5 * 60)
// Query Count Threshold for running ANALYZE
#define ANALYZE_QUERY_THRESHOLD (800)

#if defined(_DEBUG) || defined(DEBUG)
  #if defined(XP_WIN)
    #include <windows.h>
  #endif
  #define HARD_SANITY_CHECK             1
#endif

#ifdef USE_PERF_LOGGING

class sbDatabaseEnginePerformanceLogger
{
public:
  sbDatabaseEnginePerformanceLogger(const nsAString& aQuery,
                                    const nsAString& aGuid);
  ~sbDatabaseEnginePerformanceLogger();

private:
  nsString mQuery;
  nsString mGuid;
  PRTime mStart;
};

#define BEGIN_PERFORMANCE_LOG(_strQuery, _dbName) \
 sbDatabaseEnginePerformanceLogger _performanceLogger(_strQuery, _dbName)
#else
#define BEGIN_PERFORMANCE_LOG(_strQuery, _dbName) /* nothing */
#endif

#define NS_FINAL_UI_STARTUP_CATEGORY   "final-ui-startup"

/*
 * Parse a path string in the form of "n1.n2.n3..." where n is an integer.
 * Returns the next integer in the list while advancing the pos pointer to
 * the start of the next integer.  If the end of the list is reached, pos
 * is set to null
 */
static int tree_collate_func_next_num(const char* start,
                                      char** pos,
                                      int length,
                                      int eTextRep,
                                      int width)
{
  // If we are at the end of the string, set pos to null
  const char* end = start + length;
  if (*pos == end || *pos == NULL) {
    *pos = NULL;
    return 0;
  }

  int num = 0;
  int sign = 1;

  while (*pos < end) {

    // Extract the ASCII value of the digit from the encoded byte(s)
    char c;
    switch(eTextRep) {
      case SQLITE_UTF16BE:
        c = *(*pos + 1);
        break;
      case SQLITE_UTF16LE:
      case SQLITE_UTF8:
        c = **pos;
        break;
      default:
        return 0;
    }

    // If we see a period, we're done.  Also advance the pointer so the next
    // call starts on the first digit
    if (c == '.') {
      *pos += width;
      break;
    }

    // If we encounter a hyphen, treat this as a negative number.  Otherwise,
    // include the digit in the current number
    if (c == '-') {
      sign = -1;
    }
    else {
      num = (num * 10) + (c - '0');
    }
    *pos += width;
  }

  return num * sign;
}

static int tree_collate_func(void *pCtx,
                             int nA,
                             const void *zA,
                             int nB,
                             const void *zB,
                             int eTextRep)
{
  const char* cA = (const char*) zA;
  const char* cB = (const char*) zB;
  char* pA = (char*) cA;
  char* pB = (char*) cB;

  int width = eTextRep == SQLITE_UTF8 ? 1 : 2;

  /*
   * Compare each number in each string until either the numbers are different
   * or we run out of numbers to compare
   */
  int a = tree_collate_func_next_num(cA, &pA, nA, eTextRep, width);
  int b = tree_collate_func_next_num(cB, &pB, nB, eTextRep, width);

  while (pA != NULL && pB != NULL) {
    if (a != b) {
      return a < b ? -1 : 1;
    }
    a = tree_collate_func_next_num(cA, &pA, nA, eTextRep, width);
    b = tree_collate_func_next_num(cB, &pB, nB, eTextRep, width);
  }

  /*
   * At this point, if the lengths are the same, the strings are the same
   */
  if (nA == nB) {
    return 0;
  }

  /*
   * Otherwise, the longer string is always smaller
   */
  return nA > nB ? -1 : 1;
}

static int tree_collate_func_utf16be(void *pCtx,
                                     int nA,
                                     const void *zA,
                                     int nB,
                                     const void *zB)
{
  return tree_collate_func(pCtx, nA, zA, nB, zB, SQLITE_UTF16BE);
}

static int tree_collate_func_utf16le(void *pCtx,
                                     int nA,
                                     const void *zA,
                                     int nB,
                                     const void *zB)
{
  return tree_collate_func(pCtx, nA, zA, nB, zB, SQLITE_UTF16LE);
}

static int tree_collate_func_utf8(void *pCtx,
                                  int nA,
                                  const void *zA,
                                  int nB,
                                  const void *zB)
{
  return tree_collate_func(pCtx, nA, zA, nB, zB, SQLITE_UTF8);
}

// these functions are necessary because we cannot rely on the libc versions on
// unix or mac due to the discrepency between moz and libc's wchar_t sizes

int native_wcslen(const NATIVE_CHAR_TYPE *s) {
  const NATIVE_CHAR_TYPE *p = s;
  while (*p) p++;
  return p-s;
}

int native_wcscmp(const NATIVE_CHAR_TYPE *s1, const NATIVE_CHAR_TYPE *s2) {
  while (*s1 == *s2++)
    if (*s1++ == 0)
      return (0);
  return (*s1 - *(s2 - 1));
}

static PRInt32 gLocaleCollationEnabled = PR_TRUE;

/*
 * Perform collation for current locale. Data always comes in as native wide
 * chars, ie: utf16 on windows and mac, ucs4 on linux (note that on linux
 * and mac, sizeof(wchar_t)=2 eventhough the libc's native encoding is ucs4)
 *
 * IMPORTANT NOTE: Whenever the algorithm for library_collate is changed and
 * yields a different sort order than before for *any* set of arbitrary strings,
 * a new migration step must be added to issue the "reindex 'library_collate'"
 * sql query. THIS IS OF UTMOST IMPORTANCE because otherwise, the library's
 * index can become entirely trashed!
 *
 */
static int library_collate_func(collationBuffers *cBuffers,
                                const NATIVE_CHAR_TYPE *zA,
                                const NATIVE_CHAR_TYPE *zB)
{
  // shortcut when both strings are empty
  if ((zA && !*zA) &&
      (zB && !*zB))
    return 0;

  // if no dbengine service or if collation is disabled, just do a C compare

  CDatabaseEngine *db = gLocaleCollationEnabled ? gEngine : nsnull;
  
  if (!db) {
    return native_wcscmp(zA, zB);
  }

  return db->Collate(cBuffers, zA, zB);
}

inline void swap_utf16_bytes(const void *aStr,
                             int len) {
  char *d = (char *)aStr;
  char t;
  for (int i=0;i<len;i++) {
    t = *d;
    *d = *(d+1);
    d++;
    *d = t;
    d++;
  }
}

static int library_collate_func_utf16be(void *pCtx,
                                        int nA,
                                        const void *zA,
                                        int nB,
                                        const void *zB)
{
  collationBuffers *cBuffers = reinterpret_cast<collationBuffers *>(pCtx);
  if (!cBuffers) 
    return 0;
  
  // copy to our own string in order to zero terminate and being able to swap
  // the utf16 bytes if needed. note that we copy a utf16 string here regardless
  // of the native encoding.
  cBuffers->encodingConversionBuffer1.copy_utf16((const UTF16_CHARTYPE *)zA, nA);
  cBuffers->encodingConversionBuffer2.copy_utf16((const UTF16_CHARTYPE *)zB, nB);

  #ifdef LITTLEENDIAN

  // utf16 came as big endian, swap bytes
  swap_utf16_bytes(staticbuffer1a, nA);
  swap_utf16_bytes(staticbuffer1b, nB);

  #endif // ifdef LITTLEENDIAN

  #if defined(XP_UNIX) && !defined(XP_MACOSX)
  
  // on linux, native char is not utf16, we need to convert to ucs4
  
  glong size;
  NATIVE_CHAR_TYPE *a = 
    (NATIVE_CHAR_TYPE *)g_utf16_to_ucs4(
      (gunichar2 *)cBuffers->encodingConversionBuffer1.buffer(),
      (glong)nA,
      NULL,
      &size,
      NULL);
  a[size] = 0;

  NATIVE_CHAR_TYPE *b = 
    (NATIVE_CHAR_TYPE *)g_utf16_to_ucs4(
      (gunichar2 *)cBuffers->encodingConversionBuffer2.buffer(),
      (glong)nB,
      NULL,
      &size,
      NULL);
  b[size] = 0;
  
  #else // XP_UNIX && !XP_MACOSX

  // on mac and windows, utf16 is native, so we just use the buffer as is
  
  const NATIVE_CHAR_TYPE *a = cBuffers->encodingConversionBuffer1.buffer();
  const NATIVE_CHAR_TYPE *b = cBuffers->encodingConversionBuffer1.buffer();
  
  #endif
  
  int r = library_collate_func(cBuffers, a, b);
  
  #if defined(XP_UNIX) && !defined(XP_MACOSX)
  
  // free the temporary strings allocated by the utf16 to ucs4 conversion
  
  g_free(a);
  g_free(b);
  
  #endif
  
  return r;
}
                        
static int library_collate_func_utf16le(void *pCtx,
                                        int nA,
                                        const void *zA,
                                        int nB,
                                        const void *zB)
{
  collationBuffers *cBuffers = reinterpret_cast<collationBuffers *>(pCtx);
  if (!cBuffers) 
    return 0;
  
  // copy to our own string in order to zero terminate and being able to swap
  // the utf16 bytes if needed. note that we copy a utf16 string here regardless
  // of the native encoding.
  cBuffers->encodingConversionBuffer1.copy_utf16((const UTF16_CHARTYPE *)zA, nA);
  cBuffers->encodingConversionBuffer2.copy_utf16((const UTF16_CHARTYPE *)zB, nB);

  #ifdef BIGENDIAN

  // utf16 came as little endian, swap bytes
  swap_utf16_bytes(staticbuffer1a, nA);
  swap_utf16_bytes(staticbuffer1b, nB);

  #endif // ifdef LITTLEENDIAN

  #if defined(XP_UNIX) && !defined(XP_MACOSX)
  
  // on linux, native char is not utf16, we need to convert to ucs4
  
  glong size;
  NATIVE_CHAR_TYPE *a = 
    (NATIVE_CHAR_TYPE *)g_utf16_to_ucs4(
      (gunichar2 *)cBuffers->encodingConversionBuffer1.buffer(),
      (glong)nA,
      NULL,
      &size,
      NULL);
  a[size] = 0;

  NATIVE_CHAR_TYPE *b = 
    (NATIVE_CHAR_TYPE *)g_utf16_to_ucs4(
      (gunichar2 *)cBuffers->encodingConversionBuffer2.buffer(),
      (glong)nB,
      NULL,
      &size,
      NULL);
  b[size] = 0;
  
  #else // XP_UNIX && !XP_MACOSX
  
  // on mac and windows, utf16 is native, so we just use the buffer as is
  
  const NATIVE_CHAR_TYPE *a = cBuffers->encodingConversionBuffer1.buffer();
  const NATIVE_CHAR_TYPE *b = cBuffers->encodingConversionBuffer1.buffer();
  
  #endif
  
  int r = library_collate_func(cBuffers, a, b);
  
  #if defined(XP_UNIX) && !defined(XP_MACOSX)
  
  // free the temporary strings allocated by the utf16 to ucs4 conversion
  
  g_free(a);
  g_free(b);
  
  #endif
  
  return r;
}

static int library_collate_func_utf8(void *pCtx,
                                     int nA,
                                     const void *zA,
                                     int nB,
                                     const void *zB)
{
  collationBuffers *cBuffers = reinterpret_cast<collationBuffers *>(pCtx);
  if (!cBuffers) 
    return 0;

  // we first convert to the native character encoding so that we can perform
  // the entire collation algorithm (which requires splitting the strings into
  // substrings) without doing anymore conversions.

  // strlen(utf8) * sizeof(NATIVE_CHAR_TYPE) is always large enough to hold
  // the result of the utf8 to utf16/ucs4 conversion, so no need to call the OS
  // for the desired size first.

  cBuffers->encodingConversionBuffer1.grow_native(nA);
  cBuffers->encodingConversionBuffer2.grow_native(nB);
  
  NATIVE_CHAR_TYPE *a;
  NATIVE_CHAR_TYPE *b;

#ifdef XP_MACOSX

  a = cBuffers->encodingConversionBuffer1.buffer();
  b = cBuffers->encodingConversionBuffer2.buffer();

  CFStringRef cA = CFStringCreateWithBytes(NULL, 
                                           (const UInt8*)zA, 
                                           nA, 
                                           kCFStringEncodingUTF8, 
                                           false);
  CFStringRef cB = CFStringCreateWithBytes(NULL, 
                                           (const UInt8*)zB, 
                                           nB, 
                                           kCFStringEncodingUTF8, 
                                           false);

  CFStringGetCharacters(cA, CFRangeMake(0, CFStringGetLength(cA)), a);
  CFStringGetCharacters(cB, CFRangeMake(0, CFStringGetLength(cB)), b);
  
  a[CFStringGetLength(cA)] = 0;
  b[CFStringGetLength(cB)] = 0;
  
  CFRelease(cA);
  CFRelease(cB);
  
#elif XP_UNIX

  a = (NATIVE_CHAR_TYPE *)g_utf8_to_ucs4((const gchar *)zA, 
                                          nA, 
                                          NULL, 
                                          NULL, 
                                          NULL);
  b = (NATIVE_CHAR_TYPE *)g_utf8_to_ucs4((const gchar *)zB, 
                                          nB, 
                                          NULL, 
                                          NULL, 
                                          NULL);
  
#elif XP_WIN

  a = cBuffers->encodingConversionBuffer1.buffer();
  b = cBuffers->encodingConversionBuffer2.buffer();

  PRInt32 cnA = MultiByteToWideChar(CP_UTF8, 
                                    0, 
                                    (LPCSTR)zA, 
                                    nA, 
                                    a, 
                                    cBuffers->encodingConversionBuffer1
                                             .bufferLength());

  PRInt32 cnB = MultiByteToWideChar(CP_UTF8, 
                                    0, 
                                    (LPCSTR)zB, 
                                    nB, 
                                    b, 
                                    cBuffers->encodingConversionBuffer2
                                             .bufferLength());
  a[cnA] = 0;
  b[cnB] = 0;
  
#endif

  int r = library_collate_func(cBuffers,
                               (const NATIVE_CHAR_TYPE *)a, 
                               (const NATIVE_CHAR_TYPE *)b);

#if defined(XP_UNIX) && !defined(XP_MACOSX)

  g_free(a);
  g_free(b);

#endif

  return r;
}

//-----------------------------------------------------------------------------
/* Sqlite Dump Helper Class */
//-----------------------------------------------------------------------------
class CDatabaseDumpProcessor : public nsIRunnable 
{
public:
  CDatabaseDumpProcessor(CDatabaseEngine *aCallback,
                         QueryProcessorQueue *aQueryProcessorQueue,
                         nsIFile *aOutputFile);
  virtual ~CDatabaseDumpProcessor();
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

protected:
  nsresult OutputBuffer(const char *aBuffer);
  PRInt32 RunSchemaDumpQuery(const nsACString & aQuery);
  PRInt32 RunTableDumpQuery(const nsACString & aSelect);

  static int DumpCallback(void *pArg, int inArg, 
                          char **azArg, const char **azCol);
  static char *appendText(char *zIn, char const *zAppend, 
                          char quote);

protected:
  nsCOMPtr<nsIFileOutputStream>  mOutputStream;
  nsCOMPtr<nsIFile>              mOutputFile;
  nsRefPtr<CDatabaseEngine>      mEngineCallback;
  nsRefPtr<QueryProcessorQueue>  mQueryProcessorQueue;
  PRBool                         writeableSchema;  // true if PRAGMA writable_schema=on
};


NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseDumpProcessor, nsIRunnable)

CDatabaseDumpProcessor::CDatabaseDumpProcessor(CDatabaseEngine *aCallback,
                                               QueryProcessorQueue *aQueryProcessorQueue,
                                               nsIFile *aOutputFile)
: mOutputFile(aOutputFile)
, mEngineCallback(aCallback)
, mQueryProcessorQueue(aQueryProcessorQueue)
{
}

CDatabaseDumpProcessor::~CDatabaseDumpProcessor()
{
}

NS_IMETHODIMP
CDatabaseDumpProcessor::Run()
{
  nsresult rv;
  mOutputStream = 
    do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mOutputStream->Init(mOutputFile, -1, 0600, 0);
  NS_ENSURE_SUCCESS(rv, rv);

  {
    mozilla::MutexAutoLock handleLock(mQueryProcessorQueue->m_pHandleLock);
    
    // First query, 'schema' dump
    PRInt32 rc;
    nsCString schemaDump;    
    schemaDump.AppendLiteral("SELECT name, type, sql FROM sqlite_master "
                             "WHERE sql NOT NULL and type=='table'");
    rc = RunSchemaDumpQuery(schemaDump);
    if (rc != SQLITE_OK) {
      return NS_ERROR_FAILURE;
    }

    // Next query, 'table' dump
    nsCString tableDump;
    tableDump.AppendLiteral("SELECT sql FROM sqlite_master "
                            "WHERE sql NOT NULL AND type IN ('index', 'trigger', 'view')");
    rc = RunTableDumpQuery(tableDump);
    if (rc != SQLITE_OK) {
      return NS_ERROR_FAILURE;
    }
  }

  return NS_OK;
}

nsresult
CDatabaseDumpProcessor::OutputBuffer(const char *aBuffer)
{
  NS_ENSURE_ARG_POINTER(aBuffer);
  
  nsresult rv = NS_OK;
  nsCString buffer(aBuffer);
  if (buffer.Length() > 0) {
    PRUint32 writeCount;
    rv = mOutputStream->Write(buffer.get(), buffer.Length(), &writeCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

PRInt32
CDatabaseDumpProcessor::RunSchemaDumpQuery(const nsACString & aQuery)
{
  // Run |aQuery|. Use |DumpCallback()| as the callback routine so that
  // the contents of the query are output as SQL statements.
  //
  // If we get a SQLITE_CORRUPT error, rerun the query after appending
  // "ORDER BY rowid DESC" to the end.
  nsCString query(aQuery);
  PRInt32 rc = sqlite3_exec(mQueryProcessorQueue->m_pHandle,
                            query.get(),
                            (sqlite3_callback)DumpCallback,
                            this,
                            0);
  if (rc == SQLITE_CORRUPT) {
    char *zQ2 = (char *)malloc(query.Length() + 100);
    if (zQ2 == 0) {
      return rc;
    }

    sqlite3_snprintf(sizeof(zQ2), zQ2, "%s ORDER BY rowid DESC", query.get());
    rc = sqlite3_exec(mQueryProcessorQueue->m_pHandle,
                      zQ2,
                      (sqlite3_callback)DumpCallback,
                      this,
                      0);
    free(zQ2);
  }

  return rc;
}

PRInt32
CDatabaseDumpProcessor::RunTableDumpQuery(const nsACString & aSelect)
{
  nsCString select(aSelect);
  sqlite3_stmt *pSelect;
  int rc = sqlite3_prepare(mQueryProcessorQueue->m_pHandle,
                           select.get(),
                           -1,
                           &pSelect,
                           0);
  if (rc != SQLITE_OK || !pSelect) {
    return rc;
  }

  nsresult rv;
  rc = sqlite3_step(pSelect);
  while (rc == SQLITE_ROW) {
    rv = OutputBuffer((const char *)sqlite3_column_text(pSelect, 0));
    NS_ENSURE_SUCCESS(rv, rv);
    rv = OutputBuffer(";\n");
    NS_ENSURE_SUCCESS(rv, rv);

    rc = sqlite3_step(pSelect);
  }

  return sqlite3_finalize(pSelect);
}

/* static */ int 
CDatabaseDumpProcessor::DumpCallback(void *pArg, 
                                     int inArg, 
                                     char **azArg, 
                                     const char **azCol)
{
  // This callback routine is used for dumping the database.
  // Each row received by this callback consists of a table name.
  // The table type ("index" or "table") and SQL to create the table.
  // This routine should print text sufficient to recreate the table.
  int rc;
  const char *zTable;
  const char *zType;
  const char *zSql;
  CDatabaseDumpProcessor *dumpProcessor = (CDatabaseDumpProcessor *)pArg;
  
  if (inArg != 3) 
    return 1;
  
  zTable = azArg[0];
  zType = azArg[1];
  zSql = azArg[2];
  
  if (strcmp(zTable, "sqlite_sequence") == 0) {
    dumpProcessor->OutputBuffer("DELETE FROM sqlite_sequence;\n");
  }
  else if (strcmp(zTable, "sqlite_stat1") == 0) {
    dumpProcessor->OutputBuffer("ANALYZE sqlite_master;\n");
  }
  else if (strncmp(zTable, "sqlite_", 7) == 0) {
    return 0;
  }
  else if (strncmp(zSql, "CREATE VIRTUAL TABLE", 20) == 0) {
    char *zIns;
    if (!dumpProcessor->writeableSchema) {
      dumpProcessor->OutputBuffer("PRAGMA writable_schema=ON;\n");
      dumpProcessor->writeableSchema = PR_TRUE;
    }
    zIns = sqlite3_mprintf(
       "INSERT INTO sqlite_master(type,name,tbl_name,rootpage,sql)"
       "VALUES('table','%q','%q',0,'%q');",
       zTable, zTable, zSql);
    dumpProcessor->OutputBuffer(zIns);
    dumpProcessor->OutputBuffer("\n");
    sqlite3_free(zIns);
    return 0;
  }
  else {
    dumpProcessor->OutputBuffer(zSql);
    dumpProcessor->OutputBuffer(";\n");
  }

  if (strcmp(zType, "table") == 0) {
    sqlite3_stmt *pTableInfo = 0;
    char *zSelect = 0;
    char *zTableInfo = 0;
    char *zTmp = 0;
   
    zTableInfo = appendText(zTableInfo, "PRAGMA table_info(", 0);
    zTableInfo = appendText(zTableInfo, zTable, '"');
    zTableInfo = appendText(zTableInfo, ");", 0);

    rc = sqlite3_prepare(dumpProcessor->mQueryProcessorQueue->m_pHandle, 
                         zTableInfo, -1, &pTableInfo, 0);
    if (zTableInfo) { 
      free(zTableInfo);
    }
    if (rc != SQLITE_OK || !pTableInfo) {
      return 1;
    }

    zSelect = appendText(zSelect, "SELECT 'INSERT INTO ' || ", 0);
    zTmp = appendText(zTmp, zTable, '"');
    if (zTmp) {
      zSelect = appendText(zSelect, zTmp, '\'');
    }
    zSelect = appendText(zSelect, " || ' VALUES(' || ", 0);
    rc = sqlite3_step(pTableInfo);
    while (rc == SQLITE_ROW) {
      const char *zText = (const char *)sqlite3_column_text(pTableInfo, 1);
      zSelect = appendText(zSelect, "quote(", 0);
      zSelect = appendText(zSelect, zText, '"');
      rc = sqlite3_step(pTableInfo);
      if (rc == SQLITE_ROW) {
        zSelect = appendText(zSelect, ") || ',' || ", 0);
      }
      else {
        zSelect = appendText(zSelect, ") ", 0);
      }
    }
    rc = sqlite3_finalize(pTableInfo);
    if (rc != SQLITE_OK) {
      if (zSelect) {
        free(zSelect);
      }
      return 1;
    }
    zSelect = appendText(zSelect, "|| ')' FROM  ", 0);
    zSelect = appendText(zSelect, zTable, '"');

    rc = dumpProcessor->RunTableDumpQuery(nsDependentCString(zSelect));
    if (rc == SQLITE_CORRUPT) {
      zSelect = appendText(zSelect, " ORDER BY rowid DESC", 0);
      rc = dumpProcessor->RunTableDumpQuery(nsDependentCString(zSelect));
    }
    if (zSelect) {
      free(zSelect);
    }
  }
  return 0;
}

/* static */ char* 
CDatabaseDumpProcessor::appendText(char *zIn, 
                                   char const *zAppend, 
                                   char quote)
{
  // zIn is either a pointer to a NULL-terminated string in memory obtained
  // from |malloc()|, or a NULL pointer. The string pointed to by zAppend
  // is added to zIn, and the result returned in memory obtained from |malloc()|.
  // zIn, if it was not NULL, is freed.
  int len;
  int i;
  int nAppend = strlen(zAppend);
  int nIn = (zIn ? strlen(zIn) : 0);

  len = nAppend + nIn + 1;
  if (quote) {
    len += 2;
    for (i = 0; i < nAppend; i++) {
      if (zAppend[i] == quote) {
        len++;
      }
    }
  }

  zIn = (char *)realloc(zIn, len);
  if (!zIn) {
    return 0;
  }

  if (quote) {
    char *zCsr = &zIn[nIn];
    *zCsr++ = quote;
    for (i = 0; i < nAppend; i++) {
      *zCsr++ = zAppend[i];
      if (zAppend[i] == quote) {
        *zCsr++ = quote;
      }
    }
    *zCsr++ = quote;
    *zCsr++ = '\0';
    assert((zCsr - zIn) == len);
  }
  else{
    memcpy(&zIn[nIn], zAppend, nAppend);
    zIn[len-1] = '\0';
  }

  return zIn;
}

//-----------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS2(CDatabaseEngine, sbIDatabaseEngine, nsIObserver)
NS_IMPL_THREADSAFE_ISUPPORTS1(QueryProcessorQueue, nsIRunnable)

CDatabaseEngine *gEngine = nsnull;

// CLASSES ====================================================================
//-----------------------------------------------------------------------------
CDatabaseEngine::CDatabaseEngine()
: m_pDBStorePathLock(nsnull)
, m_pThreadMonitor("CDatabaseEngine:m_pThreadMonitor")
, m_CollationBuffersMapMonitor("CDatabaseEngine::m_CollationBuffersMapMonitor")
, m_AttemptShutdownOnDestruction(PR_FALSE)
, m_IsShutDown(PR_FALSE)
, m_MemoryConstraintsSet(PR_FALSE)
, m_PromptForDelete(PR_FALSE)
, m_DeleteDatabases(PR_FALSE)
, m_AddedIdleObserver(PR_FALSE)
, m_pPageSpace(nsnull)
, m_pScratchSpace(nsnull)
#ifdef XP_MACOSX
, m_Collator(nsnull)
#endif
{
  SB_PRLOG_SETUP(sbDatabaseEngine);
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CDatabaseEngine::~CDatabaseEngine()
{
  if (m_AttemptShutdownOnDestruction)
    Shutdown();
  
  if (m_MemoryConstraintsSet) {
    if (m_pPageSpace) {
      NS_Free(m_pPageSpace);
    }
    if (m_pScratchSpace) {
      NS_Free(m_pScratchSpace);
    }
  }
} //dtor

//-----------------------------------------------------------------------------
CDatabaseEngine* CDatabaseEngine::GetSingleton()
{
  if (gEngine) {
    NS_ADDREF(gEngine);
    return gEngine;
  }

  gEngine = new CDatabaseEngine;
  if (!gEngine)
    return nsnull;

  // AddRef once for us (released in nsModule destructor)
  NS_ADDREF(gEngine);

  // Set ourselves up properly
  if (NS_FAILED(gEngine->Init())) {
    NS_ERROR("Failed to Init CDatabaseEngine!");
    NS_RELEASE(gEngine);
    return nsnull;
  }

  // And AddRef once for the caller
  NS_ADDREF(gEngine);
  return gEngine;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Init()
{
  LOG("CDatabaseEngine[0x%.8x] - Init() - sqlite version %s",
       this, sqlite3_libversion());

  PRBool success = m_QueuePool.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = CreateDBStorePath();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to create db store folder in profile!");

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if(NS_SUCCEEDED(rv)) {
    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                      PR_FALSE);

    rv = observerService->AddObserver(this, NS_FINAL_UI_STARTUP_CATEGORY,
                                      PR_FALSE);
  }

  // This shouldn't be an 'else' case because we want to set this flag if
  // either of the above calls failed
  if(NS_FAILED(rv)) {
    NS_ERROR("Unable to register xpcom-shutdown observer");
    m_AttemptShutdownOnDestruction = PR_TRUE;
  }
  
  // Select a collation locale for the entire lifetime of the app, so that
  // it cannot change on us.
  rv = GetCurrentCollationLocale(mCollationLocale);

#ifdef XP_MACOSX

  LocaleRef l;
  ::LocaleRefFromLocaleString(mCollationLocale.get(), &l);
  
  ::UCCreateCollator(l,
                     kUnicodeCollationClass, 
                     kUCCollateStandardOptions |
                       kUCCollatePunctuationSignificantMask,
                     &m_Collator);
#else
  setlocale(LC_COLLATE, mCollationLocale.get());
#endif

  m_pThreadPool = do_CreateInstance("@mozilla.org/thread-pool;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = m_pThreadPool->SetThreadLimit(4);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = m_pThreadPool->SetIdleThreadLimit(1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = m_pThreadPool->SetIdleThreadTimeout(30000);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIdleService> idleService = 
    do_GetService("@mozilla.org/widget/idleservice;1", &rv);
  
  if(NS_SUCCEEDED(rv)) {
    rv = idleService->AddIdleObserver(this, IDLE_SERVICE_TIMEOUT);
    m_AddedIdleObserver = NS_SUCCEEDED(rv) ? PR_TRUE : PR_FALSE;
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
PR_STATIC_CALLBACK(PLDHashOperator)
EnumQueuesOperate(nsStringHashKey::KeyType aKey, QueryProcessorQueue *aQueue, void *aClosure)
{
  NS_ASSERTION(aQueue, "aQueue is null");
  NS_ASSERTION(aClosure, "aClosure is null");

  // Stop if thread is null.
  NS_ENSURE_TRUE(aQueue, PL_DHASH_STOP);

  // Stop if closure is null because it
  // contains the operation we are going to perform.
  NS_ENSURE_TRUE(aClosure, PL_DHASH_STOP);

  nsresult rv;
  PRUint32 *op = static_cast<PRUint32 *>(aClosure);

  switch(*op) {
    case CDatabaseEngine::dbEnginePreShutdown:
      rv = aQueue->PrepareForShutdown();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to prepare worker thread for shutdown.");
    break;

    case CDatabaseEngine::dbEngineShutdown:
      rv = aQueue->Shutdown();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to shut down processor queue.");
    break;

    default:
      ;
  }

  return PL_DHASH_NEXT;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Shutdown()
{
  m_IsShutDown = PR_TRUE;

  PRUint32 op = dbEnginePreShutdown;
  m_QueuePool.EnumerateRead(EnumQueuesOperate, &op);

  op = dbEngineShutdown;
  m_QueuePool.EnumerateRead(EnumQueuesOperate, &op);

  m_QueuePool.Clear();
  
  nsresult rv = m_pThreadPool->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);
  
  if(m_AddedIdleObserver) {
    nsCOMPtr<nsIIdleService> idleService = 
      do_GetService("@mozilla.org/widget/idleservice;1", &rv);
    if(NS_SUCCEEDED(rv)) {
      rv = idleService->RemoveIdleObserver(this, IDLE_SERVICE_TIMEOUT);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
                       "Failed to remove idle service observer.");
    }
  }

#ifdef XP_MACOSX
  if (m_Collator)
    ::UCDisposeCollator(&m_Collator);
#endif

  if(m_PromptForDeleteTimer) {
    rv = m_PromptForDeleteTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    m_PromptForDeleteTimer = nsnull;
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Observe(nsISupports *aSubject,
                                       const char *aTopic,
                                       const PRUnichar *aData)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  if(!strcmp(aTopic, NS_FINAL_UI_STARTUP_CATEGORY)) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, NS_FINAL_UI_STARTUP_CATEGORY);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Remove Observer Failed!");

    mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);
    if(m_PromptForDelete) {
      mozilla::ReentrantMonitorAutoExit mon(m_pThreadMonitor);

      rv = PromptToDeleteDatabases();
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Prompting to Delete Databases Failed!");
    }

    m_PromptForDeleteTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(!strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC)) {
    mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);
    if(m_PromptForDelete) {
      {
        mozilla::ReentrantMonitorAutoExit mon(m_pThreadMonitor);

        rv = PromptToDeleteDatabases();
        NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Prompting to Delete Databases Failed!");
      }
    }
  }
  else if(!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Remove Observer Failed!");

    // Shutdown our threads
    rv = Shutdown();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Shutdown Failed!");

    // Delete any bad databases now
    rv = DeleteMarkedDatabases();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to delete bad databases!");
  }
  else if(!strcmp(aTopic, "idle")) {
    rv = RunAnalyze();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to run ANALYZE on Databases.");
  }

  return NS_OK;
}



//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::InitMemoryConstraints()
{
  if (m_MemoryConstraintsSet) 
    return NS_ERROR_ALREADY_INITIALIZED;
    
  nsresult rv = NS_OK;

  PRInt32 preAllocCache;
  PRInt32 preAllocScratch;
  PRInt32 softLimit;
  PRInt32 pageSize;
  
  // Load values from the pref system
  nsCOMPtr<nsIPrefService> prefService =
     do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(PREF_BRANCH_BASE, getter_AddRefs(prefBranch));
  if (NS_FAILED(rv) || 
      NS_FAILED(prefBranch->GetIntPref(PREF_DB_PREALLOCCACHE_SIZE,
                                       &preAllocCache))) {
    NS_WARNING("DBEngine failed to get preAllocCache pref. Using default.");
    preAllocCache = DEFAULT_PREALLOCCACHE_SIZE; 
  }
  if (NS_FAILED(rv) ||
      NS_FAILED(prefBranch->GetIntPref(PREF_DB_PREALLOCSCRATCH_SIZE,
                                       &preAllocScratch))) {
    NS_WARNING("DBEngine failed to get preAllocScratch pref. Using default.");
    preAllocScratch = DEFAULT_PREALLOCSCRATCH_SIZE; 
  }
  if (NS_FAILED(rv) ||NS_FAILED(prefBranch->GetIntPref(PREF_DB_SOFT_LIMIT,
                                       &softLimit))) {
    NS_WARNING("DBEngine failed to get soft heap limit pref. Using default.");
    softLimit = 0; 
  }
  if (NS_FAILED(rv) || NS_FAILED(prefBranch->GetIntPref(PREF_DB_PAGE_SIZE,
                                                        &pageSize))) {
    NS_WARNING("DBEngine failed to get page size pref. Using default.");
    pageSize = DEFAULT_PAGE_SIZE; 
  }

  PRInt32 ret;
  
  if (preAllocCache > 0) {
    m_pPageSpace = NS_Alloc(pageSize * preAllocCache);
    if (!m_pPageSpace) {
       return NS_ERROR_OUT_OF_MEMORY;
    }
    ret = sqlite3_config(SQLITE_CONFIG_PAGECACHE, m_pPageSpace,
                         pageSize, preAllocCache);
    NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_FAILURE);
  }

  if (preAllocScratch > 0) {
    // http://www.sqlite.org/malloc.html recommends slots 6x page size,
    // with as many slots as there are threads.
    PRInt32 scratchSlotSize = pageSize * 6; 
    m_pScratchSpace = NS_Alloc(scratchSlotSize * preAllocScratch);  
    if (!m_pScratchSpace) {
       return NS_ERROR_OUT_OF_MEMORY;
    }
    ret = sqlite3_config(SQLITE_CONFIG_SCRATCH, m_pScratchSpace,
                         scratchSlotSize, preAllocScratch);
    NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_FAILURE);
  }
  
  // Try not to use more than X memory...
  if (softLimit > 0) {
    sqlite3_soft_heap_limit(softLimit);
  }
  
  // Only allow init to happen once
  m_MemoryConstraintsSet = PR_TRUE;

  return rv;
}

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::GetDBPrefs(const nsAString &dbGUID,
                                     PRInt32 *cacheSize, 
                                     PRInt32 *pageSize)
{
  nsresult rv = NS_OK;
  
  nsCOMPtr<nsIPrefService> prefService =
     do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefService->GetBranch(PREF_BRANCH_BASE, getter_AddRefs(prefBranch));

  if (NS_FAILED(rv) || NS_FAILED(prefBranch->GetIntPref(PREF_DB_CACHE_SIZE,
                                                        cacheSize))) {
    NS_WARNING("DBEngine failed to get cache size pref. Using default.");
    *cacheSize = DEFAULT_CACHE_SIZE; 
  }
  
  if (NS_FAILED(rv) || NS_FAILED(prefBranch->GetIntPref(PREF_DB_PAGE_SIZE,
                                                        pageSize))) {
    NS_WARNING("DBEngine failed to get page size pref. Using default.");
    *pageSize = DEFAULT_PAGE_SIZE; 
  }
  
  // Now try for values that are specific to this database guid
  // e.g. songbird.dbengine.main@library.songbirdnest.com.cacheSize
  nsCString dbBranch(PREF_BRANCH_BASE);
  dbBranch.Append(NS_ConvertUTF16toUTF8(dbGUID));
  dbBranch.Append(NS_LITERAL_CSTRING("."));
  if (NS_SUCCEEDED(prefService->GetBranch(dbBranch.get(), 
          getter_AddRefs(prefBranch)))) {
    prefBranch->GetIntPref(PREF_DB_CACHE_SIZE, cacheSize);
    prefBranch->GetIntPref(PREF_DB_PAGE_SIZE, pageSize);    
  }

  return rv;
}

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::OpenDB(const nsAString &dbGUID,
                                 CDatabaseQuery *pQuery,
                                 sqlite3 ** ppHandle)
{
  sqlite3 *pHandle = nsnull;

  nsAutoString strFilename;
  GetDBStorePath(dbGUID, pQuery, strFilename);

#if defined(USE_SQLITE_SHARED_CACHE)
  sqlite3_enable_shared_cache(1);
#endif
  
  // Allow the user to control how much memory sqlite uses.
  if (!m_MemoryConstraintsSet) {
    if (NS_FAILED(InitMemoryConstraints())) {
      NS_WARNING("DBEngine failed to set memory usage constraints.");
    }
  }
 
  PRInt32 ret = sqlite3_open(NS_ConvertUTF16toUTF8(strFilename).get(), &pHandle);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to open database: sqlite_open failed!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);
  
  ret  = sqlite3_create_collation(pHandle,
                                  "tree",
                                  SQLITE_UTF16BE,
                                  NULL,
                                  tree_collate_func_utf16be);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set tree collate function: utf16-be!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "tree",
                                 SQLITE_UTF16LE,
                                 NULL,
                                 tree_collate_func_utf16le);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set tree collate function: utf16-le!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "tree",
                                 SQLITE_UTF8,
                                 NULL,
                                 tree_collate_func_utf8);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set tree collate function: utf8!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  collationBuffers *collationBuffersEntry = new collationBuffers();

  {
    mozilla::ReentrantMonitorAutoEnter mon(m_CollationBuffersMapMonitor);
    m_CollationBuffersMap[pHandle] = collationBuffersEntry;
  }

  ret = sqlite3_create_collation(pHandle,
                                 "library_collate",
                                 SQLITE_UTF8,
                                 collationBuffersEntry,
                                 library_collate_func_utf8);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set library collate function: utf8!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "library_collate",
                                 SQLITE_UTF16LE,
                                 collationBuffersEntry,
                                 library_collate_func_utf16le);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set library collate function: utf16le!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "library_collate",
                                 SQLITE_UTF16BE,
                                 collationBuffersEntry,
                                 library_collate_func_utf16be);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set library collate function: utf16be!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);


  PRInt32 pageSize = DEFAULT_PAGE_SIZE;
  PRInt32 cacheSize = DEFAULT_CACHE_SIZE;
  
  if (NS_FAILED(GetDBPrefs(dbGUID, &cacheSize, &pageSize))) {
    NS_WARNING("DBEngine failed to get memory prefs. Using default.");
  }

  nsCString query;
  
  {
    char *strErr = nsnull;
    query = NS_LITERAL_CSTRING("PRAGMA page_size = ");
    query.AppendInt(pageSize);
    sqlite3_exec(pHandle, query.get(), nsnull, nsnull, &strErr);
    if(strErr) {
      NS_WARNING(strErr);
      sqlite3_free(strErr);
    }
  }

  {
    char *strErr = nsnull;
    query = NS_LITERAL_CSTRING("PRAGMA cache_size = ");
    query.AppendInt(cacheSize);
    sqlite3_exec(pHandle, query.get(), nsnull, nsnull, &strErr);
    if(strErr) {
      NS_WARNING(strErr);
      sqlite3_free(strErr);
    }
  }

#if defined(USE_SQLITE_FULL_DISK_CACHING)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA synchronous = 0", nsnull, nsnull, &strErr);
    if(strErr) {
      NS_WARNING(strErr);
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_READ_UNCOMMITTED)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA read_uncommitted = 1", nsnull, nsnull, &strErr);
    if(strErr) {
      NS_WARNING(strErr);
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_MEMORY_TEMP_STORE)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA temp_store = 2", nsnull, nsnull, &strErr);
    if(strErr) {
      NS_WARNING(strErr);
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_BUSY_TIMEOUT)
  sqlite3_busy_timeout(pHandle, 120000);
#endif

  *ppHandle = pHandle;

  return NS_OK;
} //OpenDB

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::CloseDB(sqlite3 *pHandle)
{
  PRInt32 retries = 0;
  PRInt32 ret = SQLITE_BUSY;

  do {
    sqlite3_interrupt(pHandle);
    if((ret = sqlite3_close(pHandle)) == SQLITE_BUSY) {
      PR_Sleep(PR_MillisecondsToInterval(50));
    }
  }
  while(ret == SQLITE_BUSY && 
        retries++ < MAX_BUSY_RETRY_CLOSE_DB);

  {
    mozilla::ReentrantMonitorAutoEnter mon(m_CollationBuffersMapMonitor);
    collationMap_t::const_iterator found = m_CollationBuffersMap.find(pHandle);
    if (found != m_CollationBuffersMap.end()) {
      delete found->second;
      m_CollationBuffersMap.erase(pHandle);
    }
  }

  NS_ASSERTION(ret == SQLITE_OK, "");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  return NS_OK;
} //CloseDB

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::CloseDatabase(const nsAString &aDatabaseGUID) 
{
  mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);

  nsRefPtr<QueryProcessorQueue> pQueue;
  if(m_QueuePool.Get(aDatabaseGUID, getter_AddRefs(pQueue))) {
    
    nsresult rv = pQueue->PrepareForShutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = pQueue->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    m_QueuePool.Remove(aDatabaseGUID);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* [noscript] PRInt32 SubmitQuery (in CDatabaseQueryPtr dbQuery); */
NS_IMETHODIMP CDatabaseEngine::SubmitQuery(CDatabaseQuery * dbQuery, PRInt32 *_retval)
{
  if (m_IsShutDown) {
    NS_WARNING("Don't submit queries after the DBEngine is shut down!");
    return NS_ERROR_FAILURE;
  }

  *_retval = SubmitQueryPrivate(dbQuery);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::SubmitQueryPrivate(CDatabaseQuery *pQuery)
{
  //Query is null, bail.
  if(!pQuery) {
    NS_WARNING("A null queury was submitted to the database engine");
    return 1;
  }

  //Grip.
  NS_ADDREF(pQuery);

  // If the query is already executing, do not add it.  This is to prevent
  // the same query from getting executed simultaneously
  PRBool isExecuting = PR_FALSE;
  pQuery->IsExecuting(&isExecuting);
  if(isExecuting) {
    //Release grip.
    NS_RELEASE(pQuery);
    return 0;
  }

  nsRefPtr<QueryProcessorQueue> pQueue = GetQueueByQuery(pQuery, PR_TRUE);
  NS_ENSURE_TRUE(pQueue, 1);

  nsresult rv = pQueue->PushQueryToQueue(pQuery);
  NS_ENSURE_SUCCESS(rv, 1);

  {
    sbSimpleAutoLock lock(pQuery->m_pLock);
    pQuery->m_IsExecuting = PR_TRUE;
  }

  rv = pQueue->RunQueue();
  NS_ENSURE_SUCCESS(rv, 1);

  PRBool bAsyncQuery = PR_FALSE;
  pQuery->IsAyncQuery(&bAsyncQuery);

  PRInt32 result = 0;
  if(!bAsyncQuery) {
    pQuery->WaitForCompletion(&result);
    pQuery->GetLastError(&result);
  }

  return result;
} //SubmitQueryPrivate

//-----------------------------------------------------------------------------
NS_IMETHODIMP
CDatabaseEngine::DumpDatabase(const nsAString & aDatabaseGUID, nsIFile *aOutFile)
{
  NS_ENSURE_ARG_POINTER(aOutFile);

  nsRefPtr<CDatabaseQuery> dummyQuery = new CDatabaseQuery();
  NS_ENSURE_TRUE(dummyQuery, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = dummyQuery->SetDatabaseGUID(aDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dummyQuery->Init();
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsRefPtr<QueryProcessorQueue> pQueue = GetQueueByQuery(dummyQuery, PR_TRUE);
  NS_ENSURE_TRUE(pQueue, NS_ERROR_FAILURE);

  nsRefPtr<CDatabaseDumpProcessor> dumpProcessor = 
    new CDatabaseDumpProcessor(this, pQueue, aOutFile);

  return dumpProcessor->Run();
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::DumpMemoryStatistics()
{
  int current    = -1;
  int highwater  = -1;

  printf("DumpMemoryStatistics() format\tCurrent\tHighwater\n");

  sqlite3_status(SQLITE_STATUS_MEMORY_USED, &current, &highwater, 0);
  printf("SQLITE_STATUS_MEMORY_USED:\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_PAGECACHE_USED, &current, &highwater, 0);
  printf("SQLITE_STATUS_PAGECACHE_USED:\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_PAGECACHE_OVERFLOW, &current, &highwater, 0);
  printf("SQLITE_STATUS_PAGECACHE_OVERFLOW:\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_SCRATCH_USED, &current, &highwater, 0);
  printf("SQLITE_STATUS_SCRATCH_USED:\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_SCRATCH_OVERFLOW, &current, &highwater, 0);
  printf("SQLITE_STATUS_SCRATCH_OVERFLOW:\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_MALLOC_SIZE, &current, &highwater, 0);
  printf("SQLITE_STATUS_MALLOC_SIZE\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_PARSER_STACK, &current, &highwater, 0);
  printf("SQLITE_STATUS_PARSER_STACK\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_PAGECACHE_SIZE, &current, &highwater, 0);
  printf("SQLITE_STATUS_PAGECACHE_SIZE\t%d\t%d\n", current, highwater);
  sqlite3_status(SQLITE_STATUS_SCRATCH_SIZE, &current, &highwater, 0);
  printf("SQLITE_STATUS_SCRATCH_SIZE\t%d\t%d\n", current, highwater);

  printf("DumpMemoryStatistics() finished.  "
         "See dbengine/src/sqlite3.h#6168\n");

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::GetCurrentMemoryUsage(PRInt32 flag, PRInt32 *_retval)
{
  int disused  = -1;
  sqlite3_status((int)flag, (int*)_retval, &disused, 0);
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::GetHighWaterMemoryUsage(PRInt32 flag, PRInt32 *_retval)
{
  int disused  = -1;
  sqlite3_status((int)flag, &disused, (int*)_retval, 0);
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::ReleaseMemory()
{
  // Attempt to free a large amount of memory.
  // This will cause SQLite to free as much as it can.
  int SB_UNUSED_IN_RELEASE(memReleased) = sqlite3_release_memory(500000000);
  LOG("CDatabaseEngine::ReleaseMemory() managed to release %d bytes\n", memReleased);

  return NS_OK;
}

//-----------------------------------------------------------------------------
already_AddRefed<QueryProcessorQueue> CDatabaseEngine::GetQueueByQuery(CDatabaseQuery *pQuery,
                                                         PRBool bCreate /*= PR_FALSE*/)
{
  NS_ENSURE_TRUE(pQuery, nsnull);

  nsAutoString strGUID;
  mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);

  nsRefPtr<QueryProcessorQueue> pQueue;

  nsresult rv = pQuery->GetDatabaseGUID(strGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  if(!m_QueuePool.Get(strGUID, getter_AddRefs(pQueue))) {
    pQueue = CreateQueueFromQuery(pQuery);
  }

  NS_ENSURE_TRUE(pQueue, nsnull);

  QueryProcessorQueue *p = pQueue.get();
  NS_ADDREF(p);

  return p;
}

//-----------------------------------------------------------------------------
already_AddRefed<QueryProcessorQueue> CDatabaseEngine::CreateQueueFromQuery(CDatabaseQuery *pQuery)
{
  nsAutoString strGUID;
  mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);

  nsresult rv = pQuery->GetDatabaseGUID(strGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsRefPtr<QueryProcessorQueue> pQueue(new QueryProcessorQueue());
  NS_ENSURE_TRUE(pQueue, nsnull);

  sqlite3 *pHandle = nsnull;
  rv = OpenDB(strGUID, pQuery, &pHandle);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = pQueue->Init(this, strGUID, pHandle);
  NS_ENSURE_SUCCESS(rv, nsnull);

  PRBool success = m_QueuePool.Put(strGUID, pQueue);
  NS_ENSURE_TRUE(success, nsnull);

  QueryProcessorQueue *p = pQueue.get();
  NS_ADDREF(p);

  return p;
}

nsresult
CDatabaseEngine::MarkDatabaseForPotentialDeletion(const nsAString &aDatabaseGUID,
                                                  CDatabaseQuery *pQuery)
{
  mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);

  m_PromptForDelete = PR_TRUE;
  m_DatabasesToDelete.insert(std::make_pair(
      nsString(aDatabaseGUID), nsRefPtr<CDatabaseQuery>(pQuery)));

  if(m_PromptForDeleteTimer) {
    nsresult rv = m_PromptForDeleteTimer->Init(this, nsITimer::TYPE_ONE_SHOT, 100);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult 
CDatabaseEngine::PromptToDeleteDatabases() 
{
  nsresult rv;

  {
    mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);
    if(!m_PromptForDelete || m_DatabasesToDelete.empty()) {
      return NS_OK;
    }
  }

  nsCOMPtr<sbIPrompter> promptService =
    do_GetService(SONGBIRD_PROMPTER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 buttons = nsIPromptService::BUTTON_POS_0 * nsIPromptService::BUTTON_TITLE_IS_STRING + 
                     nsIPromptService::BUTTON_POS_1 * nsIPromptService::BUTTON_TITLE_IS_STRING + 
                     nsIPromptService::BUTTON_POS_1_DEFAULT;
  PRInt32 promptResult = 0;

  // get dialog strings
  sbStringBundle bundle;
  nsString dialogTitle = bundle.Get("corruptdatabase.dialog.title");
  nsString dialogText = bundle.Get("corruptdatabase.dialog.text");
  nsString deleteText = bundle.Get("corruptdatabase.dialog.buttons.delete");
  nsString continueText = bundle.Get("corruptdatabase.dialog.buttons.cancel");


  // prompt.
  rv = promptService->ConfirmEx(nsnull,
                                dialogTitle.BeginReading(),
                                dialogText.BeginReading(),
                                buttons,            
                                deleteText.BeginReading(),   // button 0
                                continueText.BeginReading(), // button 1
                                nsnull,                      // button 2
                                nsnull,                      // no checkbox
                                nsnull,                      // no check value
                                &promptResult);     
  NS_ENSURE_SUCCESS(rv, rv);

  {
    mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);
    m_PromptForDelete = PR_FALSE;
  }

  // "Delete" means delete & restart.  "Continue" means let the app
  // start anyway.
  if (promptResult == 0) { 
    // metric: user chose to delete corrupt library
    nsCOMPtr<sbIMetrics> metrics =
      do_CreateInstance("@songbirdnest.com/Songbird/Metrics;1", &rv);
    
    if(NS_SUCCEEDED(rv)) {
      rv = metrics->MetricsInc(NS_LITERAL_STRING("app"), \
                               NS_LITERAL_STRING("library.error.reset"), 
                               EmptyString());
      NS_ENSURE_SUCCESS(rv, rv);
    }

    {
      mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);
      m_DeleteDatabases = PR_TRUE;
    }

    // now attempt to quit/restart.
    nsCOMPtr<nsIAppStartup> appStartup = 
      (do_GetService(NS_APPSTARTUP_CONTRACTID, &rv));
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = appStartup->Quit(nsIAppStartup::eForceQuit | nsIAppStartup::eRestart);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
CDatabaseEngine::DeleteMarkedDatabases()
{
  nsresult rv;
  nsCOMPtr<nsIPrefService> prefService =
    do_GetService(NS_PREFSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);

  if(!m_DeleteDatabases)
    return NS_OK;

  deleteDatabaseMap_t::const_iterator cit = m_DatabasesToDelete.begin();
  deleteDatabaseMap_t::const_iterator citEnd = m_DatabasesToDelete.end();

  for(; cit != citEnd; ++cit) {
    nsString strFilename;
    GetDBStorePath(cit->first, cit->second, strFilename);

    nsCOMPtr<nsILocalFile> databaseFile;
    rv = NS_NewLocalFile(strFilename, 
                         PR_FALSE, 
                         getter_AddRefs(databaseFile));
    if(NS_FAILED(rv)) {
      NS_WARNING("Failed to get local file for database!");
      continue;
    }
    
    rv = databaseFile->Remove(PR_FALSE);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to delete corrupted database file!");
  }

  // Go through prefs branch. If the databaseGUID pref matches
  // the database we are deleting, delete the entire branch.
  
  // If the db guid is the magic main library guid, we will also
  // reset the pref that asks the user to import media on startup.

  nsCAutoString prefBranchRoot(PREF_BRANCH_LIBRARY_LOADER);
  nsCOMPtr<nsIPrefBranch> loaderPrefBranch;

  rv = prefService->GetBranch(prefBranchRoot.get(), getter_AddRefs(loaderPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 libraryKeysCount;
  char** libraryKeys;

  rv = loaderPrefBranch->GetChildList("", &libraryKeysCount, &libraryKeys);
  NS_ENSURE_SUCCESS(rv, rv);

  sbAutoFreeXPCOMArray<char**> autoFree(libraryKeysCount, libraryKeys);

  for (PRUint32 index = 0; index < libraryKeysCount; index++) {
    nsCString pref(libraryKeys[index]);

    PRInt32 firstDotIndex = pref.FindChar('.');
    // bad pref string format, skip
    if(firstDotIndex == -1) {
      continue;
    }

    PRUint32 keyLength = firstDotIndex;
    if(keyLength == 0) {
      continue;
    }

    // Should be something like "songbird.library.loader.1.".
    nsCString branchString(PREF_BRANCH_LIBRARY_LOADER);
    branchString += Substring(pref, 0, keyLength + 1);
    if(!StringEndsWith(branchString, NS_LITERAL_CSTRING("."))) {
      continue;
    }

    nsCOMPtr<nsIPrefBranch> innerBranch;
    rv = prefService->GetBranch(branchString.get(), getter_AddRefs(innerBranch));
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 prefType = nsIPrefBranch::PREF_INVALID;
    rv = innerBranch->GetPrefType(PREF_LOADER_DBGUID, &prefType);
    NS_ENSURE_SUCCESS(rv, rv);

    if(prefType != nsIPrefBranch::PREF_STRING) {
      continue;
    }

    nsCString loaderDbGuid;
    rv = innerBranch->GetCharPref(PREF_LOADER_DBGUID, getter_Copies(loaderDbGuid));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = innerBranch->GetPrefType(PREF_LOADER_DBLOCATION, &prefType);
    NS_ENSURE_SUCCESS(rv, rv);

    if(prefType != nsIPrefBranch::PREF_STRING) {
      continue;
    }

    nsCString loaderDbLocation;
    rv = innerBranch->GetCharPref(PREF_LOADER_DBLOCATION, getter_Copies(loaderDbLocation));
    NS_ENSURE_SUCCESS(rv, rv);
    
    deleteDatabaseMap_t::const_iterator citD = 
      m_DatabasesToDelete.find(NS_ConvertUTF8toUTF16(loaderDbGuid));

    if(citD != m_DatabasesToDelete.end()) {
      nsString strFilename;
      GetDBStorePath(citD->first, citD->second, strFilename);

      if(strFilename.EqualsLiteral(loaderDbLocation.get())) {
        rv = innerBranch->DeleteBranch("");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = prefService->SavePrefFile(nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if(loaderDbGuid.EqualsLiteral(DBENGINE_GUID_MAIN_LIBRARY)) {
        nsCOMPtr<nsIPrefBranch> doomedBranch;
        rv = prefService->GetBranch(PREF_SCAN_COMPLETE, getter_AddRefs(doomedBranch));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = doomedBranch->DeleteBranch("");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = prefService->GetBranch(PREF_MAIN_LIBRARY, getter_AddRefs(doomedBranch));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = doomedBranch->DeleteBranch("");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = prefService->GetBranch(PREF_DOWNLOAD_LIST, getter_AddRefs(doomedBranch));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = doomedBranch->DeleteBranch("");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = prefService->SavePrefFile(nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if(loaderDbGuid.EqualsLiteral(DBENGINE_GUID_WEB_LIBRARY)) {
        nsCOMPtr<nsIPrefBranch> doomedBranch;
        rv = prefService->GetBranch(PREF_WEB_LIBRARY, getter_AddRefs(doomedBranch));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = doomedBranch->DeleteBranch("");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = prefService->SavePrefFile(nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if(loaderDbGuid.EqualsLiteral(DBENGINE_GUID_PLAYQUEUE_LIBRARY)) {
        nsCOMPtr<nsIPrefBranch> doomedBranch;
        rv = prefService->GetBranch(PREF_PLAYQUEUE_LIBRARY,
                                    getter_AddRefs(doomedBranch));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = doomedBranch->DeleteBranch("");
        NS_ENSURE_SUCCESS(rv, rv);

        rv = prefService->SavePrefFile(nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  m_DatabasesToDelete.clear();
  m_DeleteDatabases = PR_FALSE;

  return NS_OK;
}

template<class T>
PLDHashOperator CDatabaseEngine::EnumerateIntoArrayStringKey(
                                   const nsAString& aKey,
                                   T* aQueue,
                                   void* aArray)
{
  nsTArray<nsString> *stringArray = 
    reinterpret_cast< nsTArray<nsString>* >(aArray);
  
  // No need to lock the queue here since it's access to m_QueuePool is locked
  // when we are enumerating it.

  if(aQueue->m_AnalyzeCount > ANALYZE_QUERY_THRESHOLD) {
    aQueue->m_AnalyzeCount = 0;
    stringArray->AppendElement(aKey);
  }

  return PL_DHASH_NEXT;
}

nsresult 
CDatabaseEngine::RunAnalyze()
{
  nsTArray<nsString> dbGUIDs;
  {
    mozilla::ReentrantMonitorAutoEnter mon(m_pThreadMonitor);
    m_QueuePool.EnumerateRead(EnumerateIntoArrayStringKey, &dbGUIDs);
  }

  nsresult rv = NS_ERROR_UNEXPECTED;

  PRUint32 current = 0;
  PRUint32 length  = dbGUIDs.Length();

  for(; current < length; current++) {
    nsRefPtr<CDatabaseQuery> query;

    query = new CDatabaseQuery;
    if(!query)
      continue;
    rv = query->SetDatabaseGUID(dbGUIDs[current]);
    if(NS_FAILED(rv))
      continue;
    rv = query->AddQuery(NS_LITERAL_STRING("ANALYZE"));
    if(NS_FAILED(rv))
      continue;
    rv = query->SetAsyncQuery(PR_TRUE);
    if(NS_FAILED(rv))
      continue;
    rv = SubmitQueryPrivate(query);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to submit ANALYZE query.");
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CDatabaseEngine::QueryProcessor(CDatabaseEngine* pEngine,
                                                            QueryProcessorQueue *pQueue)
{  
  if(!pEngine ||
     !pQueue ) {
    NS_WARNING("Called QueryProcessor without an engine or thread!!!!");
    return;
  }

  CDatabaseQuery *pQuery = nsnull;

  while(PR_TRUE)
  {
    pQuery = nsnull;

    { // Enter Monitor
      // Wrap any calls that access the pQueue.m_Queue because they cause a
      // context switch between threads and can mess up the link between the
      // RunQueue() call and the GetQueueSize() here. See bug 6514 for more details.
      mozilla::ReentrantMonitorAutoEnter mon(pQueue->m_pQueueMonitor);
      pQueue->m_Running = PR_FALSE;

      PRUint32 queueSize = 0;
      nsresult rv = pQueue->GetQueueSize(queueSize);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get queue size.");

      // Nothing to execute
      if(!queueSize || pQueue->m_Shutdown) {
        return;
      }

      // We must have an item in the queue
      rv = pQueue->PopQueryFromQueue(&pQuery);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "No query to pop from queue.");

      if(NS_FAILED(rv)) {
        return;
      }

      pQueue->m_Running = PR_TRUE;
      pQueue->m_AnalyzeCount++;
    } // Exit Monitor

    {
      //The query is now in a running state.
      mozilla::ReentrantMonitorAutoEnter mon(pQuery->m_pQueryRunningMonitor);

      sqlite3 *pDB = pQueue->m_pHandle;

      LOG("DBE: Process Start, thread 0x%x query 0x%x",
        PR_GetCurrentThread(), pQuery);

      PRUint32 nQueryCount = 0;
      PRBool bFirstRow = PR_TRUE;

      //Default return error.
      pQuery->SetLastError(SQLITE_ERROR);
      pQuery->GetQueryCount(&nQueryCount);

      // Create a result set object
      nsRefPtr<CDatabaseResult> databaseResult =
        new CDatabaseResult(pQuery->m_AsyncQuery);
      
      // Out of memory, attempt to restart the thread
      if(NS_UNLIKELY(!databaseResult)) {
        mozilla::ReentrantMonitorAutoEnter mon(pQueue->m_pQueueMonitor);
        pQueue->m_Running = PR_FALSE;
        return;
      }

      for(PRUint32 currentQuery = 0; currentQuery < nQueryCount && !pQuery->m_IsAborting; ++currentQuery)
      {
        nsAutoPtr<bindParameterArray_t> pParameters;

        int retDB = 0; // sqlite return code.

        nsCOMPtr<sbIDatabasePreparedStatement> preparedStatement;
        nsresult rv = pQuery->PopQuery(getter_AddRefs(preparedStatement));
        if (NS_FAILED(rv)) {
          LOG("DBE: Failed to get a prepared statement from the Query object.");
          continue;
        }
        nsString strQuery;
        preparedStatement->GetQueryString(strQuery);
        // cast the prepared statement to its C implementation. this is a really lousy thing to do to an interface pointer.
        // since it mostly prevents ever being able to provide an alternative implementation.
        CDatabasePreparedStatement *actualPreparedStatement =
          static_cast<CDatabasePreparedStatement*>(preparedStatement.get());
        sqlite3_stmt *pStmt =
          actualPreparedStatement->GetStatement(pQueue->m_pHandle);

        if (!pStmt) {
          LOG("DBE: Failed to create a prepared statement from the Query object.");
          continue;
        }

        PR_Lock(pQuery->m_pLock);
        pQuery->m_CurrentQuery = currentQuery;
        PR_Unlock(pQuery->m_pLock);

        pParameters = pQuery->PopQueryParameters();

        nsAutoString dbName;
        pQuery->GetDatabaseGUID(dbName);

        BEGIN_PERFORMANCE_LOG(strQuery, dbName);

        LOG("DBE: '%s' on '%s'\n",
          NS_ConvertUTF16toUTF8(dbName).get(),
          NS_ConvertUTF16toUTF8(strQuery).get());

        // If we have parameters for this query, bind them
        PRUint32 i = 0; // we need the index as well to know where to bind our values.
        bindParameterArray_t::const_iterator const end = pParameters->end();
        for (bindParameterArray_t::const_iterator paramIter = pParameters->begin();
             paramIter != end;
             ++paramIter, ++i) {
          const CQueryParameter& p = *paramIter;

          switch(p.type) {
            case ISNULL:
              sqlite3_bind_null(pStmt, i + 1);
              LOG("DBE: Parameter %d is 'NULL'", i);
              break;
            case UTF8STRING:
              sqlite3_bind_text(pStmt, i + 1,
                p.utf8StringValue.get(),
                p.utf8StringValue.Length(),
                SQLITE_TRANSIENT);
              LOG("DBE: Parameter %d is '%s'", i, p.utf8StringValue.get());
              break;
            case STRING:
            {
              sqlite3_bind_text16(pStmt, i + 1,
                p.stringValue.get(),
                p.stringValue.Length() * sizeof(PRUnichar),
                SQLITE_TRANSIENT);
               LOG("DBE: Parameter %d is '%s'", i, NS_ConvertUTF16toUTF8(p.stringValue).get());
              break;
            }
            case DOUBLE:
              sqlite3_bind_double(pStmt, i + 1, p.doubleValue);
              LOG("DBE: Parameter %d is '%f'", i, p.doubleValue);
              break;
            case INTEGER32:
              sqlite3_bind_int(pStmt, i + 1, p.int32Value);
              LOG("DBE: Parameter %d is '%d'", i, p.int32Value);
              break;
            case INTEGER64:
              sqlite3_bind_int64(pStmt, i + 1, p.int64Value);
              LOG("DBE: Parameter %d is '%ld'", i, p.int64Value);
              break;
          }
        }

        PRInt32 totalRows = 0;

        PRUint64 rollingSum = 0;
        PRUint64 rollingLimit = 0;
        PRUint32 rollingLimitColumnIndex = 0;
        PRUint32 rollingRowCount = 0;
        pQuery->GetRollingLimit(&rollingLimit);
        pQuery->GetRollingLimitColumnIndex(&rollingLimitColumnIndex);

        PRBool finishEarly = PR_FALSE;
        do
        {
          retDB = sqlite3_step(pStmt);

          switch(retDB)
          {
          case SQLITE_ROW:
            {
              int nCount = sqlite3_column_count(pStmt);
              if(bFirstRow)
              {
                bFirstRow = PR_FALSE;

                std::vector<nsString> vColumnNames;
                vColumnNames.reserve(nCount);

                int j = 0;
                for(; j < nCount; j++) {
                  const char *p = (const char *)sqlite3_column_name(pStmt, j);
                  if (p) {
                    vColumnNames.push_back(NS_ConvertUTF8toUTF16(p));
                  }
                  else {
                    nsAutoString strColumnName;
                    strColumnName.SetIsVoid(PR_TRUE);
                    vColumnNames.push_back(strColumnName);
                  }
                }
                databaseResult->SetColumnNames(vColumnNames);
              }

              std::vector<nsString> vCellValues;
              vCellValues.reserve(nCount);

              TRACE("DBE: Result row %d:", totalRows);

              int k = 0;
              // If this is a rolling limit query, increment the rolling
              // sum by the value of the  specified column index.
              if (rollingLimit > 0) {
                rollingSum += sqlite3_column_int64(pStmt, rollingLimitColumnIndex);
                rollingRowCount++;
              }

              // Add the row to the result only if this is not a rolling
              // limit query, or if this is a rolling limit query and the
              // rolling sum has met or exceeded the limit
              if (rollingLimit == 0 || rollingSum >= rollingLimit) {
                for(; k < nCount; k++)
                {
                  const char *p = (const char *)sqlite3_column_text(pStmt, k);
                  nsString strCellValue;
                  if (p) {
                    strCellValue = NS_ConvertUTF8toUTF16(p);
                  }
                  else {
                    strCellValue.SetIsVoid(PR_TRUE);
                  }

                  vCellValues.push_back(strCellValue);
                  TRACE("Column %d: '%s' ", k,
                    NS_ConvertUTF16toUTF8(strCellValue).get());
                }
                totalRows++;

                databaseResult->AddRow(vCellValues);

                // If this is a rolling limit query, we're done
                if (rollingLimit > 0) {
                  pQuery->SetRollingLimitResult(rollingRowCount);
                  pQuery->SetLastError(SQLITE_OK);
                  TRACE("Rolling limit query complete, %d rows", totalRows);
                  finishEarly = PR_TRUE;
                }
              }
            }
            break;

          case SQLITE_DONE:
            {
              pQuery->SetLastError(SQLITE_OK);
              TRACE("Query complete, %d rows", totalRows);
            }
          break;

          case SQLITE_BUSY:
            {
              sqlite3_reset(pStmt);
              sqlite3_sleep(50);

              retDB = SQLITE_ROW;
            }
          break;

          case SQLITE_CORRUPT:
            {
              pEngine->ReportError(pDB, pStmt);

              // Even if the following fails, this method will exit cleanly
              // and report the error to the console
              rv = pEngine->MarkDatabaseForPotentialDeletion(dbName, pQuery);
              NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to mark database for deletion!");
            }
          break;

          default:
            {
              // Log all SQL errors to the error console.
              pEngine->ReportError(pDB, pStmt);
              pQuery->SetLastError(retDB);
            }
          }
        }
        while(retDB == SQLITE_ROW &&
              !pQuery->m_IsAborting &&
              !finishEarly);

        pQuery->SetResultObject(databaseResult);

        // Quoth the sqlite wiki:
        // Sometimes people think they have finished with a SELECT statement because sqlite3_step()
        // has returned SQLITE_DONE. But the SELECT is not really complete until sqlite3_reset()
        //  or sqlite3_finalize() have been called.
        sqlite3_reset(pStmt);
      }

      //Whatever happened, the query is done running now.
      {
        sbSimpleAutoLock lock(pQuery->m_pLock);
        pQuery->m_QueryHasCompleted = PR_TRUE;
        pQuery->m_IsExecuting = PR_FALSE;
        pQuery->m_IsAborting = PR_FALSE;
      }

      LOG("DBE: Notified query monitor.");

      //Fire off the callback if there is one.
      pEngine->DoSimpleCallback(pQuery);
      LOG("DBE: Simple query listeners have been processed.");

      LOG("DBE: Process End");

      mon.NotifyAll();
    }

    NS_RELEASE(pQuery);
  } // while

  return;
} //QueryProcessor

already_AddRefed<nsIEventTarget> 
CDatabaseEngine::GetEventTarget()
{
  NS_ENSURE_TRUE(m_pThreadPool, nsnull);
  
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIEventTarget> eventTarget = do_QueryInterface(m_pThreadPool, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  return eventTarget.forget();
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::ReportError(sqlite3* db, sqlite3_stmt* stmt) {
  const char *sql = sqlite3_sql(stmt);
  const char *errMsg = sqlite3_errmsg(db);

  nsString log;
  log.AppendLiteral("SQLite execution error: \n");
  log.Append(NS_ConvertUTF8toUTF16(sql));
  log.AppendLiteral("\nresulted in the error\n");
  log.Append(NS_ConvertUTF8toUTF16(errMsg));
  log.AppendLiteral("\n");

  nsresult rv;
  nsCOMPtr<nsIConsoleService> consoleService = do_GetService("@mozilla.org/consoleservice;1", &rv);

  nsCOMPtr<nsIScriptError> scriptError = do_CreateInstance(NS_SCRIPTERROR_CONTRACTID);
  if (scriptError) {
    nsresult rv = scriptError->Init(log.get(),
                                    EmptyString().get(),
                                    EmptyString().get(),
                                    0, // No line number
                                    0, // No column number
                                    0, // An error message.
                                    "DBEngine:StatementExecution");
    if (NS_SUCCEEDED(rv)) {
      rv = consoleService->LogMessage(scriptError);
    }
  }
}
//-----------------------------------------------------------------------------
PR_STATIC_CALLBACK(PLDHashOperator)
EnumSimpleCallback(nsISupports *key, sbIDatabaseSimpleQueryCallback *data, void *closure)
{
  nsCOMArray<sbIDatabaseSimpleQueryCallback> *array = static_cast<nsCOMArray<sbIDatabaseSimpleQueryCallback> *>(closure);
  array->AppendObject(data);
  return PL_DHASH_NEXT;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::DoSimpleCallback(CDatabaseQuery *pQuery)
{
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDatabaseSimpleQueryCallback> callbackSnapshot;

  nsCOMPtr<sbIDatabaseResult> pDBResult;
  nsAutoString strGUID;

  pQuery->GetResultObject(getter_AddRefs(pDBResult));
  pQuery->GetDatabaseGUID(strGUID);
  
  pQuery->m_CallbackList.EnumerateRead(EnumSimpleCallback, &callbackSnapshot);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount)
    return;

  nsString strQuery = NS_LITERAL_STRING("UNIMPLEMENTED");

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDatabaseSimpleQueryCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback)
    {
      try
      {
        callback->OnQueryEnd(pDBResult, strGUID, strQuery);
      }
      catch(...) { }
    }
  }

  return;
} //DoSimpleCallback

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::CreateDBStorePath()
{
  nsresult rv = NS_ERROR_FAILURE;
  sbSimpleAutoLock lock(m_pDBStorePathLock);

  nsCOMPtr<nsIFile> f;

  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(f));
  if(NS_FAILED(rv)) return rv;

  rv = f->Append(NS_LITERAL_STRING("db"));
  if(NS_FAILED(rv)) return rv;

  PRBool dirExists = PR_FALSE;
  rv = f->Exists(&dirExists);
  if(NS_FAILED(rv)) return rv;

  if(!dirExists)
  {
    rv = f->Create(nsIFile::DIRECTORY_TYPE, 0700);
    if(NS_FAILED(rv)) return rv;
  }

  rv = f->GetPath(m_DBStorePath);
  if(NS_FAILED(rv)) return rv;

  return NS_OK;
}

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::GetDBStorePath(const nsAString &dbGUID, CDatabaseQuery *pQuery, nsAString &strPath)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsILocalFile> f;
  nsCString spec;
  nsAutoString strDBFile(dbGUID);

  rv = pQuery->GetDatabaseLocation(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  if(!spec.IsEmpty())
  {
    nsCOMPtr<nsIFile> file;
    rv = NS_GetFileFromURLSpec(spec, getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString path;
    rv = file->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewLocalFile(path, PR_FALSE, getter_AddRefs(f));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    PR_Lock(m_pDBStorePathLock);
    rv = NS_NewLocalFile(m_DBStorePath, PR_FALSE, getter_AddRefs(f));
    PR_Unlock(m_pDBStorePathLock);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  strDBFile.AppendLiteral(".db");
  rv = f->Append(strDBFile);
  if(NS_FAILED(rv)) return rv;
  rv = f->GetPath(strPath);
  if(NS_FAILED(rv)) return rv;

  return NS_OK;
} //GetDBStorePath

NS_IMETHODIMP CDatabaseEngine::GetLocaleCollationEnabled(PRBool *aEnabled)
{
  NS_ENSURE_ARG_POINTER(aEnabled);
  *aEnabled = (PRBool)gLocaleCollationEnabled;
  return NS_OK;
}

NS_IMETHODIMP CDatabaseEngine::SetLocaleCollationEnabled(PRBool aEnabled)
{
  PR_AtomicSet(&gLocaleCollationEnabled, (PRInt32)aEnabled);
  return NS_OK;
}

PRInt32 CDatabaseEngine::CollateForCurrentLocale(collationBuffers *aCollationBuffers, 
                                                 const NATIVE_CHAR_TYPE *aStr1, 
                                                 const NATIVE_CHAR_TYPE *aStr2) {
  // shortcut when both strings are empty
  if (!aStr1 && !aStr2) 
    return 0;

  PRInt32 retval;

  // apply the proper collation algorithm, depending on the user's locale.
  
  // note that it is impossible to use the proper sort for *all* languages at
  // the same time, because what is proper depends on the original language from
  // which the string came. for instance, Hungarian artists should have their
  // accented vowels sorted the same as non-accented ones, but a French artist 
  // should have the last accent determine that order. because we cannot
  // possibly guess the origin locale for the string, the only thing we can do
  // is use the user's current locale on all strings.
  
  // that being said, many language-specific letters (such as the German Eszett)
  // have one one way of being properly sorted (in this instance, it must sort
  // with the same weight as 'ss', and are collated that way no matter what
  // locale is being used.

#ifdef XP_MACOSX

  // on osx, use carbon collate functions because the CRT functions do not
  // have access to carbon's collation setting.

  PRInt32 nA = native_wcslen(aStr1);
  PRInt32 nB = native_wcslen(aStr2); 

  // we should always have a collator, but just in case we don't,
  // use the default collation algorithm.
  if (!m_Collator) {
    ::UCCompareTextDefault(kUCCollateStandardOptions, 
                           aStr1,
                           nA,
                           aStr2,
                           nB, 
                           NULL,
                           (SInt32 *)&retval);
  } else {
    ::UCCompareText(m_Collator,
                    aStr1,
                    nA,
                    aStr2,
                    nB,
                    NULL,
                    (SInt32 *)&retval);
  }

#else

  retval = wcscoll((const wchar_t *)aStr1, (const wchar_t *)aStr2);

#endif // ifdef XP_MACOSX

  return retval;
}

// This defines where to sort strings with leading numbers relative to strings
// without leading numbers. (-1 = top, 1 = bottom)
#define LEADING_NUMBERS_SORTPOSITION -1

PRInt32 CDatabaseEngine::CollateWithLeadingNumbers(collationBuffers *aCollationBuffers, 
                                                   const NATIVE_CHAR_TYPE *aStr1, 
                                                   PRInt32 *number1Length,
                                                   const NATIVE_CHAR_TYPE *aStr2,
                                                   PRInt32 *number2Length) {
  PRBool hasLeadingNumberA = PR_FALSE;
  PRBool hasLeadingNumberB = PR_FALSE;
  
  PRFloat64 leadingNumberA;
  PRFloat64 leadingNumberB;
  
  SB_ExtractLeadingNumber(aStr1, 
                          &hasLeadingNumberA, 
                          &leadingNumberA, 
                          number1Length);
  SB_ExtractLeadingNumber(aStr2, 
                          &hasLeadingNumberB, 
                          &leadingNumberB, 
                          number2Length);
  
  // we want strings with leading numbers to always sort the same way relative
  // to those without leading numbers
  if (hasLeadingNumberA && !hasLeadingNumberB) {
    return LEADING_NUMBERS_SORTPOSITION;
  } else if (!hasLeadingNumberA && hasLeadingNumberB) {
    return -LEADING_NUMBERS_SORTPOSITION;
  } else if (hasLeadingNumberA && hasLeadingNumberB) {
    if (leadingNumberA > leadingNumberB) 
      return 1;
    else if (leadingNumberA < leadingNumberB)
      return -1;
  }
  
  // either both numbers are equal, or neither string had a leading number,
  // use the (possibly) stripped down strings to collate.

  aStr1 += *number1Length;
  aStr2 += *number2Length;
  
  return CollateForCurrentLocale(aCollationBuffers, aStr1, aStr2);
}

PRInt32 CDatabaseEngine::Collate(collationBuffers *aCollationBuffers,
                                 const NATIVE_CHAR_TYPE *aStr1,
                                 const NATIVE_CHAR_TYPE *aStr2) {

  const NATIVE_CHAR_TYPE *remainderA = aStr1;
  const NATIVE_CHAR_TYPE *remainderB = aStr2;

  while (1) {
    
    // if either string is empty, break the loop
    if (!*remainderA ||
        !*remainderB)
      break;
      
    // find the next number in each string, if any
    PRInt32 nextNumberPosA = SB_FindNextNumber(remainderA);
    PRInt32 nextNumberPosB = SB_FindNextNumber(remainderB);
    
    if (nextNumberPosA == -1 || 
        nextNumberPosB == -1) {
      // if either string does not have anymore number, break the loop
      // so we do a final collate on the remainder of the strings
      break;
    } else {
      // both strings still have more number(s)

      // if one of the strings begins with a number and the other does not,
      // enforce leading number sort position
      if (nextNumberPosA == 0 && nextNumberPosB != 0) {
        return LEADING_NUMBERS_SORTPOSITION;
      } else if (nextNumberPosA != 0 && nextNumberPosB == 0) {
        return -LEADING_NUMBERS_SORTPOSITION;
      }

      // extract the substrings that precede the numbers, then collate these. 
      // if they are not equivalent, then return the result of that collation.
      
      aCollationBuffers->substringExtractionBuffer1
                        .copy_native(remainderA, nextNumberPosA);
      aCollationBuffers->substringExtractionBuffer2
                        .copy_native(remainderB, nextNumberPosB);
      
      PRInt32 substringCollate =
        CollateForCurrentLocale(aCollationBuffers, 
                                aCollationBuffers->substringExtractionBuffer1.buffer(),
                                aCollationBuffers->substringExtractionBuffer2.buffer());

      if (substringCollate != 0) {
        return substringCollate;
      }
      
      // the leading substrings are equivalent, so parse the numbers and
      // see if they are equivalent too

      // remove the leading substrings from the remainder strings, so that
      // they now begin with the next numbers
      remainderA += nextNumberPosA;
      remainderB += nextNumberPosB;
      
      PRInt32 numberALength;
      PRInt32 numberBLength;
      
      PRInt32 leadingNumbersCollate = 
        CollateWithLeadingNumbers(aCollationBuffers,
                                  remainderA,
                                  &numberALength,
                                  remainderB,
                                  &numberBLength);
      
      // if the numbers were not equivalent, return the result of the number
      // collation
      if (leadingNumbersCollate != 0) {
        return leadingNumbersCollate;
      }
      
      // discard the numbers
      remainderA += numberALength;
      remainderB += numberBLength;;
      
      // if we failed to actually parse a number on both strings (ie, there was
      // a number char that was detected but it did not parse to a valid number)
      // then we need to advance both strings by one character, otherwise we can
      // run into an infinite loop trying to parse those two numbers if the two
      // remainders are also equal. If the remainders are not equal, then
      // leadingNumbersCollate was != 0 and we've exited already. If only one of
      // the two strings failed to parse to a number, but the remainders are
      // still equal, we've advanced on one of the strings, and we're not going
      // to infinitely loop.
      if (numberALength == 0 &&
          numberBLength == 0) {
        remainderA++;
        remainderB++;
      }
    
      // and loop ...
    }
    
  }
  
  // if both strings are now empty, the original strings were equivalent
  if (!*remainderA &&
      !*remainderB) {
    return 0;
  }
  
  // collate what we have left. although at most one string may have a leading
  // number, we want to go through CollateWithLeadingNumbers_UTF8 anyway in
  // order to enforce the position of strings with leading numbers relative to
  // strings without leading numbers
  
  PRInt32 numberALength;
  PRInt32 numberBLength;

  return CollateWithLeadingNumbers(aCollationBuffers,
                                   remainderA,
                                   &numberALength,
                                   remainderB,
                                   &numberBLength);
}

nsresult
CDatabaseEngine::GetCurrentCollationLocale(nsCString &aCollationLocale) {

#ifdef XP_MACOSX

  CFStringRef collationIdentifier = NULL;

  // try to read the collation language from the prefs (note, CFLocaleGetValue
  // with kCFLocaleCollationIdentifier would be ideal, but although that call
  // works for other constants, it does not for the collation identifier, at
  // least on 10.4, which we want to support).

  CFPropertyListRef pl = 
    CFPreferencesCopyAppValue(CFSTR("AppleCollationOrder"),
                              kCFPreferencesCurrentApplication);

  // use the value we've read if it existed and is what we're expecting
  if (pl != NULL && 
      CFGetTypeID(pl) == CFStringGetTypeID()) {
    collationIdentifier = (CFStringRef)pl;
  } else {
    // otherwise, retrieve the list of prefered languages, the first item
    // is the user's language, and defines the default collation.
    CFPropertyListRef al =
      CFPreferencesCopyAppValue(CFSTR("AppleLanguages"),
                                kCFPreferencesCurrentApplication);
    // if the array exists and is what we expect, read the first item
    if (al != NULL &&
        CFGetTypeID(al) == CFArrayGetTypeID()) {
      CFArrayRef ar = (CFArrayRef)al;
      if (CFArrayGetCount(ar) > 0) {
        CFTypeRef lang = CFArrayGetValueAtIndex(ar, 0);
        if (lang != NULL &&
            CFGetTypeID(lang) == CFStringGetTypeID()) {
          collationIdentifier = (CFStringRef)lang;
        }
      }
    }
  }

  // if everything goes wrong and we're completely unable to read either values,
  // that's kind of a bummer, print a debug message and assume that we are using
  // the standard "C" collation (strcmp). a reason this could happen might be
  // that we are running on an unsupported version of osx that does not have
  // these strings (either real old, or a future version which stores them
  // elsewhere or with different types). another reason this might happen is
  // if the computer is about to explode.

  if (collationIdentifier) {
    char buf[64]="";
    CFStringGetCString(collationIdentifier, 
                       buf, 
                       sizeof(buf), 
                       kCFStringEncodingASCII);
    buf[sizeof(buf)-1] = 0;

    aCollationLocale = buf;

  } else {
    LOG("Could not retrieve the collation locale identifier");
  }
#else

  // read the current collation id, this is usually the standard C collate
  nsCString curCollate(setlocale(LC_COLLATE, NULL));
  
  // read the user defined collation id, this usually corresponds to the
  // user's OS language selection
  aCollationLocale = setlocale(LC_COLLATE, "");
  
  // restore the default collation
  setlocale(LC_COLLATE, curCollate.get());

#endif

  return NS_OK;
}

NS_IMETHODIMP CDatabaseEngine::GetLocaleCollationID(nsAString &aID) {
  aID = NS_ConvertASCIItoUTF16(mCollationLocale);
  return NS_OK;
}

#ifdef USE_PERF_LOGGING

sbDatabaseEnginePerformanceLogger::sbDatabaseEnginePerformanceLogger(const nsAString& aQuery,
                                                                     const nsAString& aGuid) :
  mQuery(aQuery),
  mGuid(aGuid)
{
  mStart = PR_Now();
}

sbDatabaseEnginePerformanceLogger::~sbDatabaseEnginePerformanceLogger()
{
  PRUint32 total = PR_Now() - mStart;

  nsString consoleMessageStr;
  consoleMessageStr.AppendLiteral("sbDatabaseEnginePerformanceLogger\n\t");
  consoleMessageStr.AppendLiteral("DB GUID: ");
  consoleMessageStr.Append(mGuid);
  consoleMessageStr.AppendLiteral(" -- Execution Time: ");
  consoleMessageStr.AppendInt(total / PR_USEC_PER_MSEC);
  consoleMessageStr.AppendLiteral("ms\nQuery:\n\t");
  consoleMessageStr.Append(mQuery);

  nsCOMPtr<nsIConsoleService> consoleService = 
    do_GetService("@mozilla.org/consoleservice;1");

  if(consoleService) {
    consoleService->LogStringMessage(consoleMessageStr.get());    
  }
}

#endif
