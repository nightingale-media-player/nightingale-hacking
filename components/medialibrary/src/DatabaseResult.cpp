/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
#include <xpcom/nsMemory.h>
#include <nsAutoLock.h>

#ifdef DEBUG_locks
#include <nsString.h>
#include <nsPrintfCString.h>
#endif

// CLASSES ====================================================================
//=============================================================================
// CDatabaseQuery Class
//=============================================================================
//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseResult, sbIDatabaseResult)

//-----------------------------------------------------------------------------
CDatabaseResult::CDatabaseResult()
: m_pColumnNamesLock(PR_NewLock())
, m_pRowCellsLock(PR_NewLock())
, m_pColumnResolveMap(PR_NewLock())
{
  NS_ASSERTION(m_pColumnNamesLock, "CDatabaseResult.m_pColumnNamesLock failed");
  NS_ASSERTION(m_pRowCellsLock, "CDatabaseResult.m_pRowCellsLock failed");
#ifdef DEBUG_locks
  nsCAutoString log;
  log += NS_LITERAL_CSTRING("\n\nCDatabaseResult (") + nsPrintfCString("%x", this) + NS_LITERAL_CSTRING(") lock addresses:\n");
  log += NS_LITERAL_CSTRING("m_pColumnNamesLock  = ") + nsPrintfCString("%x\n", m_pColumnNamesLock);
  log += NS_LITERAL_CSTRING("m_pRowCellsLock     = ") + nsPrintfCString("%x\n", m_pRowCellsLock);
  log += NS_LITERAL_CSTRING("m_pColumnResolveMap = ") + nsPrintfCString("%x\n", m_pColumnResolveMap);
  log += NS_LITERAL_CSTRING("\n");
  NS_WARNING(log.get());
#endif
} //ctor

//-----------------------------------------------------------------------------
CDatabaseResult::~CDatabaseResult()
{
  if (m_pColumnNamesLock)
    PR_DestroyLock(m_pColumnNamesLock);
  if (m_pRowCellsLock)
    PR_DestroyLock(m_pRowCellsLock);
  if (m_pColumnResolveMap)
    PR_DestroyLock(m_pColumnResolveMap);
} //dtor

//-----------------------------------------------------------------------------
/* PRInt32 GetColumnCount (); */
NS_IMETHODIMP CDatabaseResult::GetColumnCount(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  {
    nsAutoLock lock(m_pColumnNamesLock);
    *_retval = (PRInt32)m_ColumnNames.size();
  }
  return NS_OK;
} //GetColumnCount

//-----------------------------------------------------------------------------
/* wstring GetColumnName (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnName(PRInt32 dbColumn, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(dbColumn, 0);

  {
    nsAutoLock lock(m_pColumnNamesLock);
    if((PRUint32)dbColumn < m_ColumnNames.size()) {
      _retval = m_ColumnNames[dbColumn];
    }
  }

  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* wstring GetColumnName (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnIndex(const nsAString &aColumnName, PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = GetColumnIndexFromName(aColumnName);

  return NS_OK;
} //GetColumnIndex


//-----------------------------------------------------------------------------
/* PRInt32 GetRowCount (); */
NS_IMETHODIMP CDatabaseResult::GetRowCount(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  {
    nsAutoLock lock(m_pRowCellsLock);
    *_retval = (PRInt32)m_RowCells.size();
  }

  return NS_OK;
} //GetRowCount

