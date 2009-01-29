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
 * \file DatabaseEngine.cpp
 * \brief
 */

// INCLUDES ===================================================================
#include "DatabaseEngine.h"
#include "DatabasePreparedStatement.h"

#include <nsCOMPtr.h>
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

#include <vector>
#include <algorithm>
#include <prmem.h>
#include <prtypes.h>
#include <assert.h>

#include <nsIScriptError.h>
#include <nsIConsoleService.h>

#include <nsCOMArray.h>

// The maximum characters to output in a single PR_LOG call
#define MAX_PRLOG 400

#if defined(_WIN32)
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
#define USE_SQLITE_LARGE_CACHE_SIZE
#define USE_SQLITE_LARGE_PAGE_SIZE
#define USE_SQLITE_READ_UNCOMMITTED
#define USE_SQLITE_MEMORY_TEMP_STORE
#define USE_SQLITE_BUSY_TIMEOUT
// Can not use FTS with shared cache enabled
//#define USE_SQLITE_SHARED_CACHE

#define SQLITE_MAX_RETRIES            666

#if defined(_DEBUG) || defined(DEBUG)
  #if defined(XP_WIN)
    #include <windows.h>
  #endif
  #define HARD_SANITY_CHECK             1
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo* sDatabaseEngineLog = nsnull;
static PRLogModuleInfo* sDatabaseEnginePerformanceLog = nsnull;
#define TRACE(args) PR_LOG(sDatabaseEngineLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(sDatabaseEngineLog, PR_LOG_WARN, args)

#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#ifdef PR_LOGGING
#define BEGIN_PERFORMANCE_LOG(_strQuery, _dbName) \
 sbDatabaseEnginePerformanceLogger _performanceLogger(_strQuery, _dbName)
#else
#define BEGIN_PERFORMANCE_LOG(_strQuery, _dbName) /* nothing */
#endif

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

#define CHARTYPE_OTHER 0
#define CHARTYPE_DIGIT 1
#define CHARTYPE_DECIMALPOINT 2
#define CHARTYPE_SIGN 3
#define CHARTYPE_EXPONENT 4

PRInt32 getCharType(const char *p) {
  switch (*p) {
    case '.': 
    case ',': 
      return CHARTYPE_DECIMALPOINT;
    case '+':
    case '-': 
      return CHARTYPE_SIGN;
    case 'e':
    case 'E':
      return CHARTYPE_EXPONENT;
  }
  if (*p >= '0' && *p <= '9')
    return CHARTYPE_DIGIT;
  return CHARTYPE_OTHER;
}

