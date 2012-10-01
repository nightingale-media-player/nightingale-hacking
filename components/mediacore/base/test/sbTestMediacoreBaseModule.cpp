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
* \file  sbTestMediacoreBaseModule.cpp
* \brief Songbird Mediacore Base Test Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbTestDummyMediacoreManager.h"
#include "sbTestMediacoreEventCreator.h"
#include "sbTestMediacoreStressThreads.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbTestDummyMediacoreManager);

NS_GENERIC_FACTORY_CONSTRUCTOR(sbTestMediacoreEventCreator);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTestMediacoreStressThreads);

static nsModuleComponentInfo sbTestMediacoreBaseComponents[] =
{
  {
    SB_TEST_DUMMY_MEDIACORE_MANAGER_CLASSNAME,
    SB_TEST_DUMMY_MEDIACORE_MANAGER_CID,
    SB_TEST_DUMMY_MEDIACORE_MANAGER_CONTRACTID,
    sbTestDummyMediacoreManagerConstructor
  },
  {
    SB_TEST_MEDIACORE_EVENT_CREATOR_CLASSNAME,
    SB_TEST_MEDIACORE_EVENT_CREATOR_CID,
    SB_TEST_MEDIACORE_EVENT_CREATOR_CONTRACTID,
    sbTestMediacoreEventCreatorConstructor
  },
  {
    SB_TEST_MEDIACORE_STRESS_THREADS_CLASSNAME,
    SB_TEST_MEDIACORE_STRESS_THREADS_CID,
    SB_TEST_MEDIACORE_STRESS_THREADS_CONTRACTID,
    sbTestMediacoreStressThreadsConstructor
  }
};

NS_IMPL_NSGETMODULE(SongbirdTestBaseMediacore, sbTestMediacoreBaseComponents)
