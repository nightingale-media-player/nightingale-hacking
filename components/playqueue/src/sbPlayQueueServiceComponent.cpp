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
 * \file sbPlayQueueServiceComponent.cpp
 * \brief Songbird Play Queue Service component factory.
 */

#include "sbPlayQueueService.h"

#include <nsIGenericFactory.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPlayQueueService, Init)

static const nsModuleComponentInfo sbPlayQueueServiceComponent[] =
{
  {
    SB_PLAYQUEUESERVICE_CLASSNAME,
    SB_PLAYQUEUESERVICE_CID,
    SB_PLAYQUEUESERVICE_CONTRACTID,
    sbPlayQueueServiceConstructor,
    sbPlayQueueService::RegisterSelf
  }
};

NS_IMPL_NSGETMODULE(SongbirdPlayQueueServiceComponent,
                    sbPlayQueueServiceComponent);

