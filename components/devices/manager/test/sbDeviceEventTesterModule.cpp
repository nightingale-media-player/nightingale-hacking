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
* \file  sbDeviceManagerModule.cpp
* \brief Songbird Device Manager Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbDeviceEventTesterRemoval.h"
#include "sbDeviceEventTesterStressThreads.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceEventTesterRemoval);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceEventTesterStressThreads);

static nsModuleComponentInfo sbDeviceEventTesterComponents[] =
{
  {
    "Songbird Device Event Tester - Removal",
    { 0xd37fe51b, 0xf17e, 0x464e,
      { 0x9d, 0xcf, 0x37, 0x1f, 0x82, 0x8f, 0xde, 0x66 } },
    "@songbirdnest.com/Songbird/Device/EventTester/Removal;1",
    sbDeviceEventTesterRemovalConstructor
  },
  {
    "Songbird Device Event Tester - Thread Stresser",
    { 0xda2ce34c, 0xa53a, 0x414e,
      { 0x9f, 0x23, 0x9f, 0xf3, 0xa7, 0x18, 0x7f, 0xfa } },
    "@songbirdnest.com/Songbird/Device/EventTester/StressThreads;1",
    sbDeviceEventTesterStressThreadsConstructor
  }

};

NS_IMPL_NSGETMODULE(SongbirdDeviceManagerEventTests, sbDeviceEventTesterComponents)