// extract a leading number from a C or UTF8 string.
void extractLeadingNumber(const char *str,
                          PRBool *hasLeadingNumber,
                          PRFloat64 *leadingNumber,
                          nsCString &strippedString) {

  // it would be nice to be able to do all of this with just sscanf, but 
  // unfortunately that function does not tell us where the parsed number ended,
  // and we need to know that in order to strip it from the string, so we have
  // to parse manually. also, we want to handle ',' as '.', which sscanf doesn't
  // do.

  PRBool gotDecimalPoint = PR_FALSE;
  PRBool gotExponent = PR_FALSE;
  PRBool gotSign = PR_FALSE;
  PRBool gotExponentSign = PR_FALSE;
  PRBool gotDigit = PR_FALSE;
  PRBool gotExponentDigit = PR_FALSE;
  PRBool abortParsing = PR_FALSE;
  
  const char *p = str;
  
  while (!abortParsing && *p) {
    switch (getCharType(p)) {
      case CHARTYPE_SIGN: 
        if (!gotExponent) {
          // if we already had a sign for this number, or if the number part has
          // already started (already had digits or a decimal point) we can't
          // accept a sign here, so abort parsing.
          if (gotSign || gotDigit || gotDecimalPoint) {
            abortParsing = PR_TRUE;
            break;
          }
          // remember that we got a sign for the number part
          gotSign = PR_TRUE;
        } else {
          // if we already had a sign for this exponent, or if the number part
          // of the exponent has already started (already had a digit in the
          // exponent) we can't accept a sign here, so abort parsing.
          if (gotExponentSign || gotExponentDigit) {
            abortParsing = PR_TRUE;
            break;
          }
          // remember that we got a sign for the exponent part
          gotExponentSign = PR_TRUE;
        }
        break;
      case CHARTYPE_DIGIT:
        // remember that the number part has started
        if (!gotExponent)
          gotDigit = PR_TRUE;
        else
          gotExponentDigit = PR_TRUE;
        break;
      case CHARTYPE_DECIMALPOINT:
        if (!gotExponent) {
          // if we already had a decimal point for this number, we can't have
          // another one, so abort parsing.
          if (gotDecimalPoint) {
            abortParsing = PR_TRUE;
            break;
          }
          // remember that we got a decimal point for the number part
          gotDecimalPoint = PR_TRUE;
        } else {
          // decimal points cannot be part of an exponent, so abort parsing.
          abortParsing = PR_TRUE;
          break;
        }
        break;
      case CHARTYPE_EXPONENT:
        // if we already are in the exponent part, we cannot get another
        // exponent character, so abort parsing.
        if (gotExponent) {
          abortParsing = PR_TRUE;
          break;
        }
        // this is only an exponent character if the next character is either
        // a digit or a sign (it is safe to dereference p+1, since at worst
        // it will be a null terminator)
        switch (getCharType(p+1)) {
          case CHARTYPE_DIGIT:
          case CHARTYPE_SIGN:
            // remember that we got an exponent.
            gotExponent = PR_TRUE;
            break;
          default:
            // anything else means this is not an exponent, but just the letter
            // 'e' or 'E', so abort parsing.
            abortParsing = PR_TRUE;
            break;
        }
        break;
      case CHARTYPE_OTHER:
        // anything else is a character or symbol that isn't part of a valid
        // number, so abort parsing.
        abortParsing = PR_TRUE;
        break;
    }
    p++;
  }
  
  // if we stopped the parser on an invalid char, we need to back up one char,
  // otherwise the whole string was a number and p just points at the terminal
  // null char.
  if (abortParsing)
    p--;
  
  // p now points at the first character that isn't part of a valid number.
  // copy the string, without the number.
  strippedString = p;
  
  if (p == str) {
    // no number found
    *hasLeadingNumber = PR_FALSE;
    *leadingNumber = 0;
  } else {
    // copy the number
    nsCString number(str, p-str);
    
    // if the number contains a ',', change it to '.'
    PRInt32 pos = number.FindChar(',');
    if (pos != -1) number.Replace(pos, 1, '.');
    
    // read the number
    PRFloat64 value;
    
    if(PR_sscanf(number.get(), "%lf", &value) != 1) {
      // this should not happen, we just parsed and we know there is a valid
      // number... handle it anyway.
      *hasLeadingNumber = PR_FALSE;
      *leadingNumber = 0;
      strippedString = str;
      LOG(("Failed to PR_sscanf a number in string '%s', this should not happen", str));
    } else {
      *hasLeadingNumber = PR_TRUE;
      *leadingNumber = value;
    }
  }
}
 
static PRInt32 gLocaleCollationEnabled = PR_TRUE;

/*
 * Perform collation for current locale (data always comes in as UTF8)
 *
 * IMPORTANT NOTE: Whenever the algorithm for library_collate is changed and
 * yields a different sort order than before for *any* set of arbitrary strings,
 * a new migration step must be added to issue the "reindex 'library_collate'"
 * sql query. THIS IS OF UTMOST IMPORTANCE because otherwise, the library's
 * index can become entirely trashed!
 *
 */
static int library_collate_func(int nA,
                                const char *zA,
                                int nB,
                                const char *zB)
{
  // shortcut when both strings are empty
  if (nA == 0 && nB == 0) 
    return 0;

  nsCString _zA(zA, nA);
  nsCString _zB(zB, nB);
  
  CDatabaseEngine *db = CDatabaseEngine::GetSingleton();
  // if no dbengine service or if collation is disabled, just do a C compare
  if (!db || !gLocaleCollationEnabled) {
    return strcmp(_zA.get(), _zB.get());
  }
  
  return db->CollateUTF8(_zA.get(), _zB.get());
}

void swap_string(const void *aDest,
                 const void *aSrc,
                 int len) {
                        
  char *d = (char *)aDest;
  char *s = (char *)aSrc;
  for (int i=0;i<len;i++) {
    *d++ = *(s+1);
    *d++ = *s++;
    s++;
  }
  *s++ = 0;
  *s++ = 0;
}

