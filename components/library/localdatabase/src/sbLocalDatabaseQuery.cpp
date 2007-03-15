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

#include "sbLocalDatabaseQuery.h"

#include <sbSQLBuilderCID.h>

#define COUNT_COLUMN       NS_LITERAL_STRING("count(1)")
#define GUID_COLUMN        NS_LITERAL_STRING("guid")
#define OBJSORTABLE_COLUMN NS_LITERAL_STRING("obj_sortable")
#define MEDIAITEMID_COLUMN NS_LITERAL_STRING("media_item_id")
#define PROPERTYID_COLUMN  NS_LITERAL_STRING("property_id")
#define ORDINAL_COLUMN     NS_LITERAL_STRING("ordinal")

#define PROPERTIES_TABLE       NS_LITERAL_STRING("resource_properties")
#define MEDIAITEMS_TABLE       NS_LITERAL_STRING("media_items")
#define SIMPLEMEDIALISTS_TABLE NS_LITERAL_STRING("simple_media_lists")

#define BASE_ALIAS       NS_LITERAL_STRING("_base")
#define MEDIAITEMS_ALIAS NS_LITERAL_STRING("_mi")
#define CONSTRAINT_ALIAS NS_LITERAL_STRING("_con")
#define GETNOTNULL_ALIAS NS_LITERAL_STRING("_getnotnull")
#define GETNULL_ALIAS    NS_LITERAL_STRING("_getnull")
#define SORT_ALIAS       NS_LITERAL_STRING("_sort")

#define ORDINAL_PROPERTY \
  NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#ordinal")

sbLocalDatabaseQuery::sbLocalDatabaseQuery(const nsAString& aBaseTable,
                                           const nsAString& aBaseConstraintColumn,
                                           PRUint32 aBaseConstraintValue,
                                           const nsAString& aBaseForeignKeyColumn,
                                           const nsAString& aPrimarySortProperty,
                                           PRBool aPrimarySortAscending,
                                           nsTArray<FilterSpec>* aFilters) :
  mBaseTable(aBaseTable),
  mBaseConstraintColumn(aBaseConstraintColumn),
  mBaseConstraintValue(aBaseConstraintValue),
  mBaseForeignKeyColumn(aBaseForeignKeyColumn),
  mPrimarySortProperty(aPrimarySortProperty),
  mPrimarySortAscending(aPrimarySortAscending),
  mFilters(aFilters)
{
  mIsFullLibrary = mBaseTable.Equals(MEDIAITEMS_TABLE);

  mBuilder = do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID);
  //NS_ENSURE_SUCCESS(rv, rv);

}

sbLocalDatabaseQuery::~sbLocalDatabaseQuery()
{
}

