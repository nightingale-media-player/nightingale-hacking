#ifndef SBLOCALDATABASESQL_H_
#define SBLOCALDATABASESQL_H_

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

#include <nsStringAPI.h>
#include <nsTArray.h>

/**
 * A collection of SQL statements. Most members are static as they cheaply
 * return the strings. A few are more expensive so we cache the results
 * in the instance. So for those you'll need to keep an instance of the
 * class around.
 */
class sbLocalDatabaseSQL
{
public:
  /**
   * Returns an SQL string that selects a single media item.
   */
  nsString MediaItemSelect();
  /**
   * Selects secondary properties for the given media
   */
  static nsString SecondaryPropertySelect();
  /**
   * Removes the items with id's matching those found in aRowIDs from the
   * resource_properties_fts table
   */
  static nsString MediaItemsFtsAllDelete();
  /**
   * Populates the resource_properties_fts from the resource_properties table
   * for the ID's given in aRowIDs
   */
  static nsString MediaItemsFtsAllInsert();
  /**
   * Retrieves the list of properties for the library
   */
  static nsString LibraryMediaItemsPropertiesSelect();
  /**
   * SQL statement to return the library media item
   */
  nsString LibraryMediaItemSelect();
  /**
   * Inserts a property into the property table. The property name
   * is the parameter.
   */
  static nsString PropertiesTableInsert();
  /**
   * Returns all the properties in the system (property_id, property_name)
   */
  static nsString PropertiesSelect();
  /**
   * Inserts a property into the resource_properties table
   */
  static nsString PropertiesInsert();
  /**
   * Removes a property given the item ID and property ID
   */
  static nsString PropertiesDelete();

  // These are the number of "IN" bind variables for statements which use them.
  // They are tuned to optimize performance.
  static const int MediaItemBindCount = 50;
  static const int SecondaryPropertyBindCount = 50;

private:
  nsString mMediaItemColumns;
  nsString mMediaItemColumnsWithID;
  nsString mLibraryMediaItemSelect;

  nsString MediaItemColumns(bool aIncludeMediaItem);
};

#endif /* SBLOCALDATABASEPRIMARYPROPERTYSELECT_H_ */
