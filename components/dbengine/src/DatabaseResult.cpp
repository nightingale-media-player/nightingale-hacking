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
 * \file DatabaseResult.cpp
 * \brief Database Result Object Implementation.
 */

#include "DatabaseResult.h"
#include <prmem.h>
#include <nsMemory.h>

#include <nsStringGlue.h>

#include <prlog.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDatabaseResult:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDatabaseResultLog = nsnull;
#define LOG(args) PR_LOG(gDatabaseResultLog, PR_LOG_DEBUG, args)
#else
#define LOG(args) /* nothing */
#endif

static inline 
void IfLock(PRLock *aLock) 
{
  if(NS_UNLIKELY(aLock)) {
    PR_Lock(aLock);
  }
}

static inline
void IfUnlock(PRLock *aLock) 
{
  if(NS_UNLIKELY(aLock)) {
    PR_Unlock(aLock);
  }
}

// CLASSES ====================================================================
//=============================================================================
// CDatabaseQuery Class
//=============================================================================
//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseResult, sbIDatabaseResult)

//-----------------------------------------------------------------------------
CDatabaseResult::CDatabaseResult(PRBool aRequiresLocking)
: m_RequiresLocking(aRequiresLocking)
, m_pLock(nsnull)
{
#ifdef PR_LOGGING
  if(!gDatabaseResultLog)
    gDatabaseResultLog = PR_NewLogModule("sbDatabaseResult");
#endif

  if(NS_UNLIKELY(m_RequiresLocking))
  {
    m_pLock = PR_NewLock();
    NS_ASSERTION(m_pLock, "CDatabaseResult.m_pColumnNamesLock failed");
  }
} //ctor

//-----------------------------------------------------------------------------
CDatabaseResult::~CDatabaseResult()
{
  if(m_RequiresLocking && m_pLock)
    PR_DestroyLock(m_pLock);
} //dtor

//-----------------------------------------------------------------------------
/* PRInt32 GetColumnCount (); */
NS_IMETHODIMP CDatabaseResult::GetColumnCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    *_retval = m_ColumnNames.size();
    IfUnlock(m_pLock);
  }
  else {
    *_retval = m_ColumnNames.size();
  }
  return NS_OK;
} //GetColumnCount

//-----------------------------------------------------------------------------
/* wstring GetColumnName (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnName(PRUint32 dbColumn, nsAString &_retval)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    if(dbColumn < m_ColumnNames.size()) {
      _retval = m_ColumnNames[dbColumn];
    }
    IfUnlock(m_pLock);
  }
  else if(dbColumn < m_ColumnNames.size()) {
    _retval = m_ColumnNames[dbColumn];
  }

  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* wstring GetColumnName (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnIndex(const nsAString &aColumnName, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = GetColumnIndexFromName(aColumnName);
  return NS_OK;
} //GetColumnIndex


//-----------------------------------------------------------------------------
/* PRInt32 GetRowCount (); */
NS_IMETHODIMP CDatabaseResult::GetRowCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    *_retval = m_RowCells.size();
    IfUnlock(m_pLock);
  }
  else {
    *_retval = m_RowCells.size();
  }

  return NS_OK;
} //GetRowCount

