/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird media item download service components module services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMediaItemDownloadModule.cpp
 * \brief Songbird Media Item Download Service Component Factory and Main Entry
 *        Point.
 */

//------------------------------------------------------------------------------
//
// Songbird media item download service components module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbHTTPMediaItemDownloader.h"
#include "sbMediaItemDownloadService.h"

// Mozilla imports.
#include <nsIGenericFactory.h>


//------------------------------------------------------------------------------
//
// Songbird media item download service components module registration services.
//
//------------------------------------------------------------------------------

// Component factory constructors.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbHTTPMediaItemDownloader)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaItemDownloadService)

// Component registration declarations.
SB_MEDIA_ITEM_DOWNLOADER_REGISTERSELF(sbHTTPMediaItemDownloader)

// Module component information.
static nsModuleComponentInfo sbMediaItemDownloadComponents[] =
{
  // HTTP media item downloader component info.
  {
    SB_HTTP_MEDIA_ITEM_DOWNLOADER_CLASSNAME,
    SB_HTTP_MEDIA_ITEM_DOWNLOADER_CID,
    SB_HTTP_MEDIA_ITEM_DOWNLOADER_CONTRACTID,
    sbHTTPMediaItemDownloaderConstructor,
    sbHTTPMediaItemDownloaderRegisterSelf,
    sbHTTPMediaItemDownloaderUnregisterSelf
  },

  // Songbird media item download service component info.
  {
    SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CLASSNAME,
    SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CID,
    SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CONTRACTID,
    sbMediaItemDownloadServiceConstructor,
    sbMediaItemDownloadService::RegisterSelf,
    sbMediaItemDownloadService::UnregisterSelf
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbMediaItemDownloadModule, sbMediaItemDownloadComponents)

