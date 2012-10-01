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

nsString sbLocalDatabaseSQL::MediaItemColumns(bool aIncludeMediaItem)
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

nsString sbLocalDatabaseSQL::SecondaryPropertySelect()
{
  nsString sql =
    NS_LITERAL_STRING("SELECT media_item_id, property_id, obj \
                      FROM resource_properties \
                      WHERE media_item_id IN (");
  for (int i = 0; i < sbLocalDatabaseSQL::SecondaryPropertyBindCount-1; i++) {
    sql.AppendLiteral("?, ");
  }
  sql.AppendLiteral("?)");
  
  return sql;
}

nsString sbLocalDatabaseSQL::MediaItemsFtsAllDelete()
{
  nsString sql = NS_LITERAL_STRING("DELETE FROM resource_properties_fts_all \
                                    WHERE rowid = ?");
  return sql;
}

nsString sbLocalDatabaseSQL::MediaItemSelect()
{
  nsString result(NS_LITERAL_STRING("SELECT "));
  result.Append(MediaItemColumns(PR_TRUE));
  result.AppendLiteral(" FROM media_items WHERE guid IN (");
  for (int i = 0; i < sbLocalDatabaseSQL::MediaItemBindCount-1; i++) {
    result.AppendLiteral("?, ");
  }
  result.AppendLiteral("?)");

  return result;
}

nsString sbLocalDatabaseSQL::MediaItemsFtsAllInsert()
{
  nsString sql =
    NS_LITERAL_STRING("INSERT INTO resource_properties_fts_all \
                        (rowid, alldata) VALUES (?, ?)");
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
                            (media_item_id, property_id, obj, obj_searchable, obj_sortable, obj_secondary_sortable) \
                            VALUES (?, ?, ?, ?, ?, ?)");
}


nsString sbLocalDatabaseSQL::PropertiesDelete()
{
  return NS_LITERAL_STRING("DELETE FROM resource_properties WHERE media_item_id = ? AND property_id = ? ");
}

