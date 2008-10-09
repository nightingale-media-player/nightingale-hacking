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

#include "sbLocalDatabaseSQL.h"

#include <nsMemory.h>

#include <sbStringUtils.h>

#include "sbLocalDatabaseSchemaInfo.h"

inline
nsString ExtractColumnFromProperty(sbStaticProperty const & aProperty)
{
  return NS_ConvertASCIItoUTF16(aProperty.mColumn);
}

template <class T>
static
nsString AppendNumbers(nsString & aString, T const & aNumbers)
{
  NS_NAMED_LITERAL_STRING(COMMA, ",");
  PRUint32 const length = aNumbers.Length();
  for (PRUint32 index = 0; index < length; ++index) {
    if (index > 0)
      aString.Append(COMMA);
    aString.AppendInt(aNumbers[index]);
  }
  return aString;
}

/**
 * Simple concatenate function since there's no + operator and we can't use
 * Append below
 */
inline
nsString Concat(nsAString const & aLeft, nsAString const & aRight)
{
  nsString temp(aLeft);
  temp.Append(aRight);
  return temp;
}

static
nsString sbStaticPropertyColumns()
{
  nsString result;
  NS_NAMED_LITERAL_STRING(COMMA, ",");

  for (PRUint32 property = 0; property < NS_ARRAY_LENGTH(sStaticProperties); ++property) {
    if (property != 0) {
      result.Append(COMMA);
    }
    result.AppendLiteral(sStaticProperties[property].mColumn);
  }
  return result;
}

nsString sbLocalDatabaseSQL::MediaItemColumns(PRBool aIncludeMediaItem)
{
  if (mMediaItemColumns.IsEmpty()) {
    mMediaItemColumns= sbStaticPropertyColumns();
  }
  if (mMediaItemColumnsWithID.IsEmpty()) {
    mMediaItemColumnsWithID = Concat(NS_LITERAL_STRING("media_item_id, "),
                                 mMediaItemColumns);
  }
  return aIncludeMediaItem ? mMediaItemColumnsWithID : mMediaItemColumns;
}

nsString sbLocalDatabaseSQL::SecondaryPropertySelect(nsTArray<PRUint32> const & aIDArray)
{
  NS_ASSERTION(aIDArray.Length() > 0,
               "sbLocalDatabaseSQL::MediaItemSelect must be called with at least one guid");

  nsString sql =
    NS_LITERAL_STRING("SELECT media_item_id, property_id, obj, obj_sortable \
                       FROM resource_properties \
                       WHERE media_item_id ");
  if (aIDArray.Length() > 1) {
    sql.AppendLiteral("in (");
    AppendNumbers(sql, aIDArray);
    sql.AppendLiteral(")");
  }
  else {
    sql.AppendLiteral("= ");
    sql.AppendInt(aIDArray[0]);
  }
  return sql;
}

nsString sbLocalDatabaseSQL::MediaItemsFtsAllDelete(nsTArray<PRUint32> aRowIDs)
{
  NS_ASSERTION(aRowIDs.Length() > 0,
               "sbLocalDatabaseSQL::DeleteMediaItemsFtsAll must be passed at least one row id");
  nsString sql = NS_LITERAL_STRING("DELETE FROM resource_properties_fts_all \
                                    WHERE rowid ");
  if (aRowIDs.Length() > 1) {
    sql.AppendLiteral("in (");
    AppendNumbers(sql, aRowIDs);
    sql.AppendLiteral(")");
  }
  else {
    sql.AppendLiteral("= ");
    sql.AppendInt(aRowIDs[0]);
  }
  return sql;
}

nsString sbLocalDatabaseSQL::MediaItemsFtsAllInsert(nsTArray<PRUint32> aRowIDs)
{
  nsString sql =
    NS_LITERAL_STRING("INSERT INTO resource_properties_fts_all \
                        (rowid, alldata) \
                       SELECT media_item_id, group_concat(obj_sortable) \
                       FROM resource_properties \
                       WHERE media_item_id ");

  if (aRowIDs.Length() > 1) {
    sql.AppendLiteral("in (");
    AppendNumbers(sql, aRowIDs);
    sql.AppendLiteral(")");
  }
  else {
    sql.AppendLiteral("= ");
    sql.AppendInt(aRowIDs[0]);
  }
  sql.AppendLiteral(" GROUP BY media_item_id");
  return sql;
}

nsString sbLocalDatabaseSQL::LibraryMediaItemsPropertiesSelect()
{
  return NS_LITERAL_STRING("SELECT property_id, obj  \
                            FROM resource_properties  \
                            WHERE media_item_id = 0");
}

nsString sbLocalDatabaseSQL::LibraryMediaItemSelect()
{
  if (mLibraryMediaItemSelect.IsEmpty()) {
    mLibraryMediaItemSelect.AppendLiteral("SELECT ");
    mLibraryMediaItemSelect.Append(MediaItemColumns(PR_FALSE));
    mLibraryMediaItemSelect.AppendLiteral(" FROM library_media_item");
  }
  return mLibraryMediaItemSelect;
}

nsString sbLocalDatabaseSQL::PropertiesTableInsert()
{
  return NS_LITERAL_STRING("INSERT INTO properties \
                            (property_name) \
                            VALUES (?)");
}

nsString sbLocalDatabaseSQL::PropertiesSelect()
{
  return NS_LITERAL_STRING("SELECT property_id, property_name \
                            FROM properties");
}

nsString sbLocalDatabaseSQL::PropertiesInsert()
{
  return NS_LITERAL_STRING("INSERT OR REPLACE INTO resource_properties \
                            (media_item_id, property_id, obj, obj_sortable) \
                            VALUES (?, ?, ?, ?)");
}


nsString sbLocalDatabaseSQL::PropertiesDelete(PRUint32 aMediaItemID, PRUint32 aPropertyID)
{
  nsString sql = NS_LITERAL_STRING("DELETE FROM resource_properties WHERE ");
  sql.AppendLiteral("media_item_id = ");
  sql.AppendInt(aMediaItemID);
  sql.AppendLiteral(" AND property_id = ");
  sql.AppendInt(aPropertyID);
  return sql;
}

