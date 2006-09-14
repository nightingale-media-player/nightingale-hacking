/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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
* \file  DatabaseResult.h
* \brief Songbird Database Object Definition.
*/

#ifndef __DATABASE_RESULT_H__
#define __DATABASE_RESULT_H__

// INCLUDES ===================================================================
#include "sbIDatabaseResult.h"

#include <string>
#include <vector>
#include <map>

#include <prlock.h>

#include <nscore.h>
#include <string/nsString.h>

// DEFINES ====================================================================
#define SONGBIRD_DATABASERESULT_CONTRACTID                \
  "@songbirdnest.com/Songbird/DatabaseResult;1"
#define SONGBIRD_DATABASERESULT_CLASSNAME                 \
  "Songbird Database Result Interface"
#define SONGBIRD_DATABASERESULT_CID                       \
{ /* 9a5fcaf6-4441-11db-9651-00e08161165f */              \
  0x9a5fcaf6,                                             \
  0x4441,                                                 \
  0x11db,                                                 \
	{0x96, 0x51, 0x0, 0xe0, 0x81, 0x61, 0x16, 0x5f}         \
}

// CLASSES ====================================================================
class CDatabaseResult : public sbIDatabaseResult
{
friend class CDatabaseQuery;
public:
  CDatabaseResult();
  virtual ~CDatabaseResult();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASERESULT

  nsresult AddRow(const std::vector<nsString> &vCellValues);
  nsresult DeleteRow(PRInt32 dbRow);

  nsresult SetColumnNames(const std::vector<nsString> &vColumnNames);
  nsresult SetColumnName(PRInt32 dbColumn, const nsString &strColumnName);

  nsresult SetRowCell(PRInt32 dbRow, PRInt32 dbCell, const nsString &strCellValue);
  nsresult SetRowCells(PRInt32 dbRow, const std::vector<nsString> &vCellValues);

  PRInt32 GetColumnIndexFromName(const nsAString &strColumnName);
  void RebuildColumnResolveMap();

protected:
  typedef std::vector<nsString> dbcolumnnames_t;
  typedef std::vector< std::vector<nsString> > dbrowcells_t;
  typedef std::map<nsString, PRInt32> dbcolumnresolvemap_t;
  
  dbcolumnnames_t m_ColumnNames;
  PRLock* m_pColumnNamesLock;
  
  dbrowcells_t m_RowCells;
  PRLock* m_pRowCellsLock;
  
  dbcolumnresolvemap_t m_ColumnResolveMap;
  PRLock* m_pColumnResolveMap;
};

#endif // __DATABASE_RESULT_H__

