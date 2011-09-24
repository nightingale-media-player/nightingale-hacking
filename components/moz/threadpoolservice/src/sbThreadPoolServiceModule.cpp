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
 * \file  sbThreadPoolService.cpp
 * \brief Songbird ThreadPool Service Module Component Factory and Main Entry Point.
 */

#include "sbThreadPoolService.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOM.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbThreadPoolService, Init)
NS_DEFINE_NAMED_CID(SB_THREADPOOLSERVICE_CID);

static const mozilla::Module::CIDEntry kSongbirdMozThreadpoolCIDs[] = {
    { &kSB_THREADPOOLSERVICE_CID, true, NULL, sbThreadPoolServiceConstructor },
    { NULL }
};


static const mozilla::Module::ContractIDEntry kSongbirdMozThreadpoolContracts[] = {
    { SB_THREADPOOLSERVICE_CONTRACTID, &kSB_THREADPOOLSERVICE_CID },
    { NULL }
};


static const mozilla::Module::CategoryEntry kSongbirdMozThreadpoolCategories[] = {
    { NS_XPCOM_STARTUP_CATEGORY, SB_THREADPOOLSERVICE_CLASSNAME, SB_THREADPOOLSERVICE_CONTRACTID },
    { NULL }
};


static const mozilla::Module kSongbirdMozThreadpoolModule = {
    mozilla::Module::kVersion,
    kSongbirdMozThreadpoolCIDs,
    kSongbirdMozThreadpoolContracts,
    kSongbirdMozThreadpoolCategories
};

NSMODULE_DEFN(sbMozThreadpoolModule) = &kSongbirdMozThreadpoolModule;
