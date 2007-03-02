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

/** 
 * \file DatabaseResult.cpp
 * \brief Database Result Object Implementation.
 */

#include "DatabaseResult.h"
#include <prmem.h>
#include <xpcom/nsMemory.h>

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

// CLASSES ====================================================================
//=============================================================================
// CDatabaseQuery Class
//=============================================================================
//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseResult, sbIDatabaseResult)

//-----------------------------------------------------------------------------
CDatabaseResult::CDatabaseResult()
{
#ifdef PR_LOGGING
  if (!gDatabaseResultLog)
    gDatabaseResultLog = PR_NewLogModule("sbDatabaseResult");
#endif
  m_pColumnNamesLock = PR_NewLock();
  m_pRowCellsLock = PR_NewLock();
  m_pColumnResolveMap = PR_NewLock();

  NS_ASSERTION(m_pColumnNamesLock, "CDatabaseResult.m_pColumnNamesLock failed");
  NS_ASSERTION(m_pRowCellsLock, "CDatabaseResult.m_pRowCellsLock failed");
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
NS_IMETHODIMP CDatabaseResult::GetColumnCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pColumnNamesLock);
  *_retval = m_ColumnNames.size();
  PR_Unlock(m_pColumnNamesLock);
  return NS_OK;
} //GetColumnCount

