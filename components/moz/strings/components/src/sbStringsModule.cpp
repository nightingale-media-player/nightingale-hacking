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
// Songbird strings components module.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringsModule.cpp
 * \brief Songbird Strings Component Factory and Main Entry Point.
 */

//------------------------------------------------------------------------------
//
// Songbird strings components module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbStringBundleService.h"
#include "sbStringMap.h"
#include "sbCharsetDetector.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird string bundle service.
//
//------------------------------------------------------------------------------

// Construct the sbStringBundleService object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbStringBundleService, Initialize)

NS_DEFINE_NAMED_CID(SB_STRINGBUNDLESERVICE_CID);

#define SB_STRINGMAP_CLASSNAME "sbStringMap"
#define SB_STRINGMAP_CID \
{ 0x56a00dd5, 0xcfae, 0x4910, \
  { 0xae, 0x12, 0xef, 0x53, 0x93, 0x5d, 0xcf, 0x3e } }

NS_DEFINE_NAMED_CID(SB_STRINGMAP_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbStringMap);

NS_DEFINE_NAMED_CID(SB_CHARSETDETECTOR_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbCharsetDetector);

static const mozilla::Module::CIDEntry kStringBundleCIDs[] = {
  { &kSB_STRINGBUNDLESERVICE_CID, false, NULL, sbStringBundleServiceConstructor },
  { &kSB_STRINGMAP_CID, false, NULL, sbStringMapConstructor },
  { &kSB_CHARSETDETECTOR_CID, false, NULL, sbCharsetDetectorConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kStringBundleContracts[] = {
  { SB_STRINGBUNDLESERVICE_CONTRACTID, &kSB_STRINGBUNDLESERVICE_CID },
  { SB_STRINGMAP_CONTRACTID, &kSB_STRINGMAP_CID },
  { SB_CHARSETDETECTOR_CONTRACTID, &kSB_CHARSETDETECTOR_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kStringBundleCategories[] = {
  { "xpcom-startup", "service,@songbirdnest.com/Songbird/stringbundle;1", SB_STRINGBUNDLESERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kStringsModule = {
  mozilla::Module::kVersion,
  kStringBundleCIDs,
  kStringBundleContracts,
  kStringBundleCategories
};

NSMODULE_DEFN(sbStringsModule) = &kStringsModule;