static int library_collate_func_utf16be(void *pCtx,
                                        int nA,
                                        const void *zA,
                                        int nB,
                                        const void *zB)
{
  #ifdef BIGENDIAN

  const wchar_t *_zA = (const wchar_t *)PR_Malloc((nA+1)*2);
  const wchar_t *_zB = (const wchar_t *)PR_Malloc((nB+1)*2);

  swap_string(_zA, zA, nA);
  swap_string(_zB, zB, nB);

  #else

  const wchar_t *_zA = (const wchar_t *)zA;
  const wchar_t *_zB = (const wchar_t *)zB;

  #endif
  
  NS_ConvertUTF16toUTF8 _zA8((const PRUnichar *)_zA, nA);
  NS_ConvertUTF16toUTF8 _zB8((const PRUnichar *)_zB, nB);
  
  int r = library_collate_func(_zA8.Length(), 
                               _zA8.BeginReading(), 
                               _zB8.Length(), 
                               _zB8.BeginReading());

  #ifdef BIGENDIAN
  
  PR_Free(_zA);
  PR_Free(_zB);
  
  #endif
  
  return r;
}
                        
static int library_collate_func_utf16le(void *pCtx,
                                        int nA,
                                        const void *zA,
                                        int nB,
                                        const void *zB)
{
  #ifdef LITTLEENDIAN

  const wchar_t *_zA = (const wchar_t *)PR_Malloc((nA+1)*2);
  const wchar_t *_zB = (const wchar_t *)PR_Malloc((nB+1)*2);

  swap_string(_zA, zA, nA);
  swap_string(_zB, zB, nB);

  #else

  const wchar_t *_zA = (const wchar_t *)zA;
  const wchar_t *_zB = (const wchar_t *)zB;

  #endif
  
  NS_ConvertUTF16toUTF8 _zA8((const PRUnichar *)_zA, nA);
  NS_ConvertUTF16toUTF8 _zB8((const PRUnichar *)_zB, nB);
  
  int r = library_collate_func(_zA8.Length(), 
                               _zA8.BeginReading(), 
                               _zB8.Length(), 
                               _zB8.BeginReading());

  #ifdef LITTLEENDIAN
  
  PR_Free(_zA);
  PR_Free(_zB);
  
  #endif
  
  return r;
}

static int library_collate_func_utf8(void *pCtx,
                                     int nA,
                                     const void *zA,
                                     int nB,
                                     const void *zB)
{
  return library_collate_func(nA, (const char *)zA, nB, (const char *)zB);
}

//-----------------------------------------------------------------------------
/* Sqlite Dump Helper Class */
//-----------------------------------------------------------------------------
class CDatabaseDumpProcessor : public nsIRunnable 
{
public:
  CDatabaseDumpProcessor(CDatabaseEngine *aCallback,
                         QueryProcessorThread *aQueryProcessorThread,
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
  nsRefPtr<QueryProcessorThread> mQueryProcessorThread;
  PRBool                         writeableSchema;  // true if PRAGMA writable_schema=on
};


NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseDumpProcessor, nsIRunnable)

CDatabaseDumpProcessor::CDatabaseDumpProcessor(CDatabaseEngine *aCallback,
                                               QueryProcessorThread *aQueryProcessorThread,
                                               nsIFile *aOutputFile)
