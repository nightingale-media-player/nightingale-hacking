/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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
 * Functions to help with Album Art functionality
 */
#ifndef __SB_ALBUMARTCOMMON_H__
#define __SB_ALBUMARTCOMMON_H__

#include <nsIURI.h>
#include <sbIMediaItem.h>
#include <nsIArray.h>
#include <nsTArray.h>

nsresult SetItemArtwork(nsIURI* aImageLocation, sbIMediaItem* aMediaItem);
nsresult SetItemsArtwork(nsIURI* aImageLocation, nsIArray* aMediaItems);
nsresult WriteImageMetadata(nsIArray* aMediaItems);

#endif // __SB_ALBUMARTCOMMON_H__
