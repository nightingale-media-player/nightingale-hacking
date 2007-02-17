drop table media_items;
drop table media_list_types;
drop table properties;
drop table resource_properties;
drop table database_metadata;
drop table simple_media_lists;

create table database_metadata (
  name text primary key,
  value text
);

create table media_items (
  media_item_id integer primary key autoincrement,  /* implicit index creation */
  guid text unique not null, /* implicit index creation */
  created integer not null,
  updated integer,
  content_url text not null,
  content_mime_type text,
  content_length integer,
  media_list_type_id integer
);

create table media_list_types (
  media_list_type_id integer primary key autoincrement, /* implicit index creation */
  name text unique not null, /* implicit index creation */
  cid text not null
);

create table properties (
  property_id integer primary key autoincrement, /* implicit index creation */
  property_name text not null unique /* implicit index creation */
);

create table resource_properties (
  guid text not null,
  property_id integer not null,
  obj text not null,
  obj_sortable text
);
create index idx_resource_properties_property_id_obj on resource_properties (property_id, obj);
create index idx_resource_properties_obj_sortable on resource_properties (obj_sortable);
create index idx_resource_properties_property_id_obj_sortable on resource_properties (property_id, obj_sortable);
create index idx_resource_properties_guid_property_id_obj_sortable on resource_properties (guid, property_id, obj_sortable);
create index idx_resource_properties_property_id_obj_sortable_guid on resource_properties (property_id, obj_sortable, guid);

create table simple_media_lists (
  media_item_id integer not null,
  member_media_item_id integer not null,
  ordinal integer not null
);
create index idx_simple_media_lists_media_item_id_member_media_item_id on simple_media_lists (media_item_id, member_media_item_id);

/* static data */

insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#trackName');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#albumName');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#artistName');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#duration');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#genre');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#track');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#year');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#discNumber');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#totalDiscs');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#totalTracks');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#lastPlayTime');
insert into properties (property_name) values ('http://songbirdnest.com/data/1.0#playCount');

