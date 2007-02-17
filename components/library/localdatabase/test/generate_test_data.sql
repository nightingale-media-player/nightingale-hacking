.mode tabs
---
.output data_sort_trackname_asc.txt
select guid, obj_sortable from resource_properties where property_id = 1 order by obj_sortable asc;
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 1) order by guid;
---
.output data_sort_trackname_desc.txt
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 1) order by guid;
select guid, obj_sortable from resource_properties where property_id = 1 order by obj_sortable desc;
---
.output data_sort_playcount_asc.txt
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 12) order by guid;
select guid, obj_sortable from resource_properties where property_id = 12 order by obj_sortable asc;
---
.output data_sort_playcount_desc.txt
select guid, obj_sortable from resource_properties where property_id = 12 order by obj_sortable desc;
select guid, '(null)' from media_items where guid not in (select  guid from resource_properties where property_id = 12) order by guid;
---
.output data_sort_contenturl_asc.txt
select guid, content_url from media_items order by content_url asc;
---
.output data_sort_contenturl_desc.txt
select guid, content_url from media_items order by content_url desc;
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
rp0.obj_sortable asc, rp1.obj_sortable asc;
select guid, '(null)' from media_items where guid not in (select guid from resource_properties where property_id = 2) order by guid;
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
rp0.obj_sortable asc, rp1.obj_sortable desc;
select guid, '(null)' from media_items where guid not in (select guid from resource_properties where property_id = 2) order by guid;
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
rp0.obj_sortable asc, rp1.obj_sortable asc, rp2.obj_sortable asc;
select guid, '(null)' from media_items where guid not in (select guid from resource_properties where property_id = 2) order by guid;
---
.output stdout

