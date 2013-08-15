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
// Songbird temporary file service module services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryFileServiceModule.cpp
 * \brief Songbird Temporary File Service Component Factory and Main Entry
 *        Point.
 */

//------------------------------------------------------------------------------
//
// Songbird temporary file service module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbTemporaryFileFactory.h"
#include "sbTemporaryFileService.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird temporary file service module temporary file services.
//
//------------------------------------------------------------------------------

// Songbird temporary file service defs.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTemporaryFileService, Initialize);
NS_DEFINE_NAMED_CID(SB_TEMPORARYFILESERVICE_CID);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbTemporaryFileFactory);
NS_DEFINE_NAMED_CID(SB_TEMPORARYFILEFACTORY_CID);

static const mozilla::Module::CIDEntry kTemporaryFileServiceCIDs[] = {
  { &kSB_TEMPORARYFILESERVICE_CID, true, NULL, sbTemporaryFileServiceConstructor },
  { &kSB_TEMPORARYFILEFACTORY_CID, false, NULL, sbTemporaryFileFactoryConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kTemporaryFileServiceContracts[] = {
  { SB_TEMPORARYFILESERVICE_CONTRACTID, &kSB_TEMPORARYFILESERVICE_CID },
  { SB_TEMPORARYFILEFACTORY_CONTRACTID, &kSB_TEMPORARYFILEFACTORY_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kTemporaryFileServiceCategories[] = {
  { "app-startup", SB_TEMPORARYFILESERVICE_CLASSNAME, SB_TEMPORARYFILESERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kTemporaryFileServiceModule = {
  mozilla::Module::kVersion,
  kTemporaryFileServiceCIDs,
  kTemporaryFileServiceContracts,
  kTemporaryFileServiceCategories
};

NSMODULE_DEFN(sbTemporaryFileService) = &kTemporaryFileServiceModule;
