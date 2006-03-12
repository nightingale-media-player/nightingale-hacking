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
* \file  DatabaseQuery.h
* \brief Songbird Database Object Definition.
*/

#pragma once

// INCLUDES ===================================================================
#include "IDatabaseResult.h"

#include <string>
#include <vector>

#include <prlock.h>

#ifndef PRUSTRING_DEFINED
#define PRUSTRING_DEFINED
#include <string>
#include "nscore.h"
namespace std
{
  typedef basic_string< PRUnichar > prustring;
};
#endif


// DEFINES ====================================================================
#define SONGBIRD_DATABASERESULT_CONTRACTID  "@songbird.org/Songbird/DatabaseResult;1"
#define SONGBIRD_DATABASERESULT_CLASSNAME   "Songbird Database Result Interface"

// {D711447E-B6B0-4e63-A50E-3523794E08DF}
#define SONGBIRD_DATABASERESULT_CID { 0xd711447e, 0xb6b0, 0x4e63, { 0xa5, 0xe, 0x35, 0x23, 0x79, 0x4e, 0x8, 0xdf } }

// CLASSES ====================================================================
class CDatabaseResult : public sbIDatabaseResult
{
friend class CDatabaseQuery;
public:
  CDatabaseResult();
  virtual ~CDatabaseResult();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASERESULT

  nsresult AddRow(const std::vector<std::prustring> &vCellValues);
  nsresult DeleteRow(PRInt32 dbRow);

  nsresult SetColumnNames(const std::vector<std::prustring> &vColumnNames);
  nsresult SetColumnName(PRInt32 dbColumn, const std::prustring &strColumnName);

  nsresult SetRowCell(PRInt32 dbRow, PRInt32 dbCell, const std::prustring &strCellValue);
  nsresult SetRowCells(PRInt32 dbRow, const std::vector<std::prustring> &vCellValues);

  PRInt32 GetColumnIndexFromName(const std::prustring &strColumnName);

protected:
  typedef std::vector<std::prustring> dbcolumnnames_t;
  typedef std::vector<std::vector<std::prustring> > dbrowcells_t;
  
  dbcolumnnames_t m_ColumnNames;
  PRLock* m_pColumnNamesLock;
  
  dbrowcells_t m_RowCells;
  PRLock* m_pRowCellsLock;
  
};