NS_IMETHODIMP
sbLocalDatabaseQuery::GetFullCountQuery(nsAString& aQuery)
{
  nsresult rv;

  rv = AddCountColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddBaseTable();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->ToString(aQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::GetFullGuidRangeQuery(nsAString& aQuery)
{
  nsresult rv;

  rv = AddGuidColumns(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddBaseTable();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddPrimarySort();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddRange();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->ToString(aQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::GetNonNullCountQuery(nsAString& aQuery)
{
  nsresult rv;

  rv = AddCountColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddBaseTable();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddNonNullPrimarySortConstraint();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->ToString(aQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::GetNullGuidRangeQuery(nsAString& aQuery)
{
  nsresult rv;

  rv = AddGuidColumns(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddBaseTable();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddJoinToGetNulls();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddRange();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->ToString(aQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddCountColumns()
{
  nsresult rv;

  rv = mBuilder->AddColumn(EmptyString(), COUNT_COLUMN);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddGuidColumns(PRBool aIsNull)
{
  nsresult rv;

  rv = mBuilder->AddColumn(MEDIAITEMS_ALIAS, GUID_COLUMN);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aIsNull) {
    rv = mBuilder->AddColumn(EmptyString(), NS_LITERAL_STRING("''"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    if (IsTopLevelProperty(mPrimarySortProperty)) {
      nsAutoString columnName;
      rv = GetTopLevelPropertyColumn(mPrimarySortProperty, columnName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mBuilder->AddColumn(MEDIAITEMS_ALIAS, columnName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      if (mPrimarySortProperty.Equals(ORDINAL_PROPERTY)) {

        if (mBaseTable.Equals(SIMPLEMEDIALISTS_TABLE)) {
          rv = mBuilder->AddColumn(CONSTRAINT_ALIAS,
                                   ORDINAL_COLUMN);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        rv = mBuilder->AddColumn(SORT_ALIAS, OBJSORTABLE_COLUMN);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddBaseTable()
{
  nsresult rv;

  if (mIsFullLibrary) {
    rv = mBuilder->SetBaseTableName(MEDIAITEMS_TABLE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBuilder->SetBaseTableAlias(MEDIAITEMS_ALIAS);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    /*
     * Join the base table to the media items table and add the needed
     * constraint.
     */
    rv = mBuilder->SetBaseTableName(mBaseTable);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBuilder->SetBaseTableAlias(CONSTRAINT_ALIAS);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = mBuilder->CreateMatchCriterionLong(CONSTRAINT_ALIAS,
                                            mBaseConstraintColumn,
                                            sbISQLSelectBuilder::MATCH_EQUALS,
                                            mBaseConstraintValue,
                                            getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                           MEDIAITEMS_TABLE,
                           MEDIAITEMS_ALIAS,
                           MEDIAITEMID_COLUMN,
                           CONSTRAINT_ALIAS,
                           mBaseForeignKeyColumn);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddFilters()
{
  nsresult rv;

  PRUint32 len = mFilters->Length();
  for (PRUint32 i = 0; i < len; i++) {
    const FilterSpec& fs = mFilters->ElementAt(i);

    nsAutoString tableAlias;
    tableAlias.AppendLiteral("_p");
    tableAlias.AppendInt(i);

    PRBool isTopLevelProperty = IsTopLevelProperty(fs.property);

    // Add the critera
    nsCOMPtr<sbISQLBuilderCriterion> criterion;

    if (fs.isSearch) {

      nsAutoString searchTerm;
      searchTerm.AppendLiteral("%");
      searchTerm.Append(fs.values[0]);
      searchTerm.AppendLiteral("%");

      // XXX: Lets not support search on top level properties
      if (isTopLevelProperty) {
        return NS_ERROR_INVALID_ARG;
      }

      /*
       * If no property is specified, we need to search all properties.  Use
       * a subquery here to prevent duplicates
       */
      if (fs.property.IsEmpty()) {
        nsCOMPtr<sbISQLSelectBuilder> builder =
          do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->AddColumn(EmptyString(), GUID_COLUMN);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetDistinct(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->SetBaseTableName(PROPERTIES_TABLE);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = builder->CreateMatchCriterionString(EmptyString(),
                                                 OBJSORTABLE_COLUMN,
                                                 sbISQLSelectBuilder::MATCH_LIKE,
                                                 searchTerm,
                                                 getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
        rv = mBuilder->CreateMatchCriterionIn(MEDIAITEMS_ALIAS,
                                              GUID_COLUMN,
                                              getter_AddRefs(inCriterion));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = inCriterion->AddSubquery(builder);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mBuilder->AddCriterion(inCriterion);
        NS_ENSURE_SUCCESS(rv, rv);

      }
      else {
        rv = mBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                               PROPERTIES_TABLE,
                               tableAlias,
                               GUID_COLUMN,
                               MEDIAITEMS_ALIAS,
                               GUID_COLUMN);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mBuilder->CreateMatchCriterionLong(tableAlias,
                                                NS_LITERAL_STRING("property_id"),
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                GetPropertyId(fs.property),
                                                getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        // Add the search term
        rv = mBuilder->CreateMatchCriterionString(tableAlias,
                                                  OBJSORTABLE_COLUMN,
                                                  sbISQLSelectBuilder::MATCH_LIKE,
                                                  searchTerm,
                                                  getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }
    else {
      /*
       * A filter
       */
      nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
      if (isTopLevelProperty) {

        // Add the constraint for the top level table.
        nsAutoString columnName;
        rv = GetTopLevelPropertyColumn(fs.property, columnName);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mBuilder->CreateMatchCriterionIn(MEDIAITEMS_ALIAS,
                                              columnName,
                                              getter_AddRefs(inCriterion));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        rv = mBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                               PROPERTIES_TABLE,
                               tableAlias,
                               GUID_COLUMN,
                               MEDIAITEMS_ALIAS,
                               GUID_COLUMN);
        NS_ENSURE_SUCCESS(rv, rv);

        // Add the propery constraint for the filter
        rv = mBuilder->CreateMatchCriterionLong(tableAlias,
                                                NS_LITERAL_STRING("property_id"),
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                GetPropertyId(fs.property),
                                                getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mBuilder->CreateMatchCriterionIn(tableAlias,
                                              OBJSORTABLE_COLUMN,
                                              getter_AddRefs(inCriterion));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // For each filter value, add the term to an IN constraint
      PRUint32 numValues = fs.values.Length();
      for (PRUint32 j = 0; j < numValues; j++) {
        const nsAString& filterTerm = fs.values[j];
        rv = inCriterion->AddString(filterTerm);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = mBuilder->AddCriterion(inCriterion);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddRange()
{
  nsresult rv;

  rv = mBuilder->SetOffsetIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->SetLimitIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddPrimarySort()
{
  nsresult rv;

  /*
   * If this is a top level propery, simply add the sort
   */
  if (IsTopLevelProperty(mPrimarySortProperty)) {
    nsAutoString columnName;
    rv = GetTopLevelPropertyColumn(mPrimarySortProperty, columnName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBuilder->AddOrder(MEDIAITEMS_ALIAS,
                            columnName,
                            mPrimarySortAscending);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    /*
     * If this is the custom sort, sort by the "ordinal" column in the
     * "simple_media_lists" table.  Make sure that base table is actually
     * the simple media lists table before proceeding.
     */
    if (mPrimarySortProperty.Equals(ORDINAL_PROPERTY)) {
      nsAutoString baseTable;
      rv = mBuilder->GetBaseTableName(baseTable);
      NS_ENSURE_SUCCESS(rv, rv);

      if (baseTable.Equals(SIMPLEMEDIALISTS_TABLE)) {
        rv = mBuilder->AddOrder(CONSTRAINT_ALIAS,
                                ORDINAL_COLUMN,
                                mPrimarySortAscending);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        return NS_ERROR_INVALID_ARG;
      }
    }
    else {
      /*
       * Join an instance of the properties table to the base table
       */
      rv = mBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                             PROPERTIES_TABLE,
                             SORT_ALIAS,
                             GUID_COLUMN,
                             MEDIAITEMS_ALIAS,
                             GUID_COLUMN);
      NS_ENSURE_SUCCESS(rv, rv);

      /*
       * Restrict the sort table to the sort property
       */
      nsCOMPtr<sbISQLBuilderCriterion> criterion;
      rv = mBuilder->CreateMatchCriterionLong(SORT_ALIAS,
                                              PROPERTYID_COLUMN,
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              GetPropertyId(mPrimarySortProperty),
                                              getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mBuilder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);

      /*
       * Add a sort on the primary sort
       */
      rv = mBuilder->AddOrder(SORT_ALIAS,
                              OBJSORTABLE_COLUMN,
                              mPrimarySortAscending);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  /*
   * Sort on media_item_id to make the order of the rows always the same
   * XXX: We could probably remove this in non test builds since it really
   * does not matter.  The only reason I've added this is because the natural
   * database ordering seems to change between architectures and this causes
   * tests to fail that have pre-generated results to test against
   */
  rv = mBuilder->AddOrder(MEDIAITEMS_ALIAS,
                          MEDIAITEMID_COLUMN,
                          PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;

}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddNonNullPrimarySortConstraint()
{
  nsresult rv;

  nsCOMPtr<sbISQLBuilderCriterion> criterion;

  if (IsTopLevelProperty(mPrimarySortProperty)) {
    nsAutoString columnName;
    rv = GetTopLevelPropertyColumn(mPrimarySortProperty, columnName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mBuilder->CreateMatchCriterionNull(MEDIAITEMS_ALIAS,
                                            columnName,
                                            sbISQLSelectBuilder::MATCH_NOTEQUALS,
                                            getter_AddRefs(criterion));
    rv = mBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    if (mPrimarySortProperty.Equals(ORDINAL_PROPERTY)) {

      nsAutoString baseTable;
      rv = mBuilder->GetBaseTableName(baseTable);
      NS_ENSURE_SUCCESS(rv, rv);

      if (baseTable.Equals(SIMPLEMEDIALISTS_TABLE)) {
        rv = mBuilder->CreateMatchCriterionNull(CONSTRAINT_ALIAS,
                                                ORDINAL_COLUMN,
                                                sbISQLSelectBuilder::MATCH_NOTEQUALS,
                                                getter_AddRefs(criterion));
        rv = mBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        return NS_ERROR_INVALID_ARG;
      }
    }
    else {
      rv = mBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                             PROPERTIES_TABLE,
                             GETNOTNULL_ALIAS,
                             GUID_COLUMN,
                             MEDIAITEMS_ALIAS,
                             GUID_COLUMN);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mBuilder->CreateMatchCriterionLong(GETNOTNULL_ALIAS,
                                              PROPERTYID_COLUMN,
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              GetPropertyId(mPrimarySortProperty),
                                              getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mBuilder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::AddJoinToGetNulls()
{
  nsresult rv;

  nsCOMPtr<sbISQLBuilderCriterion> criterionGuid;
  rv = mBuilder->CreateMatchCriterionTable(GETNULL_ALIAS,
                                           GUID_COLUMN,
                                           sbISQLSelectBuilder::MATCH_EQUALS,
                                           MEDIAITEMS_ALIAS,
                                           GUID_COLUMN,
                                           getter_AddRefs(criterionGuid));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionProperty;
  rv = mBuilder->CreateMatchCriterionLong(GETNULL_ALIAS,
                                          PROPERTYID_COLUMN,
                                          sbISQLSelectBuilder::MATCH_EQUALS,
                                          GetPropertyId(mPrimarySortProperty),
                                          getter_AddRefs(criterionProperty));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = mBuilder->CreateAndCriterion(criterionGuid,
                                    criterionProperty,
                                    getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->AddJoinWithCriterion(sbISQLSelectBuilder::JOIN_LEFT,
                                      PROPERTIES_TABLE,
                                      GETNULL_ALIAS,
                                      criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->CreateMatchCriterionNull(GETNULL_ALIAS,
                                          OBJSORTABLE_COLUMN,
                                          sbISQLSelectBuilder::MATCH_EQUALS,
                                          getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mBuilder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
sbLocalDatabaseQuery::IsTopLevelProperty(const nsAString& aProperty)
{
  // XXX: This should be replaced with a call to the properties manager
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#guid") ||
      aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#created") ||
      aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#updated") ||
      aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#contentUrl") ||
      aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#contentMimeType") ||
      aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#contentLength")) {
    return PR_TRUE;
  }
  else {
    return PR_FALSE;
  }

}

PRInt32
sbLocalDatabaseQuery::GetPropertyId(const nsAString& aProperty)
{
  // XXX: This should be replaced with a call to the properties manager
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#trackName")) {
    return 1;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#albumName")) {
    return 2;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#artistName")) {
    return 3;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#duration")) {
    return 4;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#genre")) {
    return 5;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#track")) {
    return 6;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#year")) {
    return 7;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#discNumber")) {
    return 8;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#totalDiscs")) {
    return 9;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#totalTracks")) {
    return 10;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#lastPlayTime")) {
    return 11;
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#playCount")) {
    return 12;
  }

  return -1;
}

NS_IMETHODIMP
sbLocalDatabaseQuery::GetTopLevelPropertyColumn(const nsAString& aProperty,
                                                nsAString& columnName)
{
  nsAutoString retval;

  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#guid")) {
    retval = NS_LITERAL_STRING("guid");
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#created")) {
    retval = NS_LITERAL_STRING("created");
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#updated")) {
    retval = NS_LITERAL_STRING("updated");
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#contentUrl")) {
    retval = NS_LITERAL_STRING("content_url");
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#contentMimeType")) {
    retval = NS_LITERAL_STRING("content_mime_type");
  }
  if (aProperty.EqualsLiteral("http://songbirdnest.com/data/1.0#contentLength")) {
    retval = NS_LITERAL_STRING("content_length");
  }

  if (retval.IsEmpty()) {
    return NS_ERROR_INVALID_ARG;
  }
  else {
    columnName = retval;
    return NS_OK;
  }
}

