/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
} //ctor

//-----------------------------------------------------------------------------
CDatabaseResult::~CDatabaseResult()
{
} //dtor

//-----------------------------------------------------------------------------
/* PRInt32 GetColumnCount (); */
NS_IMETHODIMP CDatabaseResult::GetColumnCount(PRInt32 *_retval)
{
  *_retval = 0;

  {
    sbCommon::CScopedLock lock(&m_ColumnNamesLock);
    *_retval = m_ColumnNames.size();
  }

  return NS_OK;
} //GetColumnCount

//-----------------------------------------------------------------------------
/* wstring GetColumnName (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnName(PRInt32 dbColumn, PRUnichar **_retval)
{
  *_retval = NULL;

  {
    sbCommon::CScopedLock lock(&m_ColumnNamesLock);

    if(dbColumn < m_ColumnNames.size())
    {
      size_t nLen = m_ColumnNames[dbColumn].length() + 1;
      *_retval = (PRUnichar *) nsMemory::Clone(m_ColumnNames[dbColumn].c_str(), nLen * sizeof(PRUnichar));
      if(*_retval == NULL) return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* PRInt32 GetRowCount (); */
NS_IMETHODIMP CDatabaseResult::GetRowCount(PRInt32 *_retval)
{
  *_retval = 0;

  {
    sbCommon::CScopedLock lock(&m_RowCellsLock);
    *_retval = m_RowCells.size();
  }

  return NS_OK;
} //GetRowCount

//-----------------------------------------------------------------------------
/* wstring GetRowCell (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCell(PRInt32 dbRow, PRInt32 dbCell, PRUnichar **_retval)
{
  *_retval = NULL;

  {
    sbCommon::CScopedLock lock(&m_RowCellsLock);
    if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size())
    {
      size_t nLen = m_RowCells[dbRow][dbCell].length() + 1;
      *_retval = (PRUnichar *) nsMemory::Clone(m_RowCells[dbRow][dbCell].c_str(), nLen * sizeof(PRUnichar));
      if(*_retval == NULL) return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* wstring GetRowCellByColumn (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumn(PRInt32 dbRow, const PRUnichar *dbColumn, PRUnichar **_retval)
{
  PRInt32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCell(dbRow, dbCell, _retval);
} //GetRowCellByColumn

//-----------------------------------------------------------------------------
/* wstring GetColumnNamePtr (in PRInt32 dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetColumnNamePtr(PRInt32 dbColumn, PRUnichar **_retval)
{
  *_retval = NULL;

  {
    sbCommon::CScopedLock lock(&m_ColumnNamesLock);
    if(dbColumn < m_ColumnNames.size())
    {
      *_retval = const_cast<PRUnichar *>(m_ColumnNames[dbColumn].c_str());
    }
  }

  return NS_OK;
} //GetColumnName

//-----------------------------------------------------------------------------
/* wstring GetRowCellPtr (in PRInt32 dbRow, in PRInt32 dbCell); */
NS_IMETHODIMP CDatabaseResult::GetRowCellPtr(PRInt32 dbRow, PRInt32 dbCell, PRUnichar **_retval)
{
  *_retval = NULL;

  {
    sbCommon::CScopedLock lock(&m_RowCellsLock);
    if(dbRow < m_RowCells.size() && dbCell < m_RowCells[dbRow].size())
    {
      *_retval = const_cast<PRUnichar *>(m_RowCells[dbRow][dbCell].c_str());
    }
  }

  return NS_OK;
} //GetRowCell

//-----------------------------------------------------------------------------
/* noscript wstring GetRowCellByColumnPtr (in PRInt32 dbRow, in wstring dbColumn); */
NS_IMETHODIMP CDatabaseResult::GetRowCellByColumnPtr(PRInt32 dbRow, const PRUnichar *dbColumn, PRUnichar **_retval)
{
  PRInt32 dbCell = GetColumnIndexFromName(dbColumn);
  return GetRowCellPtr(dbRow, dbCell, _retval);
} //GetRowCellByColumnPtr

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseResult::ClearResultSet()
{
  m_ColumnNamesLock.Lock();
  m_RowCellsLock.Lock();

  m_ColumnNames.clear();
  m_RowCells.clear();

  m_RowCellsLock.Unlock();
  m_ColumnNamesLock.Unlock();

  return NS_OK;
} //ClearResultSet

//-----------------------------------------------------------------------------
void CDatabaseResult::AddRow(const std::vector<std::prustring> &vCellValues)
{
  {
    sbCommon::CScopedLock lock(&m_RowCellsLock);
    m_RowCells.push_back(vCellValues);
  }
} //AddRow

//-----------------------------------------------------------------------------
void CDatabaseResult::DeleteRow(PRInt32 dbRow)
{
  {
    sbCommon::CScopedLock lock(&m_RowCellsLock);

    if(dbRow < m_RowCells.size())
    {
      dbrowcells_t::iterator itRows = m_RowCells.begin();
      itRows += dbRow;

      if(itRows != m_RowCells.end())
        m_RowCells.erase(itRows);
    }
  }
} //DeleteRow

//-----------------------------------------------------------------------------
void CDatabaseResult::SetColumnNames(const std::vector<std::prustring> &vColumnNames)
{
  sbCommon::CScopedLock lock(&m_ColumnNamesLock);
  m_ColumnNames = vColumnNames;
} //SetColumnNames

//-----------------------------------------------------------------------------
void CDatabaseResult::SetColumnName(PRInt32 dbColumn, const std::prustring &strColumnName)
{
  sbCommon::CScopedLock lock(&m_ColumnNamesLock);
  m_ColumnNames[dbColumn] = strColumnName;
} //SetColumnName

//-----------------------------------------------------------------------------
void CDatabaseResult::SetRowCell(PRInt32 dbRow, PRInt32 dbCell, const std::prustring &strCellValue)
{
  sbCommon::CScopedLock lock(&m_RowCellsLock);
  m_RowCells[dbRow][dbCell] = strCellValue;
} //SetRowCell

//-----------------------------------------------------------------------------
void CDatabaseResult::SetRowCells(PRInt32 dbRow, const std::vector<std::prustring> &vCellValues)
{
  sbCommon::CScopedLock lock(&m_RowCellsLock);
  m_RowCells[dbRow] = vCellValues;
} //SetRowCells

//-----------------------------------------------------------------------------
PRInt32 CDatabaseResult::GetColumnIndexFromName(const std::prustring &strColumnName)
{
  sbCommon::CScopedLock lock(&m_ColumnNamesLock);
  PRUint32 nSize =  m_ColumnNames.size();

  for(PRUint32 i = 0; i < nSize; i++)
  {
    if(strColumnName == m_ColumnNames[i])
      return i;
  }
  
  return -1;
} //GetColumnIndexFromName
