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

/**
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  // Note on property_id values used below:
  // 1 = trackName
  // 2 = albumName
  // 3 = artistName
  // 6 = trackNumber
  // 11 = lastPlayTime

  var sqlArtistAscAlbumAsc = <>
select
  rp0.media_item_id,
  rp0.obj_sortable || ' ' || rp1.obj_sortable
from
  resource_properties rp0
  join resource_properties rp1 on rp0.media_item_id = rp1.media_item_id
where
  rp0.property_id = 3 and
  rp1.property_id = 2
order by
  rp0.obj_sortable asc,
  rp1.obj_sortable asc,
  rp0.media_item_id asc;
  </>.toString();

  var sqlAlbumAscTrackAsc = <>
select
  rp0.media_item_id,
  rp0.obj_sortable || ' ' || rp1.obj_sortable
from
  resource_properties rp0
  join resource_properties rp1 on rp0.media_item_id = rp1.media_item_id
where
  rp0.property_id = 2 and
  rp1.property_id = 6
order by
  rp0.obj_sortable asc,
  rp1.obj_sortable asc,
  rp0.media_item_id asc;
  </>.toString();

  var sqlAlbumAscTrackAscNull = <>
select
  media_item_id,
  '(null)'
from
  media_items
where
  media_item_id not in (
    select
      media_item_id
    from
      resource_properties
    where
      property_id = 2
  )
order by
  media_item_id asc;
  </>.toString();

  var sqlAlbumAscTrackDesc = <>
select
  rp0.media_item_id,
  rp0.obj_sortable || ' ' || rp1.obj_sortable
from
  resource_properties rp0
  join resource_properties rp1 on rp0.media_item_id = rp1.media_item_id
where
  rp0.property_id = 2 and
  rp1.property_id = 6
order by
  rp0.obj_sortable asc,
  rp1.obj_sortable desc,
  rp0.media_item_id asc;
  </>.toString();

  var sqlArtistAscAlbumAscTrackAsc = <>
select
  rp0.media_item_id,
  rp0.obj_sortable || ' ' || rp1.obj_sortable || ' ' || rp2.obj_sortable
from
  resource_properties rp0
  join resource_properties rp1 on rp0.media_item_id = rp1.media_item_id
  join resource_properties rp2 on rp0.media_item_id = rp2.media_item_id
where
  rp0.property_id = 3 and
  rp1.property_id = 2 and
  rp2.property_id = 6
order by
  rp0.obj_sortable asc,
  rp1.obj_sortable asc,
  rp2.obj_sortable asc,
  rp0.media_item_id asc;
  </>.toString();

  var sqlArtistAscAlbumAscTrackAscNull = <>
select
  media_item_id,
  '(null)'
from
  media_items
where
  media_item_id not in (
    select
      media_item_id
    from
      resource_properties
    where
      property_id = 3
  )
order by
  media_item_id asc;
  </>.toString();

  var sqlTrackNameAscLastPlayTimeDesc = <>
select
  rp0.media_item_id,
  rp0.obj_sortable || ' ' || rp1.obj_sortable
from
  resource_properties rp0
  left join resource_properties rp1 on rp0.media_item_id = rp1.media_item_id and rp1.property_id = 11
where
  rp0.property_id = 1
order by
  rp0.obj_sortable asc,
  rp1.obj_sortable desc,
  rp0.media_item_id asc;
  </>.toString();

  var sqlTrackNameAscLastPlayTimeDescNull = <>
select
  media_item_id,
  '(null)'
from
  media_items
where
  media_item_id not in (
    select
      media_item_id
    from
      resource_properties
    where
      property_id = 1
  )
order by
  media_item_id asc;
  </>.toString();

  var databaseGUID = "test_guidarray_sortmulti";
  var library = createLibrary(databaseGUID);
  var array;

  var d = execQuery(databaseGUID, sqlArtistAscAlbumAsc);
  var d_null = execQuery(databaseGUID, sqlAlbumAscTrackAscNull);
  var dataSortArtistAscAlbumAsc = d.concat(d_null);

  d = execQuery(databaseGUID, sqlAlbumAscTrackAsc);
  // sqlAlbumAscTrackAscNull is the same for both test, so just resuse it
  d_null = execQuery(databaseGUID, sqlAlbumAscTrackAscNull);
  dataSortAlbumAscTrackAsc = d.concat(d_null);

  d = execQuery(databaseGUID, sqlAlbumAscTrackDesc);
  // sqlAlbumAscTrackAscNull is the same for both test, so just resuse it
  d_null = execQuery(databaseGUID, sqlAlbumAscTrackAscNull);
  var dataSortAlbumAscTrackDesc = d.concat(d_null);

  d = execQuery(databaseGUID, sqlArtistAscAlbumAscTrackAsc);
  d_null = execQuery(databaseGUID, sqlArtistAscAlbumAscTrackAscNull);
  var dataSortArtistAscAlbumAscTrackAsc = d.concat(d_null);

  d = execQuery(databaseGUID, sqlTrackNameAscLastPlayTimeDesc);
  d_null = execQuery(databaseGUID, sqlTrackNameAscLastPlayTimeDescNull);
  var dataSortTrackNameAscLastPlayTimeDesc = d.concat(d_null);

  // Try a variety of fetch sizes to expose edge cases
  var fetchSizes = [0, 1, 2, 5, 7, 10, 50, 100, 200];

  for(var i = 0; i < fetchSizes.length; i++) {
    array = makeArray(library);
    array.baseTable = "media_items";
    array.addSort(SBProperties.artistName, true);
    array.addSort(SBProperties.albumName, true);
    array.fetchSize = fetchSizes[i];

    assertSortArray(array, dataSortArtistAscAlbumAsc);

    array = makeArray(library);
    array.baseTable = "media_items";
    array.addSort(SBProperties.albumName, true);
    array.addSort(SBProperties.trackNumber, true);
    array.fetchSize = fetchSizes[i];

    assertSortArray(array, dataSortAlbumAscTrackAsc);

    array = makeArray(library);
    array.baseTable = "media_items";
    array.addSort(SBProperties.albumName, true);
    array.addSort(SBProperties.trackNumber, false);
    array.fetchSize = fetchSizes[i];

    assertSortArray(array, dataSortAlbumAscTrackDesc);

    array = makeArray(library);
    array.baseTable = "media_items";
    array.addSort(SBProperties.artistName, true);
    array.addSort(SBProperties.albumName, true);
    array.addSort(SBProperties.trackNumber, true);
    array.fetchSize = fetchSizes[i];

    assertSortArray(array, dataSortArtistAscAlbumAscTrackAsc);

    array = makeArray(library);
    array.baseTable = "media_items";
    array.addSort(SBProperties.trackName, true);
    array.addSort(SBProperties.lastPlayTime, false);
    array.fetchSize = fetchSizes[i];

    assertSortArray(array, dataSortTrackNameAscLastPlayTimeDesc);
  }

}

function assertSortArray(array, data) {

  assertEqual(array.length, data.length);

  for (var i = 0; i < array.length; i++) {
    assertEqual(array.getMediaItemIdByIndex(i), data[i][0]);
  }
}
