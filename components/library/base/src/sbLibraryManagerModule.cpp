/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file  LibraryManagerComponent.cpp
 * \brief Nightingale LibraryManager Component Factory and Main Entry Point.
 */

#include <nsIGenericFactory.h>
#include "sbLibraryCID.h"
#include "sbLibraryConstraints.h"
#include "sbLibraryManager.h"
#include "sbMediaListDuplicateFilter.h"
#include "sbMediaListEnumeratorWrapper.h"
#include "sbMediaListViewMap.h"
#include "sbMediaItemWatcher.h"
#include "sbTemporaryMediaItem.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLibraryManager, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediaListViewMap, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibrarySort);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibraryConstraintBuilder);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibraryConstraint);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaItemWatcher);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTemporaryMediaItem);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaListDuplicateFilter)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaListEnumeratorWrapper)

static nsModuleComponentInfo components[] =
{
  {
    NIGHTINGALE_LIBRARY_CONSTRAINT_CLASSNAME,
    NIGHTINGALE_LIBRARY_CONSTRAINT_CID,
    NIGHTINGALE_LIBRARY_CONSTRAINT_CONTRACTID,
    sbLibraryConstraintConstructor
  },
  {
    NIGHTINGALE_LIBRARY_CONSTRAINTBUILDER_CLASSNAME,
    NIGHTINGALE_LIBRARY_CONSTRAINTBUILDER_CID,
    NIGHTINGALE_LIBRARY_CONSTRAINTBUILDER_CONTRACTID,
    sbLibraryConstraintBuilderConstructor
  },
  {
    NIGHTINGALE_LIBRARYMANAGER_CLASSNAME,
    NIGHTINGALE_LIBRARYMANAGER_CID,
    NIGHTINGALE_LIBRARYMANAGER_CONTRACTID,
    sbLibraryManagerConstructor,
    sbLibraryManager::RegisterSelf
  },
  {
    NIGHTINGALE_LIBRARYSORT_CLASSNAME,
    NIGHTINGALE_LIBRARYSORT_CID,
    NIGHTINGALE_LIBRARYSORT_CONTRACTID,
    sbLibrarySortConstructor
  },
  {
    NIGHTINGALE_MEDIALISTDUPLICATEFILTER_CLASSNAME,
    NIGHTINGALE_MEDIALISTDUPLICATEFILTER_CID,
    NIGHTINGALE_MEDIALISTDUPLICATEFILTER_CONTRACTID,
    sbMediaListDuplicateFilterConstructor
  },
  {
    NIGHTINGALE_MEDIALISTENUMERATORWRAPPER_CLASSNAME,
    NIGHTINGALE_MEDIALISTENUMERATORWRAPPER_CID,
    NIGHTINGALE_MEDIALISTENUMERATORWRAPPER_CONTRACTID,
    sbMediaListEnumeratorWrapperConstructor
  },
  {
    NIGHTINGALE_MEDIALISTVIEWMAP_CLASSNAME,
    NIGHTINGALE_MEDIALISTVIEWMAP_CID,
    NIGHTINGALE_MEDIALISTVIEWMAP_CONTRACTID,
    sbMediaListViewMapConstructor,
    sbMediaListViewMap::RegisterSelf
  },
  {
    NIGHTINGALE_MEDIAITEMWATCHER_CLASSNAME,
    NIGHTINGALE_MEDIAITEMWATCHER_CID,
    NIGHTINGALE_MEDIAITEMWATCHER_CONTRACTID,
    sbMediaItemWatcherConstructor
  },
  {
    NIGHTINGALE_TEMPORARYMEDIAITEM_CLASSNAME,
    NIGHTINGALE_TEMPORARYMEDIAITEM_CID,
    NIGHTINGALE_TEMPORARYMEDIAITEM_CONTRACTID,
    sbTemporaryMediaItemConstructor
  }
};

NS_IMPL_NSGETMODULE(NightingaleLibraryManager, components)
