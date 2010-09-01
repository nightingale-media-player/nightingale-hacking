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

EXPORTED_SYMBOLS = [ "kPlaylistCommands" ];

var kPlaylistCommands = {
  // Atomic commands
  MEDIAITEM_PLAY                 : "{b071e1d7-59c4-4390-b627-cec52ed3f2b1}",
  MEDIAITEM_REMOVE               : "{42beadfc-2e95-47f5-beaa-606ebdd1c682}",
  MEDIAITEM_EDIT                 : "{07320d73-3582-4dd5-8751-a0140980c210}",
  MEDIAITEM_DOWNLOAD             : "{a131503b-067b-48f3-bd3e-08262a5fb87e}",
  MEDIAITEM_RESCAN               : "{5b735afd-e55e-4caa-a8c5-4979ba6b03bc}",
  MEDIAITEM_REVEAL               : "{389d85d8-6387-4d9f-bac8-28ce3f4bc470}",
  MEDIAITEM_SUBSCRIBE            : "{5b7dbb49-6cef-4370-8d49-c3e131cdc49a}",
  MEDIAITEM_ADDTOLIBRARY         : "{63fd715b-f700-4bb5-945c-95a496197e83}",
  MEDIAITEM_ADDTOPLAYLIST        : "{5c6242bd-3f51-4b49-8978-6d1a4495ae77}",
  MEDIAITEM_ADDTODEVICE          : "{c6f648d7-e057-46c7-a8b8-d87749225a84}",
  MEDIAITEM_COPYTRACKLOCATION    : "{610a44aa-629e-4d02-90f7-b475f8d9fdeb}",
  MEDIAITEM_SHOWDOWNLOADPLAYLIST : "{8df121ae-6f56-4ccf-a99f-376a8d0bebd2}",
  MEDIAITEM_PAUSERESUMEDOWNLOAD  : "{2042637a-a3eb-4596-a423-3421992f2676}",
  MEDIAITEM_CLEANUPDOWNLOADS     : "{9c68ad34-b35d-4b36-8cc7-5ef14709abe3}",
  MEDIAITEM_CLEARHISTORY         : "{7332def6-0b4b-4c4f-83f6-2f18ebc41259}",
  MEDIAITEM_GETARTWORK           : "{20f183c2-64c4-4e53-974c-eb6b6db1e570}",
  MEDIAITEM_QUEUENEXT            : "{78386542-1dd2-11b2-8db0-ca4eded7ab5c}",
  MEDIAITEM_QUEUELAST            : "{ec6e6281-17c6-45bf-8937-1a9eb12332d7}",

  MEDIALIST_PLAY                 : "{77c0c7d6-6380-4760-b95c-2a5a7c76047c}",
  MEDIALIST_REMOVE               : "{8be21529-6b2e-4ac7-b96d-c97d78dab81b}",
  MEDIALIST_RENAME               : "{b77c5259-62fc-4cd7-af79-ece9adb778b6}",
  MEDIALIST_QUEUENEXT            : "{41a96e57-361c-4675-8ac1-9d808c6a4c74}",
  MEDIALIST_QUEUELAST            : "{db4f216e-6d95-4251-b7a5-f931664425d5}",
  MEDIALIST_UPDATESMARTMEDIALIST : "{a21c0a87-9f0b-4f43-aafe-92c055a13a2d}",
  MEDIALIST_EDITSMARTMEDIALIST   : "{5bc8bf35-57e9-4130-9f3a-d7a32367d70f}",

  PLAYQUEUE_SAVETOPLAYLIST       : "{5fab0587-f3b3-4729-8c95-691991f361cb}",
  PLAYQUEUE_CLEARALL             : "{51157e1b-0b7e-43e0-890c-6549ad65cc17}",
  PLAYQUEUE_CLEARHISTORY         : "{a0ae8ae5-b6f0-4620-85e2-fa0658892edd}",

  // Bundled commands
  MEDIAITEM_DEFAULT              : "{5a5d24dd-0fed-4be0-b200-ac1ed9095d1f}",
  MEDIAITEM_WEBPLAYLIST          : "{8ebde25c-79e9-4bdb-836d-e0d502b1a452}",
  MEDIAITEM_WEBTOOLBAR           : "{9b68b4b3-2362-47c9-92df-0d73aa3fc504}",
  MEDIAITEM_DOWNLOADPLAYLIST     : "{0bc53d42-81d1-437a-9ae6-d8c214a2eb0f}",
  MEDIAITEM_DOWNLOADTOOLBAR      : "{c8f507b2-904f-4769-9112-6eb067fc91c3}",
  MEDIAITEM_SMARTPLAYLIST        : "{9a8b43d3-161a-4eb6-92b4-d96ad53497fc}",

  MEDIALIST_DEFAULT              : "{26c21ce7-bcdf-4857-86d4-82c9747c907f}",
  MEDIALIST_DOWNLOADPLAYLIST     : "{3d6125f1-e5cf-4c08-9c83-697ddf5809cb}",
  MEDIALIST_DEVICE_LIBRARY       : "devicelibrary@playlistcommands.songbirdnest.com",
  MEDIALIST_CDDEVICE_LIBRARY     : "cddevicelibrary@playlistcommands.songbirdnest.com",
  MEDIALIST_PLAYQUEUE_LIBRARY    : "playqueuelibrary@playlistcommands.songbirdnest.com"
}

