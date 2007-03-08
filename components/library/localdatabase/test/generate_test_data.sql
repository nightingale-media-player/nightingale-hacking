.mode tabs
---
.output data_sort_trackname_asc.txt
select mi.guid, obj_sortable from resource_properties rp join media_items mi on rp.guid = mi.guid where property_id = 1 order by obj_sortable asc, media_item_id asc;
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 1) order by media_item_id asc;
---
.output data_sort_trackname_desc.txt
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 1) order by media_item_id asc;
select mi.guid, obj_sortable from resource_properties rp join media_items mi on rp.guid = mi.guid where property_id = 1 order by obj_sortable desc, media_item_id asc;
---
.output data_sort_playcount_asc.txt
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 12) order by media_item_id asc;
select mi.guid, obj_sortable from resource_properties rp join media_items mi on rp.guid = mi.guid where property_id = 12 order by obj_sortable asc, media_item_id asc;
---
.output data_sort_playcount_desc.txt
select mi.guid, obj_sortable from resource_properties rp join media_items mi on rp.guid = mi.guid where property_id = 12 order by obj_sortable desc, media_item_id asc;
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 12) order by media_item_id asc;
---
.output data_sort_contenturl_asc.txt
select guid, content_url from media_items order by content_url asc, media_item_id asc;
---
.output data_sort_contenturl_desc.txt
select guid, content_url from media_items order by content_url desc, media_item_id asc;
---
.output data_sort_created_asc.txt
select guid, created from media_items order by created asc, media_item_id asc;
---
.output data_sort_album_asc_track_asc.txt
select
rp0.guid, rp0.obj_sortable || ' ' || rp1.obj_sortable
from
resource_properties rp0
join resource_properties rp1 on rp0.guid = rp1.guid
where
rp0.property_id = 2 and
rp1.property_id = 6
order by
rp0.obj_sortable asc, rp1.obj_sortable asc, rp0.guid asc;
select guid, '(null)' from media_items where guid not in (select guid from resource_properties where property_id = 2) order by guid asc;
---
.output data_sort_album_asc_track_desc.txt
select
rp0.guid, rp0.obj_sortable || ' ' || rp1.obj_sortable
from
resource_properties rp0
join resource_properties rp1 on rp0.guid = rp1.guid
where
rp0.property_id = 2 and
rp1.property_id = 6
order by
rp0.obj_sortable asc, rp1.obj_sortable desc, rp0.guid asc;
select guid, '(null)' from media_items where guid not in (select guid from resource_properties where property_id = 2) order by guid asc;
---
.output data_sort_artist_asc_album_asc_track_asc.txt
select
rp0.guid, rp0.obj_sortable || ' ' || rp1.obj_sortable || ' ' || rp2.obj_sortable
from
resource_properties rp0
join resource_properties rp1 on rp0.guid = rp1.guid
join resource_properties rp2 on rp0.guid = rp2.guid
where
rp0.property_id = 3 and
rp1.property_id = 2 and
rp2.property_id = 6
order by
rp0.obj_sortable asc, rp1.obj_sortable asc, rp2.obj_sortable asc, rp0.guid asc;
select guid, '(null)' from media_items where guid not in (select guid from resource_properties where property_id = 2) order by guid asc;
---
.output data_sort_sml101_ordinal_asc.txt
select
guid, ordinal
from
media_items mi,
simple_media_lists sml
where
mi.media_item_id = sml.member_media_item_id and
sml.media_item_id = 101
order by sml.ordinal asc;
---
.output data_sort_sml101_ordinal_desc.txt
select
guid, ordinal
from
media_items mi,
simple_media_lists sml
where
mi.media_item_id = sml.member_media_item_id and
sml.media_item_id = 101
order by sml.ordinal desc;
---
.output stdout

