drop table if exists media_items;
drop table if exists media_list_types;
drop table if exists properties;
drop table if exists resource_properties;
drop table if exists library_metadata;
drop table if exists simple_media_lists;
drop table if exists library_media_item;
drop table if exists resource_properties_fts;
drop table if exists resource_properties_fts_all;

create table library_metadata (
  name text primary key,
  value text
);

create table media_items (
  media_item_id integer primary key autoincrement,  /* implicit index creation */
  guid text unique not null, /* implicit index creation */
  created integer not null,
  updated integer not null,
  content_url text not null,
  content_mime_type text,
  content_length integer,
  content_hash text,
  hidden integer not null check(hidden in (0, 1)),
  media_list_type_id integer,
  is_list integer not null check(is_list in (0, 1)) default 0,
  metadata_hash_identity text
);
create index idx_media_items_hidden on media_items (hidden);
create index idx_media_items_created on media_items (created);
create index idx_media_items_content_url on media_items (content_url);
create index idx_media_items_media_list_type_id on media_items (media_list_type_id);
create index idx_media_items_is_list on media_items (is_list);
create index idx_media_items_hidden_media_list_type_id on media_items (hidden, media_list_type_id);
create index idx_media_items_content_mime_type on media_items(content_mime_type);
create index idx_media_items_metadata_hash_identity on media_items(metadata_hash_identity);

create table library_media_item (
  guid text unique not null, /* implicit index creation */
  created integer not null,
  updated integer not null,
  content_url text not null,
  content_mime_type text,
  content_length integer,
  content_hash text,
  hidden integer not null check(hidden in (0, 1)),
  media_list_type_id integer,
  is_list integer not null check(is_list in (0, 1)) default 0,
  metadata_hash_identity text
);

create table media_list_types (
  media_list_type_id integer primary key autoincrement, /* implicit index creation */
  type text unique not null, /* implicit index creation */
  factory_contractid text not null
);

create table properties (
  property_id integer primary key autoincrement, /* implicit index creation */
  property_name text not null unique /* implicit index creation */
);

create table resource_properties (
  media_item_id integer not null,
  property_id integer not null,
  obj text not null,
  obj_searchable text,
  obj_sortable text collate library_collate,
  obj_secondary_sortable text collate library_collate,
  primary key (media_item_id, property_id)
);
create index idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id on resource_properties (property_id, obj_sortable, obj_secondary_sortable, media_item_id);
create index idx_resource_properties_property_id_obj_sortable_media_item_id on resource_properties (property_id, obj_sortable, media_item_id);

create index idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id_asc on resource_properties (property_id, obj_sortable ASC, obj_secondary_sortable ASC, media_item_id ASC);
create index idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id_desc on resource_properties (property_id, obj_sortable DESC, obj_secondary_sortable DESC, media_item_id DESC);

create table simple_media_lists (
  media_item_id integer not null,
  member_media_item_id integer not null,
  ordinal text not null collate tree
);
create index idx_simple_media_lists_media_item_id_member_media_item_id on simple_media_lists (media_item_id, member_media_item_id, ordinal);
create unique index idx_simple_media_lists_media_item_id_ordinal on simple_media_lists (media_item_id, ordinal);
create index idx_simple_media_lists_member_media_item_id on simple_media_lists (member_media_item_id);

/* resource_properties_fts is disabled. See bug 9488 and bug 9617. */
/* create virtual table resource_properties_fts using FTS3 (propertyid, obj); */

create virtual table resource_properties_fts_all using FTS3 (
  alldata
);

/* note the empty comment blocks at the end of the lines in the body of the */
/* trigger need to be there to prevent the parser from splitting on the */
/* line ending semicolon */
   
/* We can reinsert this when we start using the FTS3 resource_properties_fts table again. */
/* delete from resource_properties_fts where rowid in (select rowid from resource_properties where media_item_id = OLD.media_item_id) */
create trigger tgr_media_items_simple_media_lists_delete before delete on media_items
begin
  delete from resource_properties_fts_all where rowid = OLD.media_item_id; /**/
  delete from simple_media_lists where member_media_item_id = OLD.media_item_id or media_item_id = OLD.media_item_id; /**/
  delete from resource_properties where media_item_id = OLD.media_item_id; /**/
end;

/* static data */

insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#trackName');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#albumName');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#artistName');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#duration');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#genre');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#trackNumber');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#year');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#discNumber');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#totalDiscs');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#totalTracks');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#lastPlayTime');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#playCount');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#customType');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#isSortable');

insert into media_list_types (type, factory_contractid) values ('simple', '@songbirdnest.com/Songbird/Library/LocalDatabase/SimpleMediaListFactory;1');

/**************************************************************************** */
/*  XXXAus: !!!WARNING!!! When changing this value, you _MUST_ update         */
/*  sbLocalDatabaseMigrationHelper._latestSchemaVersion.                      */
/**************************************************************************** */
insert into library_metadata (name, value) values ('version', '29');

/**************************************************************************** */
/*  XXXkreeger: !! WARNING !! When changing this schema, the |ANALYZE| data   */
/*  must be updated and replace the contents below.                           */
/*                                                                            */
/*  To obtain new analyze data:                                               */
/*  * Launch a release build of the app with a clean profile                  */
/*  * Import 10,000+ tracks                                                   */
/*  * Spend some time scrolling, searching, filtering, and sorting            */
/*  * Play some tracks, and exercise the smart playlists                      */
/*  * Shutdown                                                                */
/*  * Run ./songbird -test localdatabaselibraryperf                           */
/*    (requires exporting environment variables                               */
/*     SB_ENABLE_TESTS="1" SB_ENABLE_LIBRARY_PERF="1"                         */
/*     and SB_PERF_RESULTS="/builds/songbird/trunk/results.txt")              */
/*  * Run Songbird, and install the Developer Tools Extension                 */
/*  * Open XPCOM Viewer from the Tools menu                                   */
/*  * Execute the following in the JS Shell:                                  */
/*     Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");*/
/*     LibraryUtils.mainLibrary.optimize();                                   */
/*  * Shutdown                                                                */
/*  * Open the main library db using sqlite and run ".dump sqlite_stat1"      */
/*                                                                            */
/**************************************************************************** */

/* Insert static |ANALYZE| results */
ANALYZE;
INSERT INTO "sqlite_stat1" VALUES('simple_media_lists','idx_simple_media_lists_media_item_id_ordinal','20614 3436 1');
INSERT INTO "sqlite_stat1" VALUES('simple_media_lists','idx_simple_media_lists_member_media_item_id','20614 3');
INSERT INTO "sqlite_stat1" VALUES('simple_media_lists','idx_simple_media_lists_media_item_id_member_media_item_id','20614 3436 1 1');
INSERT INTO "sqlite_stat1" VALUES('resource_properties','idx_resource_properties_property_id_obj_sortable_obj_secondary_sortable_media_item_id','56414 1946 77 29 1');
INSERT INTO "sqlite_stat1" VALUES('resource_properties','sqlite_autoindex_resource_properties_1','56414 6 1');
INSERT INTO "sqlite_stat1" VALUES('properties','sqlite_autoindex_properties_1','82 1');
INSERT INTO "sqlite_stat1" VALUES('library_media_item','sqlite_autoindex_library_media_item_1','1 1');
INSERT INTO "sqlite_stat1" VALUES('resource_properties_fts_all_segdir','sqlite_autoindex_resource_properties_fts_all_segdir_1','25 7 1');
INSERT INTO "sqlite_stat1" VALUES('media_list_types','sqlite_autoindex_media_list_types_1','3 1');
INSERT INTO "sqlite_stat1" VALUES('media_items','idx_media_items_hidden_media_list_type_id','10040 5020 2');
INSERT INTO "sqlite_stat1" VALUES('media_items','idx_media_items_is_list','10040 5020');
INSERT INTO "sqlite_stat1" VALUES('media_items','idx_media_items_media_list_type_id','10040 2');
INSERT INTO "sqlite_stat1" VALUES('media_items','idx_media_items_content_url','10040 1');
INSERT INTO "sqlite_stat1" VALUES('media_items','idx_media_items_created','10040 9');
INSERT INTO "sqlite_stat1" VALUES('media_items','idx_media_items_hidden','10040 5020');
INSERT INTO "sqlite_stat1" VALUES('media_items','sqlite_autoindex_media_items_1','10040 1');
INSERT INTO "sqlite_stat1" VALUES('library_metadata','sqlite_autoindex_library_metadata_1','3 1');