//-----------------------------------------------------------------------------
/* wstring GetRowCell (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCell(PRUint32 dbRow, PRUint32 dbCell, nsAString &_retval)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size()) {
      _retval = m_RowCells[dbRow][dbCell];
    }
    IfUnlock(m_pLock);
  }
  else if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size()) {
    _retval = m_RowCells[dbRow][dbCell];
  }

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* wstring GetRowCellByColumn (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumn(PRUint32 dbRow, const nsAString &dbColumn, nsAString &_retval)
{
  PRUint32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCell(dbRow, dbCell, _retval);
} //GetRowCellByColumn

//-----------------------------------------------------------------------------
/* wstring GetColumnNamePtr (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnNamePtr(PRUint32 dbColumn, PRUnichar **_retval)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    if(dbColumn < m_ColumnNames.size()) {
      *_retval = const_cast<PRUnichar *>(m_ColumnNames[dbColumn].BeginReading());
    }
    else {
      *_retval = nsnull;
    }
    IfUnlock(m_pLock);
  }
  else if(dbColumn < m_ColumnNames.size()) {
    *_retval = const_cast<PRUnichar *>(m_ColumnNames[dbColumn].BeginReading());
  }
  else {
    *_retval = nsnull;
  }

  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* wstring GetRowCellPtr (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCellPtr(PRUint32 dbRow, PRUint32 dbCell, PRUnichar **_retval)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size()) {
      *_retval = const_cast<PRUnichar *>(m_RowCells[dbRow][dbCell].BeginReading());
    }
    else {
      *_retval = nsnull;
    }
    IfUnlock(m_pLock);
  }
  else if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size()) {
    *_retval = const_cast<PRUnichar *>(m_RowCells[dbRow][dbCell].BeginReading());
  }
  else {
    *_retval = nsnull;
  }

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* noscript wstring GetRowCellByColumnPtr (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumnPtr(PRUint32 dbRow, const nsAString &dbColumn, PRUnichar **_retval)
{
  PRUint32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCellPtr(dbRow, dbCell, _retval);
} //GetRowCellByColumnPtr

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseResult::ClearResultSet()
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);

    m_ColumnNames.clear();
    m_RowCells.clear();
    m_ColumnResolveMap.clear();

    IfUnlock(m_pLock);
  }
  else {
    m_ColumnNames.clear();
    m_RowCells.clear();
    m_ColumnResolveMap.clear();
  }

  return NS_OK;
} //ClearResultSet

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::AddRow(const std::vector<nsString> &vCellValues)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    m_RowCells.push_back(vCellValues);
    IfUnlock(m_pLock);
  }
  else {
    m_RowCells.push_back(vCellValues);
  }

  return NS_OK;
} //AddRow

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::DeleteRow(PRUint32 dbRow)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    if(dbRow < m_RowCells.size()) {
      dbrowcells_t::iterator itRows = m_RowCells.begin();
      itRows += dbRow;

      if(itRows != m_RowCells.end())
        m_RowCells.erase(itRows);
    }
    IfUnlock(m_pLock);
  }
  else if(dbRow < m_RowCells.size()) {
    dbrowcells_t::iterator itRows = m_RowCells.begin();
    itRows += dbRow;

    if(itRows != m_RowCells.end())
      m_RowCells.erase(itRows);
  }

  return NS_OK;
} //DeleteRow

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetColumnNames(const std::vector<nsString> &vColumnNames)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    m_ColumnNames = vColumnNames;
    IfUnlock(m_pLock);
  }
  else {
    m_ColumnNames = vColumnNames;
  }

  return NS_OK;
} //SetColumnNames

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetColumnName(PRUint32 dbColumn, const nsString &strColumnName)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    m_ColumnNames[dbColumn] = strColumnName;
    IfUnlock(m_pLock);
  }
  else {
    m_ColumnNames[dbColumn] = strColumnName;
  }

  return NS_OK;
} //SetColumnName

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetRowCell(PRUint32 dbRow, PRUint32 dbCell, const nsString &strCellValue)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    m_RowCells[dbRow][dbCell] = strCellValue;
    IfUnlock(m_pLock);
  }
  else {
    m_RowCells[dbRow][dbCell] = strCellValue;
  }

  return NS_OK;
} //SetRowCell

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetRowCells(PRUint32 dbRow, const std::vector<nsString> &vCellValues)
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);
    m_RowCells[dbRow] = vCellValues;
    IfUnlock(m_pLock);
  }
  else {
    m_RowCells[dbRow] = vCellValues;
  }

  return NS_OK;
} //SetRowCells

//-----------------------------------------------------------------------------
PRUint32 CDatabaseResult::GetColumnIndexFromName(const nsAString &strColumnName)
{
  RebuildColumnResolveMap();
  PRUint32 retval = (PRUint32)-1;

  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);

    dbcolumnresolvemap_t::const_iterator itColumnIndex =
      m_ColumnResolveMap.find(nsString(strColumnName));

    if(itColumnIndex != m_ColumnResolveMap.end())
      retval = itColumnIndex->second;

    IfUnlock(m_pLock);
  }
  else {
    dbcolumnresolvemap_t::const_iterator itColumnIndex =
      m_ColumnResolveMap.find(nsString(strColumnName));

    if(itColumnIndex != m_ColumnResolveMap.end())
      retval = itColumnIndex->second;
  }

  return retval;
} //GetColumnIndexFromName

//-----------------------------------------------------------------------------
void CDatabaseResult::RebuildColumnResolveMap()
{
  if(NS_UNLIKELY(m_RequiresLocking)) {
    IfLock(m_pLock);

    if(m_ColumnNames.size() != m_ColumnResolveMap.size() ||
       m_ColumnResolveMap.size() == 0) {
      m_ColumnResolveMap.clear();

      PRUint32 nSize =  m_ColumnNames.size();
      for(PRUint32 i = 0; i < nSize; i++) {
        m_ColumnResolveMap.insert(std::make_pair(m_ColumnNames[i], i));
      }
    }
    
    IfUnlock(m_pLock);
  }
  else if(m_ColumnNames.size() != m_ColumnResolveMap.size() ||
          m_ColumnResolveMap.size() == 0) {
    m_ColumnResolveMap.clear();

    PRUint32 nSize =  m_ColumnNames.size();
    for(PRUint32 i = 0; i < nSize; i++) {
      m_ColumnResolveMap.insert(std::make_pair(m_ColumnNames[i], i));
    }
  }
}
