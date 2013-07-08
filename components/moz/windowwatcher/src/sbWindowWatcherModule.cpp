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
 * \file  sbWindowWatcherModule.cpp
 * \brief Songbird Window Watcher Module Component Factory and Main Entry Point.
 */

//------------------------------------------------------------------------------
//
// Songbird window watcher module imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbWindowWatcher.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird window watcher module services.
//
//------------------------------------------------------------------------------

// Construct the sbWindowWatcher object and call its Init method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbWindowWatcher, Init)
NS_DEFINE_NAMED_CID(SB_WINDOWWATCHER_CID);

static const mozilla::Module::CIDEntry kWindowWatcherCIDs[] = {
  { &kSB_WINDOWWATCHER_CID, false, NULL, sbWindowWatcherConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kWindowWatcherContracts[] = {
  { SB_WINDOWWATCHER_CONTRACTID, &kSB_WINDOWWATCHER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kWindowWatcherCategories[] = {
  { "xpcom-startup", SB_WINDOWWATCHER_CLASSNAME, SB_WINDOWWATCHER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kWindowWatcherModule = {
  mozilla::Module::kVersion,
  kWindowWatcherCIDs,
  kWindowWatcherContracts,
  kWindowWatcherCategories
};
 
NSMODULE_DEFN(sbWindowWatcher) = &kWindowWatcherModule;