//-----------------------------------------------------------------------------
/* wstring GetRowCell (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCell(PRInt32 dbRow, PRInt32 dbCell, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  {
    nsAutoLock lock(m_pRowCellsLock);
    if((PRUint32)dbRow < m_RowCells.size() && (PRUint32)dbCell < m_RowCells[dbRow].size()) {
      _retval = m_RowCells[dbRow][dbCell];
    }
  }

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* wstring GetRowCellByColumn (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumn(PRInt32 dbRow, const nsAString &dbColumn, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  PRInt32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCell(dbRow, dbCell, _retval);
} //GetRowCellByColumn

//-----------------------------------------------------------------------------
/* wstring GetColumnNamePtr (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnNamePtr(PRInt32 dbColumn, PRUnichar **_retval)
{
  NS_ENSURE_ARG_MIN(dbColumn, 0);
  {
    nsAutoLock lock(m_pColumnNamesLock);
    if((PRUint32)dbColumn < m_ColumnNames.size())
    {
      //*_retval = ToNewUnicode(m_ColumnNames[dbColumn]);
      //if(!*_retval) return NS_ERROR_OUT_OF_MEMORY;
      *_retval = NS_CONST_CAST(PRUnichar *, PromiseFlatString(m_ColumnNames[dbColumn]).get());
    }
    else
    {
      *_retval = nsnull;
    }
  }
  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* wstring GetRowCellPtr (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCellPtr(PRInt32 dbRow, PRInt32 dbCell, PRUnichar **_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  NS_ENSURE_ARG_MIN(dbCell, 0);
  {
    nsAutoLock lock(m_pRowCellsLock);
    if((PRUint32)dbRow < m_RowCells.size() && (PRUint32)dbCell < m_RowCells[dbRow].size())
    {
      //*_retval = ToNewUnicode(m_RowCells[dbRow][dbCell]);
      //if(!*_retval) return NS_ERROR_OUT_OF_MEMORY;
      *_retval = NS_CONST_CAST(PRUnichar *, PromiseFlatString(m_RowCells[dbRow][dbCell]).get());
    }
    else
    {
      *_retval = nsnull;
    }
  }
  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* noscript wstring GetRowCellByColumnPtr (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumnPtr(PRInt32 dbRow, const nsAString &dbColumn, PRUnichar **_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  PRInt32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCellPtr(dbRow, dbCell, _retval);
} //GetRowCellByColumnPtr

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseResult::ClearResultSet()
{
  nsAutoLock cLock(m_pColumnNamesLock);
  nsAutoLock rLock(m_pRowCellsLock);
  nsAutoLock mLock(m_pColumnResolveMap);

  m_ColumnNames.clear();
  m_RowCells.clear();
  m_ColumnResolveMap.clear();

  return NS_OK;
} //ClearResultSet

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::AddRow(const std::vector<nsString> &vCellValues)
{
  nsAutoLock lock(m_pRowCellsLock);
  m_RowCells.push_back(vCellValues);
  return NS_OK;
} //AddRow

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::DeleteRow(PRInt32 dbRow)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  {
    nsAutoLock lock(m_pRowCellsLock);

    if((PRUint32)dbRow < m_RowCells.size())
    {
      dbrowcells_t::iterator itRows = m_RowCells.begin();
      itRows += dbRow;

      if(itRows != m_RowCells.end())
        m_RowCells.erase(itRows);
    }
  }
  return NS_OK;
} //DeleteRow

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetColumnNames(const std::vector<nsString> &vColumnNames)
{
  nsAutoLock lock(m_pColumnNamesLock);
  m_ColumnNames = vColumnNames;
  return NS_OK;
} //SetColumnNames

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetColumnName(PRInt32 dbColumn, const nsString &strColumnName)
{
  NS_ENSURE_ARG_MIN(dbColumn, 0);
  nsAutoLock lock(m_pColumnNamesLock);
  m_ColumnNames[dbColumn] = strColumnName;
  return NS_OK;
} //SetColumnName

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetRowCell(PRInt32 dbRow, PRInt32 dbCell, const nsString &strCellValue)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  nsAutoLock lock(m_pRowCellsLock);
  m_RowCells[dbRow][dbCell] = strCellValue;
  return NS_OK;
} //SetRowCell

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetRowCells(PRInt32 dbRow, const std::vector<nsString> &vCellValues)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  nsAutoLock lock(m_pRowCellsLock);
  m_RowCells[dbRow] = vCellValues;
  return NS_OK;
} //SetRowCells

//-----------------------------------------------------------------------------
PRInt32 CDatabaseResult::GetColumnIndexFromName(const nsAString &strColumnName)
{
  RebuildColumnResolveMap();

  nsAutoLock lock(m_pColumnResolveMap);
  
  dbcolumnresolvemap_t::const_iterator itColumnIndex = m_ColumnResolveMap.find(PromiseFlatString(strColumnName));
  if(itColumnIndex != m_ColumnResolveMap.end())
    return itColumnIndex->second;

  return -1;
} //GetColumnIndexFromName

//-----------------------------------------------------------------------------
void CDatabaseResult::RebuildColumnResolveMap()
{
  nsAutoLock cLock(m_pColumnNamesLock);
  nsAutoLock mLock(m_pColumnResolveMap);

  if(m_ColumnNames.size() != m_ColumnResolveMap.size() ||
     m_ColumnResolveMap.size() == 0)
  {
    m_ColumnResolveMap.clear();

    PRUint32 nSize =  (PRUint32)m_ColumnNames.size();
    for(PRUint32 i = 0; i < nSize; i++)
    {
      m_ColumnResolveMap.insert(std::make_pair<nsString, PRInt32>(m_ColumnNames[i], i));
    }
  }
}