//-----------------------------------------------------------------------------
/* wstring GetColumnName (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnName(PRUint32 dbColumn, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(dbColumn, 0);

  PR_Lock(m_pColumnNamesLock);
  if(dbColumn < m_ColumnNames.size()) {
    _retval = m_ColumnNames[dbColumn];
  }
  PR_Unlock(m_pColumnNamesLock);

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

  PR_Lock(m_pRowCellsLock);
  *_retval = m_RowCells.size();
  PR_Unlock(m_pRowCellsLock);

  return NS_OK;
} //GetRowCount

//-----------------------------------------------------------------------------
/* wstring GetRowCell (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCell(PRUint32 dbRow, PRUint32 dbCell, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);

  PR_Lock(m_pRowCellsLock);
  if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size()) {
    _retval = m_RowCells[dbRow][dbCell];
  }
  PR_Unlock(m_pRowCellsLock);

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* wstring GetRowCellByColumn (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumn(PRUint32 dbRow, const nsAString &dbColumn, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  PRUint32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCell(dbRow, dbCell, _retval);
} //GetRowCellByColumn

//-----------------------------------------------------------------------------
/* wstring GetColumnNamePtr (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnNamePtr(PRUint32 dbColumn, PRUnichar **_retval)
{
  NS_ENSURE_ARG_MIN(dbColumn, 0);
  PR_Lock(m_pColumnNamesLock);
  if(dbColumn < m_ColumnNames.size())
  {
    *_retval = NS_CONST_CAST(PRUnichar *, PromiseFlatString(m_ColumnNames[dbColumn]).get());
  }
  else
  {
    *_retval = nsnull;
  }
  PR_Unlock(m_pColumnNamesLock);
  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* wstring GetRowCellPtr (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCellPtr(PRUint32 dbRow, PRUint32 dbCell, PRUnichar **_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  NS_ENSURE_ARG_MIN(dbCell, 0);

  PR_Lock(m_pRowCellsLock);
  if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size())
  {
    *_retval = NS_CONST_CAST(PRUnichar *, PromiseFlatString(m_RowCells[dbRow][dbCell]).get());
  }
  else
  {
    *_retval = nsnull;
  }
  PR_Unlock(m_pRowCellsLock);

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* noscript wstring GetRowCellByColumnPtr (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumnPtr(PRUint32 dbRow, const nsAString &dbColumn, PRUnichar **_retval)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  PRUint32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCellPtr(dbRow, dbCell, _retval);
} //GetRowCellByColumnPtr

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseResult::ClearResultSet()
{
  PR_Lock(m_pColumnNamesLock);
  PR_Lock(m_pRowCellsLock);
  PR_Lock(m_pColumnResolveMap);

  m_ColumnNames.clear();
  m_RowCells.clear();
  m_ColumnResolveMap.clear();

  PR_Unlock(m_pColumnResolveMap);
  PR_Unlock(m_pRowCellsLock);
  PR_Unlock(m_pColumnNamesLock);

  return NS_OK;
} //ClearResultSet

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::AddRow(const std::vector<nsString> &vCellValues)
{
  PR_Lock(m_pRowCellsLock);
  m_RowCells.push_back(vCellValues);
  PR_Unlock(m_pRowCellsLock);
  return NS_OK;
} //AddRow

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::DeleteRow(PRUint32 dbRow)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);

  PR_Lock(m_pRowCellsLock);

  if(dbRow < m_RowCells.size())
  {
    dbrowcells_t::iterator itRows = m_RowCells.begin();
    itRows += dbRow;

    if(itRows != m_RowCells.end())
      m_RowCells.erase(itRows);
  }

  PR_Unlock(m_pRowCellsLock);

  return NS_OK;
} //DeleteRow

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetColumnNames(const std::vector<nsString> &vColumnNames)
{
  PR_Lock(m_pColumnNamesLock);
  m_ColumnNames = vColumnNames;
  PR_Unlock(m_pColumnNamesLock);
  return NS_OK;
} //SetColumnNames

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetColumnName(PRUint32 dbColumn, const nsString &strColumnName)
{
  NS_ENSURE_ARG_MIN(dbColumn, 0);
  PR_Lock(m_pColumnNamesLock);
  m_ColumnNames[dbColumn] = strColumnName;
  PR_Unlock(m_pColumnNamesLock);
  return NS_OK;
} //SetColumnName

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetRowCell(PRUint32 dbRow, PRUint32 dbCell, const nsString &strCellValue)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  PR_Lock(m_pRowCellsLock);
  m_RowCells[dbRow][dbCell] = strCellValue;
  PR_Unlock(m_pRowCellsLock);
  return NS_OK;
} //SetRowCell

//-----------------------------------------------------------------------------
nsresult CDatabaseResult::SetRowCells(PRUint32 dbRow, const std::vector<nsString> &vCellValues)
{
  NS_ENSURE_ARG_MIN(dbRow, 0);
  PR_Lock(m_pRowCellsLock);
  m_RowCells[dbRow] = vCellValues;
  PR_Unlock(m_pRowCellsLock);
  return NS_OK;
} //SetRowCells

//-----------------------------------------------------------------------------
PRUint32 CDatabaseResult::GetColumnIndexFromName(const nsAString &strColumnName)
{
  RebuildColumnResolveMap();
  PRUint32 retval = -1;

  PR_Lock(m_pColumnResolveMap);

  dbcolumnresolvemap_t::const_iterator itColumnIndex =
    m_ColumnResolveMap.find(PromiseFlatString(strColumnName));

  if(itColumnIndex != m_ColumnResolveMap.end())
    retval = itColumnIndex->second;

  PR_Unlock(m_pColumnResolveMap);

  return retval;
} //GetColumnIndexFromName

//-----------------------------------------------------------------------------
void CDatabaseResult::RebuildColumnResolveMap()
{
  PR_Lock(m_pColumnNamesLock);
  PR_Lock(m_pColumnResolveMap);

  if(m_ColumnNames.size() != m_ColumnResolveMap.size() ||
     m_ColumnResolveMap.size() == 0)
  {
    m_ColumnResolveMap.clear();

    PRUint32 nSize =  m_ColumnNames.size();
    for(PRUint32 i = 0; i < nSize; i++)
    {
      m_ColumnResolveMap.insert(std::make_pair<nsString, PRUint32>(m_ColumnNames[i], i));
    }
  }
  PR_Unlock(m_pColumnResolveMap);
  PR_Unlock(m_pColumnNamesLock);
}