: mEngineCallback(aCallback)
, mQueryProcessorThread(aQueryProcessorThread)
, mOutputFile(aOutputFile)
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
    nsAutoLock handleLock(mQueryProcessorThread->m_pHandleLock);
    
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
  PRInt32 rc = sqlite3_exec(mQueryProcessorThread->m_pHandle,
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
    rc = sqlite3_exec(mQueryProcessorThread->m_pHandle,
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
  int rc = sqlite3_prepare(mQueryProcessorThread->m_pHandle,
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

    rc = sqlite3_prepare(dumpProcessor->mQueryProcessorThread->m_pHandle, 
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
NS_IMPL_THREADSAFE_ISUPPORTS1(QueryProcessorThread, nsIRunnable)

CDatabaseEngine *gEngine = nsnull;

// CLASSES ====================================================================
//-----------------------------------------------------------------------------
CDatabaseEngine::CDatabaseEngine()
: m_pDBStorePathLock(nsnull)
, m_pDatabasesGUIDListLock(nsnull)
, m_pThreadMonitor(nsnull)
, m_AttemptShutdownOnDestruction(PR_FALSE)
, m_IsShutDown(PR_FALSE)
#ifdef XP_MACOSX
, m_Collator(nsnull)
#endif
{
#ifdef PR_LOGGING
  if (!sDatabaseEngineLog)
    sDatabaseEngineLog = PR_NewLogModule("sbDatabaseEngine");
  if (!sDatabaseEnginePerformanceLog)
    sDatabaseEnginePerformanceLog = PR_NewLogModule("sbDatabaseEnginePerformance");
#endif
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CDatabaseEngine::~CDatabaseEngine()
{
  if (m_AttemptShutdownOnDestruction)
    Shutdown();
  if (m_pDBStorePathLock)
    PR_DestroyLock(m_pDBStorePathLock);
  if (m_pThreadMonitor)
    nsAutoMonitor::DestroyMonitor(m_pThreadMonitor);
} //dtor

//-----------------------------------------------------------------------------
CDatabaseEngine* CDatabaseEngine::GetSingleton()
{
  if (gEngine) {
    NS_ADDREF(gEngine);
    return gEngine;
  }

  NS_NEWXPCOM(gEngine, CDatabaseEngine);
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
  LOG(("CDatabaseEngine[0x%.8x] - Init() - sqlite version %s",
       this, sqlite3_libversion()));

  PRBool success = m_ThreadPool.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  m_pThreadMonitor =
    nsAutoMonitor::NewMonitor("CDatabaseEngine.m_pThreadMonitor");

  NS_ENSURE_TRUE(m_pThreadMonitor, NS_ERROR_OUT_OF_MEMORY);

  m_pDBStorePathLock = PR_NewLock();
  NS_ENSURE_TRUE(m_pDBStorePathLock, NS_ERROR_OUT_OF_MEMORY);

  m_pDatabasesGUIDListLock = PR_NewLock();
  NS_ENSURE_TRUE(m_pDatabasesGUIDListLock, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = CreateDBStorePath();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to create db store folder in profile!");

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if(NS_SUCCEEDED(rv)) {
    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                      PR_FALSE);
  }

  // This shouldn't be an 'else' case because we want to set this flag if
  // either of the above calls failed
  if (NS_FAILED(rv)) {
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
                     kUCCollateStandardOptions,
                     &m_Collator);
#endif

  return NS_OK;
}


//-----------------------------------------------------------------------------
PR_STATIC_CALLBACK(PLDHashOperator)
EnumThreadsOperate(nsStringHashKey::KeyType aKey, QueryProcessorThread *aThread, void *aClosure)
{
  NS_ASSERTION(aThread, "aThread is null");
  NS_ASSERTION(aClosure, "aClosure is null");

  // Stop if thread is null.
  NS_ENSURE_TRUE(aThread, PL_DHASH_STOP);

  // Stop if closure is null because it
  // contains the operation we are going to perform.
  NS_ENSURE_TRUE(aClosure, PL_DHASH_STOP);

  nsresult rv;
  PRUint32 *op = static_cast<PRUint32 *>(aClosure);

  switch(*op) {
    case CDatabaseEngine::dbEnginePreShutdown:
      rv = aThread->PrepareForShutdown();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to prepare worker thread for shutdown.");
    break;

    case CDatabaseEngine::dbEngineShutdown:
      rv = aThread->GetThread()->Shutdown();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to shutdown worker thread.");
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
  m_ThreadPool.EnumerateRead(EnumThreadsOperate, &op);

  op = dbEngineShutdown;
  m_ThreadPool.EnumerateRead(EnumThreadsOperate, &op);

  m_ThreadPool.Clear();
  
#ifdef XP_MACOSX
  if (m_Collator)
    ::UCDisposeCollator(&m_Collator);
#endif

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Observe(nsISupports *aSubject,
                                       const char *aTopic,
                                       const PRUnichar *aData)
{
  // Bail if we don't care about the message
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    return NS_OK;

  // Shutdown our threads
  nsresult rv = Shutdown();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Shutdown Failed!");

  // And remove ourselves from the observer service
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
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

  ret = sqlite3_create_collation(pHandle,
                                 "library_collate",
                                 SQLITE_UTF8,
                                 NULL,
                                 library_collate_func_utf8);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set library collate function: utf8!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "library_collate",
                                 SQLITE_UTF16LE,
                                 NULL,
                                 library_collate_func_utf16le);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set library collate function: utf16le!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "library_collate",
                                 SQLITE_UTF16BE,
                                 NULL,
                                 library_collate_func_utf16be);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set library collate function: utf16be!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

#if defined(USE_SQLITE_LARGE_PAGE_SIZE)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA page_size = 16384", nsnull, nsnull, &strErr);
    if(strErr) {
      NS_WARNING(strErr);
      sqlite3_free(strErr);
    }
  }
#endif

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

#if defined(USE_SQLITE_LARGE_CACHE_SIZE)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA cache_size = 16000", nsnull, nsnull, &strErr);
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
  sqlite3_interrupt(pHandle);

  PRInt32 ret = sqlite3_close(pHandle);
  NS_ASSERTION(ret == SQLITE_OK, "");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  return NS_OK;
} //CloseDB

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::CloseDatabase(const nsAString &aDatabaseGUID) 
{
  nsAutoMonitor mon(m_pThreadMonitor);

  nsRefPtr<QueryProcessorThread> pThread;
  if(m_ThreadPool.Get(aDatabaseGUID, getter_AddRefs(pThread))) {
    
    nsresult rv = pThread->PrepareForShutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = pThread->GetThread()->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    m_ThreadPool.Remove(aDatabaseGUID);
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

  nsRefPtr<QueryProcessorThread> pThread = GetThreadByQuery(pQuery, PR_TRUE);
  NS_ENSURE_TRUE(pThread, 1);

  nsresult rv = pThread->PushQueryToQueue(pQuery);
  NS_ENSURE_SUCCESS(rv, 1);

  PR_Lock(pQuery->m_StateLock);
  pQuery->m_IsExecuting = PR_TRUE;
  PR_Unlock(pQuery->m_StateLock);

  rv = pThread->NotifyQueue();
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
  
  nsRefPtr<QueryProcessorThread> pThread = GetThreadByQuery(dummyQuery, PR_TRUE);
  NS_ENSURE_TRUE(pThread, NS_ERROR_FAILURE);

  nsRefPtr<CDatabaseDumpProcessor> dumpProcessor = 
    new CDatabaseDumpProcessor(this, pThread, aOutFile);

  return dumpProcessor->Run();
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::DumpMemoryStatistics()
{
  int status_op  = -1;
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
NS_IMETHODIMP CDatabaseEngine::ReleaseMemory()
{
  // Attempt to free a large amount of memory.
  // This will cause SQLite to free as much as it can.
  int memReleased = sqlite3_release_memory(500000000);
  printf("CDatabaseEngine::ReleaseMemory() managed to release %d bytes\n", memReleased);
  return NS_OK;
}

//-----------------------------------------------------------------------------
already_AddRefed<QueryProcessorThread> CDatabaseEngine::GetThreadByQuery(CDatabaseQuery *pQuery,
                                                         PRBool bCreate /*= PR_FALSE*/)
{
  NS_ENSURE_TRUE(pQuery, nsnull);

  nsAutoString strGUID;
  nsAutoMonitor mon(m_pThreadMonitor);

  nsRefPtr<QueryProcessorThread> pThread;

  nsresult rv = pQuery->GetDatabaseGUID(strGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  if(!m_ThreadPool.Get(strGUID, getter_AddRefs(pThread))) {
    pThread = CreateThreadFromQuery(pQuery);
  }

  NS_ENSURE_TRUE(pThread, nsnull);

  QueryProcessorThread *p = pThread.get();
  NS_ADDREF(p);

  return p;
}

//-----------------------------------------------------------------------------
already_AddRefed<QueryProcessorThread> CDatabaseEngine::CreateThreadFromQuery(CDatabaseQuery *pQuery)
{
  nsAutoString strGUID;
  nsAutoMonitor mon(m_pThreadMonitor);

  nsresult rv = pQuery->GetDatabaseGUID(strGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsRefPtr<QueryProcessorThread> pThread(new QueryProcessorThread());
  NS_ENSURE_TRUE(pThread, nsnull);

  sqlite3 *pHandle = nsnull;
  rv = OpenDB(strGUID, pQuery, &pHandle);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = pThread->Init(this, strGUID, pHandle);
  NS_ENSURE_SUCCESS(rv, nsnull);

  PRBool success = m_ThreadPool.Put(strGUID, pThread);
  NS_ENSURE_TRUE(success, nsnull);

  QueryProcessorThread *p = pThread.get();
  NS_ADDREF(p);

  return p;
}

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CDatabaseEngine::QueryProcessor(CDatabaseEngine* pEngine,
                                                            QueryProcessorThread *pThread)
{  
  if(!pEngine ||
     !pThread ) {
    NS_WARNING("Called QueryProcessor without an engine or thread!!!!");
    return;
  }

  CDatabaseQuery *pQuery = nsnull;

  while(PR_TRUE)
  {
    pQuery = nsnull;


    { // Enter Monitor
      // Wrap any calls that access the pThread.m_Queue because they cause a
      // context switch between threads and can mess up the link between the
      // NotifyQueue() call and the Wait() here. See bug 6514 for more details.
      nsAutoMonitor mon(pThread->m_pQueueMonitor);

      PRUint32 queueSize = 0;
      nsresult rv = pThread->GetQueueSize(queueSize);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get queue size.");

      while (!queueSize &&
             !pThread->m_Shutdown) {

        mon.Wait();

        rv = pThread->GetQueueSize(queueSize);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get queue size.");
      }

      // Handle shutdown request
      if (pThread->m_Shutdown) {

#if defined(XP_WIN)
        // Cleanup all thread resources.
        sqlite3_thread_cleanup();
#endif

        return;
      }

      // We must have an item in the queue
      rv = pThread->PopQueryFromQueue(&pQuery);
      NS_ASSERTION(NS_SUCCEEDED(rv),
        "Failed to pop query from queue. Thread will restart.");

      // Restart the thread.
      if(NS_FAILED(rv)) {

#if defined(XP_WIN)
        // Cleanup all thread resources.
        sqlite3_thread_cleanup();
#endif

        return;
      }

    } // Exit Monitor

    //The query is now in a running state.
    nsAutoMonitor mon(pQuery->m_pQueryRunningMonitor);

    sqlite3 *pDB = pThread->m_pHandle;

    LOG(("DBE: Process Start, thread 0x%x query 0x%x",
      PR_GetCurrentThread(), pQuery));

    PRUint32 nQueryCount = 0;
    PRBool bFirstRow = PR_TRUE;

    //Default return error.
    pQuery->SetLastError(SQLITE_ERROR);
    pQuery->GetQueryCount(&nQueryCount);

    for(PRUint32 currentQuery = 0; currentQuery < nQueryCount && !pQuery->m_IsAborting; ++currentQuery)
    {
      nsAutoPtr<bindParameterArray_t> pParameters;
      
      int retDB = 0; // sqlite return code.
      
      nsCOMPtr<sbIDatabasePreparedStatement> preparedStatement;
      nsresult rv = pQuery->PopQuery(getter_AddRefs(preparedStatement));
      if (NS_FAILED(rv)) {
        LOG(("DBE: Failed to get a prepared statement from the Query object."));
        continue;
      }
      nsString strQuery;
      preparedStatement->GetQueryString(strQuery);
      // cast the prepared statement to its C implementation. this is a really lousy thing to do to an interface pointer.
      // since it mostly prevents ever being able to provide an alternative implementation.
      CDatabasePreparedStatement *actualPreparedStatement = static_cast<CDatabasePreparedStatement*>(preparedStatement.get());
      sqlite3_stmt *pStmt = actualPreparedStatement->GetStatement(pThread->m_pHandle);
      
      if (!pStmt) {
        LOG(("DBE: Failed to create a prepared statement from the Query object."));
        continue;
      }
      
      PR_Lock(pQuery->m_CurrentQueryLock);
      pQuery->m_CurrentQuery = currentQuery;
      PR_Unlock(pQuery->m_CurrentQueryLock);

      pParameters = pQuery->PopQueryParameters();

      nsAutoString dbName;
      pQuery->GetDatabaseGUID(dbName);

      BEGIN_PERFORMANCE_LOG(strQuery, dbName);

      LOG(("DBE: '%s' on '%s'\n",
        NS_ConvertUTF16toUTF8(dbName).get(),
        NS_ConvertUTF16toUTF8(strQuery).get()));

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
            LOG(("DBE: Parameter %d is 'NULL'", i));
            break;
          case UTF8STRING:
            sqlite3_bind_text(pStmt, i + 1,
              p.utf8StringValue.get(),
              p.utf8StringValue.Length(),
              SQLITE_TRANSIENT);
            LOG(("DBE: Parameter %d is '%s'", i, p.utf8StringValue.get()));
            break;
          case STRING:
          {
            nsCString cStringValue = NS_ConvertUTF16toUTF8(p.stringValue);
            sqlite3_bind_text(pStmt, i + 1,
              cStringValue.get(),
              cStringValue.Length(),
              SQLITE_TRANSIENT);
             LOG(("DBE: Parameter %d is '%s'", i, cStringValue.get()));
            break;
          }
          case DOUBLE:
            sqlite3_bind_double(pStmt, i + 1, p.doubleValue);
            LOG(("DBE: Parameter %d is '%f'", i, p.doubleValue));
            break;
          case INTEGER32:
            sqlite3_bind_int(pStmt, i + 1, p.int32Value);
            LOG(("DBE: Parameter %d is '%d'", i, p.int32Value));
            break;
          case INTEGER64:
            sqlite3_bind_int64(pStmt, i + 1, p.int64Value);
            LOG(("DBE: Parameter %d is '%ld'", i, p.int64Value));
            break;
        }
      }

      PRInt32 nRetryCount = 0;
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
            CDatabaseResult *pRes = pQuery->GetResultObject();
            PR_Lock(pQuery->m_pQueryResultLock);

            int nCount = sqlite3_column_count(pStmt);

            if(bFirstRow)
            {
              bFirstRow = PR_FALSE;

              PR_Unlock(pQuery->m_pQueryResultLock);
              pRes->ClearResultSet();
              PR_Lock(pQuery->m_pQueryResultLock);

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
              pRes->SetColumnNames(vColumnNames);
            }

            std::vector<nsString> vCellValues;
            vCellValues.reserve(nCount);

            TRACE(("DBE: Result row %d:", totalRows));

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
                TRACE(("Column %d: '%s' ", k,
                  NS_ConvertUTF16toUTF8(strCellValue).get()));
              }
              totalRows++;

              pRes->AddRow(vCellValues);

              // If this is a rolling limit query, we're done
              if (rollingLimit > 0) {
                pQuery->SetRollingLimitResult(rollingRowCount);
                pQuery->SetLastError(SQLITE_OK);
                TRACE(("Rolling limit query complete, %d rows", totalRows));
                finishEarly = PR_TRUE;
              }
            }

            PR_Unlock(pQuery->m_pQueryResultLock);
          }
          break;

        case SQLITE_DONE:
          {
            pQuery->SetLastError(SQLITE_OK);
            TRACE(("Query complete, %d rows", totalRows));
          }
        break;

        case SQLITE_BUSY:
          {
            sqlite3_reset(pStmt);
            sqlite3_sleep(50);

            retDB = SQLITE_ROW;
          }
        break;

        default:
          {
            // Log all SQL errors to the error console.
            pEngine->ReportError(pDB, pStmt);
            pQuery->SetLastError(retDB);
          }
        }

        // Throttle slightly.
        // XXXAus: Not in use for the time being.
        // PR_Sleep(PR_MillisecondsToInterval(0));
      }
      while(retDB == SQLITE_ROW &&
            !pQuery->m_IsAborting &&
            !finishEarly);

      //Didn't get any rows
      if(bFirstRow)
      {
        CDatabaseResult *pRes = pQuery->GetResultObject();
        pRes->ClearResultSet();
      }

      // Quoth the sqlite wiki:
      // Sometimes people think they have finished with a SELECT statement because sqlite3_step() 
      // has returned SQLITE_DONE. But the SELECT is not really complete until sqlite3_reset() 
      //  or sqlite3_finalize() have been called. 
      sqlite3_reset(pStmt);
    }

    //Whatever happened, the query is done running now.
    PR_Lock(pQuery->m_StateLock);

    pQuery->m_QueryHasCompleted = PR_TRUE;
    pQuery->m_IsExecuting = PR_FALSE;
    pQuery->m_IsAborting = PR_FALSE;

    PR_Unlock(pQuery->m_StateLock);

    LOG(("DBE: Notified query monitor."));

    //Fire off the callback if there is one.
    pEngine->DoSimpleCallback(pQuery);
    LOG(("DBE: Simple query listeners have been processed."));

    LOG(("DBE: Process End"));

    mon.NotifyAll();
    mon.Exit();

    NS_RELEASE(pQuery);
  } // while

  return;
} //QueryProcessor

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

PRInt32 CDatabaseEngine::CollateUTF8(const char *aStr1, const char *aStr2) {

  PRInt32 retval;

  // extract leading numbers
  
  PRBool hasLeadingNumberA = PR_FALSE;
  PRBool hasLeadingNumberB = PR_FALSE;
  
  PRFloat64 leadingNumberA;
  PRFloat64 leadingNumberB;
  
  nsCString strippedA;
  nsCString strippedB;
  
  extractLeadingNumber(aStr1, &hasLeadingNumberA, &leadingNumberA, strippedA);
  extractLeadingNumber(aStr2, &hasLeadingNumberB, &leadingNumberB, strippedB);
  
  // we want strings with leading numbers to always sort at the end
  if (hasLeadingNumberA && !hasLeadingNumberB) {
    return 1;
  } else if (!hasLeadingNumberA && hasLeadingNumberB) {
    return -1;
  } else if (hasLeadingNumberA && hasLeadingNumberB) {
    if (leadingNumberA > leadingNumberB) 
      return 1;
    else if (leadingNumberA < leadingNumberB)
      return -1;
  }
  
  // either both numbers are equal, or neither string had a leading number,
  // use the (possibly) stripped down strings to collate.
  
#ifndef XP_MACOSX

  // unless we are on osx, we rely on the CRT's locale settings, so read the
  // current locale for collate so we can restore it later
  nsCString oldlocale_collate(setlocale(LC_COLLATE, NULL));

  setlocale(LC_COLLATE, mCollationLocale.get());

#ifdef XP_UNIX

  // on linux, use glib's utf8 collate function, so we don't have to care about
  // the whole wchar_t size mess

  retval = g_utf8_collate(strippedA.get(), strippedB.get());
  
#endif

#ifdef XP_WIN

  // on windows, use wcscoll with wchar_t strings

  // convert to utf16
  nsString input_a_utf16 = NS_ConvertUTF8toUTF16(strippedA);
  nsString input_b_utf16 = NS_ConvertUTF8toUTF16(strippedB);

  // apply the proper collation algorithm, depending on the user's locale.
  
  // note that it is impossible to use the proper sort for *all* languages at
  // the same time, because what is proper depends on the original language from
  // which the string came. for instance, hungarian artists should have their
  // accented vowels sorted the same as non-accented ones, but a french artist 
  // should have the last accent determine that order. because we cannot
  // possibly guess the origin locale for the string, the only thing we can do
  // is use the user's current locale on all strings.
  
  // that being said, many language-specific letters (such as the german eszett)
  // have one one way of being properly sorted (in this instance, it must sort
  // with the same weight as 'ss', and are collated that way no matter what
  // locale is being used.
  retval = wcscoll((wchar_t *)input_a_utf16.BeginReading(), 
                   (wchar_t *)input_b_utf16.BeginReading());

#endif

  // restore the previous collate setting
  setlocale(LC_COLLATE, oldlocale_collate.get());
  
#else

  // on osx, use carbon collate functions because the CRT functions do not
  // have access to carbon's collation setting.

  NS_ConvertUTF8toUTF16 _zA16(strippedA);
  NS_ConvertUTF8toUTF16 _zB16(strippedB);
  
  // we should always have a collator, but just in case we don't,
  // use the default collation algorithm.
  if (!m_Collator) {
    ::UCCompareTextDefault(kUCCollateStandardOptions, 
                           (const UniChar *)_zA16.BeginReading(),
                           _zA16.Length(),
                           (const UniChar *)_zB16.BeginReading(),
                           _zB16.Length(), 
                           NULL,
                           (SInt32 *)&retval);
  } else {
    ::UCCompareText(m_Collator,
                    (const UniChar *)_zA16.BeginReading(),
                    _zA16.Length(),
                    (const UniChar *)_zB16.BeginReading(),
                    _zB16.Length(), 
                    NULL,
                    (SInt32 *)&retval);
  }
  
#endif

  return retval;
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
    LOG(("Could not retrieve the collation locale identifier"))
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

#ifdef PR_LOGGING
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

  PRUint32 length = mQuery.Length();
  for (PRUint32 i = 0; i < (length / MAX_PRLOG) + 1; i++) {
    nsAutoString q(Substring(mQuery, MAX_PRLOG * i, MAX_PRLOG));
    if (i == 0) {
      PR_LOG(sDatabaseEnginePerformanceLog, PR_LOG_DEBUG,
             ("sbDatabaseEnginePerformanceLogger %s\t%u\t%s",
              NS_LossyConvertUTF16toASCII(mGuid).get(),
              total,
              NS_LossyConvertUTF16toASCII(q).get()));
    }
    else {
      PR_LOG(sDatabaseEnginePerformanceLog, PR_LOG_DEBUG,
             ("sbDatabaseEnginePerformanceLogger +%s",
              NS_LossyConvertUTF16toASCII(q).get()));
    }
  }
}
#endif

