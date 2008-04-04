-- Drop this trigger and re-create it later because it references
-- resource_properties.guid
drop trigger tgr_media_items_simple_media_lists_delete;

-- New resource_properties table schema
create table resource_properties_new (
  media_item_id integer not null,
  property_id integer not null,
  obj text not null,
  obj_sortable text,
  primary key (media_item_id, property_id)
);

-- The properties for the library resource are now represented in the
-- resource_properties table using media_item_id = 0 rather than the
-- guid of the library resource
insert into resource_properties_new
select
  0,
  rp.property_id,
  rp.obj,
  rp.obj_sortable
from
  resource_properties rp
where
  rp.guid = (select value from library_metadata where name = 'resource-guid');

-- Copy the old resource_properties table into the new one while doing the
-- guid -> media_item_id convertion
insert into resource_properties_new
select
  mi.media_item_id,
  rp.property_id,
  rp.obj,
  rp.obj_sortable
from
  resource_properties rp
  join media_items mi on rp.guid = mi.guid;

-- Drop the old resource_properties table and rename the new one
drop table resource_properties;
alter table resource_properties_new rename to resource_properties;

-- Re-create all the indexes on the resource_properties table
create index idx_resource_properties_property_id_obj on resource_properties (property_id, obj);
create index idx_resource_properties_obj_sortable on resource_properties (obj_sortable);
create index idx_resource_properties_media_item_id_property_id_obj_sortable on resource_properties (media_item_id, property_id, obj_sortable);
create index idx_resource_properties_property_id_obj_sortable_media_item_id on resource_properties (property_id, obj_sortable, media_item_id);

-- Re-create the cascade delete trigger
create trigger tgr_media_items_simple_media_lists_delete before delete on media_items
begin
  delete from simple_media_lists where member_media_item_id = OLD.media_item_id or media_item_id = OLD.media_item_id; /**/
  delete from resource_properties where media_item_id = OLD.media_item_id; /**/
end;

-- Update the library's version
update
  library_metadata
set
  value = '2'
where
  name = 'version';
