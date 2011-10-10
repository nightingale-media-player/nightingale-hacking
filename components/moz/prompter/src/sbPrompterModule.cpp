/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/** 
 * \file  sbPrompterModule.cpp
 * \brief Nightingale Prompter Module Component Factory and Main Entry Point.
 */

// Self imports.
#include "sbPrompter.h"

// Mozilla imports.
#include <nsIGenericFactory.h>

// Construct the sbPrompter object and call its Init method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbPrompter, Init)

// Module component information.
static const nsModuleComponentInfo components[] =
{
  {
    NIGHTINGALE_PROMPTER_CLASSNAME,
    NIGHTINGALE_PROMPTER_CID,
    NIGHTINGALE_PROMPTER_CONTRACTID,
    sbPrompterConstructor
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbPrompter, components)

