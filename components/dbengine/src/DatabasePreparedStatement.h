/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/** 
 * \file  DatabasePreparedStatement.h
 * \brief Nightingale Database Prepared Query.
 */

#ifndef __DATABASE_PREPAREDSTATEMENT_H__
#define __DATABASE_PREPAREDSTATEMENT_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include "sqlite3.h"

#include "DatabaseQuery.h"
#include "DatabaseEngine.h"

#include <prmon.h>
#include <prlog.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include "sbIDatabasePreparedStatement.h"

// CLASSES ====================================================================
class CDatabasePreparedStatement : public sbIDatabasePreparedStatement
{

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASEPREPAREDSTATEMENT

  CDatabasePreparedStatement(const nsAString &sql);
  virtual ~CDatabasePreparedStatement();
  
  sqlite3_stmt* GetStatement(sqlite3 *db);

protected:
  CDatabaseQuery *mQuery;
  sqlite3_stmt *mStatement;
  nsString mSql;
};

#endif // __DATABASE_PREPAREDSTATEMENT_H__

