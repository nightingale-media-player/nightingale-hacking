/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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
// Songbird file utilities module services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbFileUtilsModule.cpp
 * \brief Songbird File Utilities Component Factory and Main Entry Point.
 */
 
// Local imports.
#include "sbDirectoryEnumerator.h"
#include "sbFileUtils.h"

// Mozilla imports.
#include <mozilla/ModuleUtils.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDirectoryEnumerator, Initialize)

NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileUtils)
NS_DEFINE_NAMED_CID(SB_FILEUTILS_CID);

NS_DEFINE_NAMED_CID(SB_DIRECTORYENUMERATOR_CID);

static const mozilla::Module::CIDEntry kSongbirdMozFileUtilsCIDs[] = {
    { &kSB_DIRECTORYENUMERATOR_CID, false, NULL, sbDirectoryEnumeratorConstructor },
    { &kSB_FILEUTILS_CID, false, NULL, sbFileUtilsConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdMozFileUtilsContracts[] = {
    { SB_DIRECTORYENUMERATOR_CONTRACTID, &kSB_DIRECTORYENUMERATOR_CID },
    { SB_FILEUTILS_CONTRACTID, &kSB_FILEUTILS_CID },
    { NULL }
};

static const mozilla::Module kSongbirdMozFileUtilsModule = {
    mozilla::Module::kVersion,
    kSongbirdMozFileUtilsCIDs,
    kSongbirdMozFileUtilsContracts
};

NSMODULE_DEFN(sbMozFileUtilsModule) = &kSongbirdMozFileUtilsModule;

