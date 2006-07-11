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

#include <prlock.h>

#include <nscore.h>
#include <string/nsString.h>

// DEFINES ====================================================================
#define SONGBIRD_DATABASERESULT_CONTRACTID                \
  "@songbirdnest.com/Songbird/DatabaseResult;1"
#define SONGBIRD_DATABASERESULT_CLASSNAME                 \
  "Songbird Database Result Interface"
#define SONGBIRD_DATABASERESULT_CID                       \
{ /* 9079e1b1-01e9-4cb8-9f2a-08fdf69597cf */              \
  0x9079e1b1,                                             \
  0x1e9,                                                  \
  0x4cb8,                                                 \
  {0x9f, 0x2a, 0x8, 0xfd, 0xf6, 0x95, 0x97, 0xcf}         \
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

protected:
  typedef std::vector<nsString> dbcolumnnames_t;
  typedef std::vector< std::vector<nsString> > dbrowcells_t;
  
  dbcolumnnames_t m_ColumnNames;
  PRLock* m_pColumnNamesLock;
  
  dbrowcells_t m_RowCells;
  PRLock* m_pRowCellsLock;
  
};

#endif // __DATABASE_RESULT_H__

