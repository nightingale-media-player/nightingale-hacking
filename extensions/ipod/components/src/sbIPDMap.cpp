/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
// 
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
//=END SONGBIRD GPL
*/

//------------------------------------------------------------------------------
//
// iPod device mapping services.
//
//   These services may only be used within both the connect and request locks.
//
//------------------------------------------------------------------------------

/** 
 * \file  sbIPDMap.cpp
 * \brief Songbird iPod Device Mapping Source.
 */

//------------------------------------------------------------------------------
//
// iPod device mapping imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDDevice.h"

// Songbird imports.
#include <sbIDatabaseResult.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <prprf.h>


//------------------------------------------------------------------------------
//
// iPod device mapping services.
//
//------------------------------------------------------------------------------

/**
 * Initialize the iPod device mapping services.
 */

nsresult
sbIPDDevice::MapInitialize()
{
  nsresult rv;

  // Create the ID map database query object.
  nsCOMPtr<sbIDatabaseQuery> idQuery;
  rv = IDMapCreateDBQuery(getter_AddRefs(idQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the ID map table if it doesn't exist.
  rv = ExecuteQuery(idQuery,
                    "CREATE TABLE IF NOT EXISTS ipod_id_map "
                      "(ipod_id BLOB NOT NULL, songbird_id BLOB NOT NULL)",
                    nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Finalize the iPod device mapping services.
 */

void
sbIPDDevice::MapFinalize()
{
}


//------------------------------------------------------------------------------
//
// iPod device ID mapping services.
//
//   These services provide support for mapping iPod IDs (e.g., track and
// playlist IDs) with Songbird library IDs (e.g., media item and media list
// GUIDs).  These services provide a persistent mapping.
//
//------------------------------------------------------------------------------

/**
 * Add a mapping between the Songbird ID specified by aSBID and the iPod ID
 * specified by aIPodID to the ID map.
 *
 * \param aSBID                 Songbird ID to add to map.
 * \param aIPodID               iPod ID to add to map.
 */

nsresult
sbIPDDevice::IDMapAdd(nsAString& aSBID,
                      guint64    aIPodID)
{
  nsresult rv;

  // Produce the database query string.
  char queryStr[256];
  PR_snprintf(queryStr,
              sizeof(queryStr),
              "INSERT OR REPLACE INTO ipod_id_map"
                "(songbird_id, ipod_id) VALUES"
                "(\"%s\", \"%08x:%08x\")",
              NS_ConvertUTF16toUTF8(aSBID).get(),
              (PRUint32) ((aIPodID >> 32) & 0xFFFFFFFF),
              (PRUint32) (aIPodID & 0xFFFFFFFF));

  // Create the ID map database query object.
  nsCOMPtr<sbIDatabaseQuery> idQuery;
  rv = IDMapCreateDBQuery(getter_AddRefs(idQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the query.
  rv = ExecuteQuery(idQuery, queryStr, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Remove all mappings for the iPod ID specified by aIPodID from the ID map.
 *
 * \param aIPodID               iPod ID to remove from map.
 */

nsresult
sbIPDDevice::IDMapRemove(guint64 aIPodID)
{
  nsresult rv;

  // Produce the database query string.
  char queryStr[256];
  PR_snprintf(queryStr,
              sizeof(queryStr),
              "DELETE FROM ipod_id_map WHERE ipod_id = \"%08x:%08x\"",
              (PRUint32) ((aIPodID >> 32) & 0xFFFFFFFF),
              (PRUint32) (aIPodID & 0xFFFFFFFF));

  // Create the ID map database query object.
  nsCOMPtr<sbIDatabaseQuery> idQuery;
  rv = IDMapCreateDBQuery(getter_AddRefs(idQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the query.
  rv = ExecuteQuery(idQuery, queryStr, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aIPodIDList the list of all iPod IDs mapped to the Songbird ID
 * specified by aSBID.
 *
 * \param aSBID                 Songbird ID to look up in map.
 * \param aIPodIDList           List of mapped iPod IDs.
 */

nsresult
sbIPDDevice::IDMapGet(nsAString&         aSBID,
                      nsTArray<guint64>& aIPodIDList)
{
  nsresult rv;

  // Produce the database query string.
  char queryStr[256];
  PR_snprintf(queryStr,
              sizeof(queryStr),
              "SELECT ipod_id FROM ipod_id_map WHERE songbird_id = \"%s\"",
              NS_ConvertUTF16toUTF8(aSBID).get());

  // Create the ID map database query object.
  nsCOMPtr<sbIDatabaseQuery> idQuery;
  rv = IDMapCreateDBQuery(getter_AddRefs(idQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the query.
  nsCOMPtr<sbIDatabaseResult> dbResult;
  rv = ExecuteQuery(idQuery, queryStr, getter_AddRefs(dbResult));
  NS_ENSURE_SUCCESS(rv, rv);

  // Convert results to iPod IDs.
  PRUint32 rowCount;
  rv = dbResult->GetRowCount(&rowCount);
  aIPodIDList.Clear();
  for (PRUint32 i = 0; i < rowCount; i++) {
    // Get the next query result string.
    nsAutoString iPodIDStr;
    rv = dbResult->GetRowCell(i, 0, iPodIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    // Scan the iPod ID from the ID string.
    guint32 iPodIDHi, iPodIDLo;
    int     numScanned;
    numScanned = PR_sscanf(NS_ConvertUTF16toUTF8(iPodIDStr).get(),
                           "%x:%x\n",
                           &iPodIDHi,
                           &iPodIDLo);
    if (numScanned >= 2) {
      guint64 iPodID = (((guint64) iPodIDHi) << 32) | ((guint64) iPodIDLo);
      aIPodIDList.AppendElement(iPodID);
    }
  }

  return NS_OK;
}


/**
 * Return in aIPodID the first iPod ID mapped to the Songbird ID specified by
 * aSBID.
 *
 * \param aSBID                 Mapped Songbird ID.
 * \param aIPodID               iPod ID to look up in map.
 *
 * \return NS_ERROR_NOT_AVAILABLE
 *                              No mapping was found.
 */

nsresult
sbIPDDevice::IDMapGet(nsAString& aSBID,
                      guint64*   aIPodID)
{
  // Validate arguments.
  NS_ASSERTION(aIPodID, "aIPodID is null");

  // Function variables.
  nsresult rv;

  // Get the list of iPod IDs mapped to the Songbird ID.
  nsTArray<guint64> iPodIDList;
  rv = IDMapGet(aSBID, iPodIDList);
  if (NS_FAILED(rv) || (iPodIDList.Length() == 0))
    return NS_ERROR_NOT_AVAILABLE;

  // Return the first iPod ID.
  *aIPodID = iPodIDList[0];

  return NS_OK;
}


/**
 * Return in aSBID the Songbird ID mapped to the iPod ID specified by aIPodID.
 *
 * \param aIPodID               iPod ID to look up in map.
 * \param aSBID                 Mapped Songbird ID.
 *
 * \return NS_ERROR_NOT_AVAILABLE
 *                              No mapping was found.
 */

nsresult
sbIPDDevice::IDMapGet(guint64    aIPodID,
                      nsAString& aSBID)
{
  nsresult rv;

  // Produce the database query string.
  char queryStr[256];
  PR_snprintf(queryStr,
              sizeof(queryStr),
              "SELECT songbird_id FROM ipod_id_map "
                "WHERE ipod_id = \"%08x:%08x\"",
              (PRUint32) ((aIPodID >> 32) & 0xFFFFFFFF),
              (PRUint32) (aIPodID & 0xFFFFFFFF));

  // Create the ID map database query object.
  nsCOMPtr<sbIDatabaseQuery> idQuery;
  rv = IDMapCreateDBQuery(getter_AddRefs(idQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the query.
  nsCOMPtr<sbIDatabaseResult> dbResult;
  rv = ExecuteQuery(idQuery, queryStr, getter_AddRefs(dbResult));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get Songbird ID from the query result.
  PRUint32 rowCount;
  rv = dbResult->GetRowCount(&rowCount);
  if (!(rowCount > 0))
    return NS_ERROR_NOT_AVAILABLE;
  rv = dbResult->GetRowCell(0, 0, aSBID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aLibraryGUID and aResourceGUID the Songbird library and resource
 * GUIDs mapped to the iPod ID specified by aIPodID.
 *
 * \param aIPodID               iPod ID to look up in map.
 * \param aLibraryGUID          Mapped Songbird library GUID.
 * \param aResourceGUID         Mapped Songbird library resource GUID.
 *
 * \return NS_ERROR_NOT_AVAILABLE
 *                              No mapping was found.
 */

nsresult
sbIPDDevice::IDMapGet(guint64    aIPodID,
                      nsAString& aLibraryGUID,
                      nsAString& aResourceGUID)
{
  nsresult rv;

  // Get the Songbird ID mapped to the iPod item ID.
  nsAutoString sbID;
  rv = IDMapGet(aIPodID, sbID);
  if (NS_FAILED(rv))
    return NS_ERROR_NOT_AVAILABLE;

  // Get the library and resource GUIDs.
  rv = DecodeSBID(sbID, aLibraryGUID, aResourceGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aMediaItem the media item mapped to the iPod ID specified by
 * aIPodID.
 *
 * \param aIPodID               iPod ID to look up in map.
 * \param aMediaItem            Mapped media item.
 *
 * \return NS_ERROR_NOT_AVAILABLE
 *                              No mapping was found.
 */

nsresult
sbIPDDevice::IDMapGet(guint64        aIPodID,
                      sbIMediaItem** aMediaItem)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the Songbird library and item GUIDs mapped to the iPod item ID.
  nsAutoString libraryGUID;
  nsAutoString itemGUID;
  rv = IDMapGet(aIPodID, libraryGUID, itemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Search for the mapped media item.
  nsCOMPtr<sbILibrary>   library;
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = mLibraryManager->GetLibrary(libraryGUID, getter_AddRefs(library));
  if (NS_FAILED(rv))
    return NS_ERROR_NOT_AVAILABLE;
  rv = library->GetItemByGuid(itemGUID, getter_AddRefs(mediaItem));
  if (NS_FAILED(rv))
    return NS_ERROR_NOT_AVAILABLE;

  // Return results.
  NS_ADDREF(*aMediaItem = mediaItem);

  return NS_OK;
}


/**
 * Create and return in aQuery a query object for use with the ID map database.
 *
 * \param aQuery                ID map database query object.
 */

nsresult
sbIPDDevice::IDMapCreateDBQuery(sbIDatabaseQuery** aQuery)
{
  // Validate arguments.
  NS_ASSERTION(aQuery, "aQuery is null");

  // Function variables.
  nsresult rv;

  // Create the ID map database query object.
  nsCOMPtr<sbIDatabaseQuery>
    query = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1",
                              &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the database query.
  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = query->SetDatabaseGUID(NS_LITERAL_STRING("iPod"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*aQuery = query);

  return NS_OK;
}


/**
 * Return in aSBID the Songbird ID for the media item specified by aMediaItem.
 *
 * \param aMediaItem            Media item for which to get Songbird ID.
 * \param aSBID                 Songbird ID for media item.
 */

nsresult
sbIPDDevice::GetSBID(sbIMediaItem* aMediaItem,
                     nsAString&    aSBID)
{
  // Validate arguments.
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Function variables.
  nsresult rv;

  // Get the media item library.
  nsCOMPtr<sbILibrary> library;
  rv = aMediaItem->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the media item and library GUIDs.
  nsAutoString libraryGUID;
  nsAutoString itemGUID;
  rv = library->GetGuid(libraryGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aMediaItem->GetGuid(itemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Encode the Songbird ID.
  rv = EncodeSBID(aSBID, libraryGUID, itemGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal iPod device mapping services.
//
//------------------------------------------------------------------------------

/**
 * Return in aIPodID the ID of the iPod item specified by aIPodItem and aType.
 *
 * \param aIPodItem             iPod item for which to get ID.
 * \param aType                 Type of iPod item.
 * \param aIPodID               Returned iPod ID.
 */

nsresult
sbIPDDevice::IPodItemGetID(void*    aIPodItem,
                           int      aType,
                           guint64* aIPodID)
{
  // Validate parameters.
  NS_ASSERTION(aIPodItem, "aIPodItem is null");
  NS_ASSERTION(aIPodID, "aIPodID is null");

  // Get the iPod item ID.
  guint64 iPodID;
  if (aType == TypePlaylist) {
    Itdb_Playlist* playlist = (Itdb_Playlist*) aIPodItem;
    iPodID = playlist->id;
  } else {
    Itdb_Track* track = (Itdb_Track*) aIPodItem;
    iPodID = track->dbid;
  }

  // Return results.
  *aIPodID = iPodID;

  return NS_OK;
}


/**
 * Decode the mapped Songbird ID specified by aSBID into the Songbird library
 * GUID specified by aLibraryGUID and resource GUID specified by aResourceGUID.
 *
 * \param aSBID                 Mapped Songbird ID.
 * \param aLibraryGUID          Mapped Songbird library GUID.
 * \param aResourceGUID         Mapped Songbird library resource GUID.
 */

nsresult
sbIPDDevice::DecodeSBID(const nsAString& aSBID,
                        nsAString&       aLibraryGUID,
                        nsAString&       aResourceGUID)
{
  // Extract the Songbird library and resource GUID's from the Songbird ID.
  PRInt32 pos = aSBID.RFindChar(SBIDDelimiter);
  NS_ENSURE_TRUE(pos != -1, NS_ERROR_INVALID_ARG);
  aLibraryGUID.Assign(Substring(aSBID, 0, pos));
  aResourceGUID.Assign(Substring(aSBID, pos + 1));

  return NS_OK;
}


/**
 * Encode the Songbird library GUID specified by aLibraryGUID and resource GUID
 * specified by aResourceGUID into the mapped Songbird ID specified by aSBID.
 *
 * \param aSBID                 Mapped Songbird ID.
 * \param aLibraryGUID          Mapped Songbird library GUID.
 * \param aResourceGUID         Mapped Songbird library resource GUID.
 */

nsresult
sbIPDDevice::EncodeSBID(nsAString&       aSBID,
                        const nsAString& aLibraryGUID,
                        const nsAString& aResourceGUID)
{
  // Produce the Songbird ID from the Songbird library and resource GUID's.
  aSBID.Assign(aLibraryGUID);
  nsCAutoString nsDelimiter;
  char delimiter[2];
  delimiter[0] = SBIDDelimiter;
  delimiter[1] = '\0';
  aSBID.AppendLiteral(delimiter);
  aSBID.Append(aResourceGUID);

  return NS_OK;
}


/**
 * Execute the databse query specified by aQueryStr using the database query
 * object specified by aDBQuery and return the results in aDBResult.  If
 * aDBResult is null, do not return the results.
 *
 * \param aDBQuery              Database query object.
 * \param aQueryStr             Database query string.
 * \param aDBResult             Database query result.
 */

nsresult
sbIPDDevice::ExecuteQuery(sbIDatabaseQuery*   aDBQuery,
                          const char*         aQueryStr,
                          sbIDatabaseResult** aDBResult)
{
  // Validate arguments.
  NS_ASSERTION(aDBQuery, "aDBQuery is null");
  NS_ASSERTION(aQueryStr, "aQueryStr is null");

  // Function variables.
  PRInt32  dbError;
  nsresult rv;

  // Set up the query.
  rv = aDBQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aDBQuery->AddQuery(NS_ConvertUTF8toUTF16(aQueryStr));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the query.
  rv = aDBQuery->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_FALSE(dbError, NS_ERROR_FAILURE);

  // Read results or reset query if results not needed.
  if (aDBResult) {
    nsCOMPtr<sbIDatabaseResult> dbResult;
    rv = aDBQuery->GetResultObject(getter_AddRefs(dbResult));
    NS_ENSURE_TRUE(dbResult, NS_ERROR_FAILURE);
    NS_ADDREF(*aDBResult = dbResult);
  } else {
    rv = aDBQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


