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
* \file  sbiTUnesImporterComponent.cpp
* \brief Songbird iTunes Importer component
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbiTunesXMLParser.h"
#include "sbiTunesImporter.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbiTunesXMLParser);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbiTunesImporter);

NS_DEFINE_NAMED_CID(SBITUNESXMLPARSER_CID);
NS_DEFINE_NAMED_CID(SBITUNESIMPORTER_CID);

#define SB_LIBRARY_IMPORTER_CATEGORY "library-importer"

static const mozilla::Module::CIDEntry kiTunesImporterCIDs[] = {
  { &kSBITUNESXMLPARSER_CID, false, NULL, sbiTunesXMLParserConstructor },
  { &kSBITUNESIMPORTER_CID, false, NULL, sbiTunesImporterConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kiTunesImporterContracts[] = {
  { SBITUNESXMLPARSER_CONTRACTID, &kSBITUNESXMLPARSER_CID },
  { SBITUNESIMPORTER_CONTRACTID, &kSBITUNESIMPORTER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kiTunesImporterCategories[] = {
  { NULL }
};

static const mozilla::Module kiTunesImporterModule = {
  mozilla::Module::kVersion,
  kiTunesImporterCIDs,
  kiTunesImporterContracts,
  kiTunesImporterCategories
};

NSMODULE_DEFN(sbiTunesImporter) = &kiTunesImporterModule;
