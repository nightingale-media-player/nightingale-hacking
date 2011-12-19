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

/**
 * \file  LibraryManagerComponent.cpp
 * \brief Songbird LibraryManager Component Factory and Main Entry Point.
 */

#include <mozilla/ModuleUtils.h>
#include "sbLibraryCID.h"
#include "sbLibraryConstraints.h"
#include "sbLibraryManager.h"
#include "sbMediaListDuplicateFilter.h"
#include "sbMediaListEnumeratorWrapper.h"
#include "sbMediaListViewMap.h"
#include "sbMediaItemControllerCleanup.h"
#include "sbMediaItemWatcher.h"
#include "sbTemporaryMediaItem.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLibraryManager, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediaListViewMap, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibrarySort);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibraryConstraintBuilder);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibraryConstraint);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaItemWatcher);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTemporaryMediaItem);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaListDuplicateFilter);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaListEnumeratorWrapper);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaItemControllerCleanup);

NS_DEFINE_NAMED_CID(SONGBIRD_LIBRARY_CONSTRAINT_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_LIBRARY_CONSTRAINTBUILDER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_LIBRARYMANAGER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_LIBRARYSORT_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_MEDIALISTDUPLICATEFILTER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_MEDIALISTENUMERATORWRAPPER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_MEDIALISTVIEWMAP_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_MEDIAITEMWATCHER_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_TEMPORARYMEDIAITEM_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CID);

static const mozilla::Module::CIDEntry kSongbirdLibraryManagerCIDs[] = {
    { &kSONGBIRD_LIBRARY_CONSTRAINT_CID, false, NULL, sbLibraryConstraintConstructor },
    { &kSONGBIRD_LIBRARY_CONSTRAINTBUILDER_CID, false, NULL, sbLibraryConstraintBuilderConstructor },
    { &kSONGBIRD_LIBRARYMANAGER_CID, false, NULL, sbLibraryManagerConstructor },
    { &kSONGBIRD_LIBRARYSORT_CID, false, NULL, sbLibrarySortConstructor },
    { &kSONGBIRD_MEDIALISTDUPLICATEFILTER_CID, false, NULL, sbMediaListDuplicateFilterConstructor },
    { &kSONGBIRD_MEDIALISTENUMERATORWRAPPER_CID, false, NULL, sbMediaListEnumeratorWrapperConstructor },
    { &kSONGBIRD_MEDIALISTVIEWMAP_CID, false, NULL, sbMediaListViewMapConstructor },
    { &kSONGBIRD_MEDIAITEMWATCHER_CID, false, NULL, sbMediaItemWatcherConstructor },
    { &kSONGBIRD_TEMPORARYMEDIAITEM_CID, false, NULL, sbTemporaryMediaItemConstructor },
    { &kSONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CID, false, NULL, sbMediaItemControllerCleanupConstructor },
    { NULL }
};


static const mozilla::Module::ContractIDEntry kSongbirdLibraryManagerContracts[] = {
    { SONGBIRD_LIBRARY_CONSTRAINT_CONTRACTID, &kSONGBIRD_LIBRARY_CONSTRAINT_CID },
    { SONGBIRD_LIBRARY_CONSTRAINTBUILDER_CONTRACTID, &kSONGBIRD_LIBRARY_CONSTRAINTBUILDER_CID },
    { SONGBIRD_LIBRARYMANAGER_CONTRACTID, &kSONGBIRD_LIBRARYMANAGER_CID },
    { SONGBIRD_LIBRARYSORT_CONTRACTID, &kSONGBIRD_LIBRARYSORT_CID },
    { SONGBIRD_MEDIALISTDUPLICATEFILTER_CONTRACTID, &kSONGBIRD_MEDIALISTDUPLICATEFILTER_CID },
    { SONGBIRD_MEDIALISTENUMERATORWRAPPER_CONTRACTID, &kSONGBIRD_MEDIALISTENUMERATORWRAPPER_CID },
    { SONGBIRD_MEDIALISTVIEWMAP_CONTRACTID, &kSONGBIRD_MEDIALISTVIEWMAP_CID },
    { SONGBIRD_MEDIAITEMWATCHER_CONTRACTID, &kSONGBIRD_MEDIAITEMWATCHER_CID },
    { SONGBIRD_TEMPORARYMEDIAITEM_CONTRACTID, &kSONGBIRD_TEMPORARYMEDIAITEM_CID },
    { SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CONTRACTID, &kSONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CID },
    { NULL }
};

static const mozilla::Module::CategoryEntry kSongbirdLibraryManagerCategories[] = {
    { SONGBIRD_LIBRARY_CONSTRAINT_CLASSNAME, SONGBIRD_LIBRARY_CONSTRAINT_CONTRACTID },
    { SONGBIRD_LIBRARYMANAGER_CLASSNAME, SONGBIRD_LIBRARYMANAGER_CONTRACTID },
    { SONGBIRD_LIBRARYSORT_CLASSNAME, SONGBIRD_LIBRARYSORT_CONTRACTID },
    { SONGBIRD_MEDIALISTDUPLICATEFILTER_CLASSNAME, SONGBIRD_MEDIALISTDUPLICATEFILTER_CONTRACTID },
    { SONGBIRD_MEDIALISTENUMERATORWRAPPER_CLASSNAME, SONGBIRD_MEDIALISTENUMERATORWRAPPER_CONTRACTID },
    { SONGBIRD_MEDIALISTVIEWMAP_CLASSNAME, SONGBIRD_MEDIALISTVIEWMAP_CONTRACTID },
    { SONGBIRD_MEDIAITEMWATCHER_CLASSNAME, SONGBIRD_MEDIAITEMWATCHER_CONTRACTID },
    { SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CLASSNAME, "service," SONGBIRD_MEDIAITEMCONTROLLERCLEANUP_CONTRACTID },
    { NULL }
};

static const mozilla::Module kSongbirdLibraryManager = {
    mozilla::Module::kVersion,
    kSongbirdLibraryManagerCIDs,
    kSongbirdLibraryManagerContracts,
    kSongbirdLibraryManagerCategories
};

NSMODULE_DEFN(SongbirdLibraryManager) = &kSongbirdLibraryManager;
