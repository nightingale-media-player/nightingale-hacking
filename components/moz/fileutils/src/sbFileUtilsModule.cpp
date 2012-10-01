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

//------------------------------------------------------------------------------
//
// Songbird file utilities module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbDirectoryEnumerator.h"
#include "sbFileUtils.h"

// Mozilla imports.
#include <nsIGenericFactory.h>


//------------------------------------------------------------------------------
//
// Songbird file utilities module directory enumerator services.
//
//------------------------------------------------------------------------------

// Songbird directory enumerator defs.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDirectoryEnumerator, Initialize)


//------------------------------------------------------------------------------
//
// Songbird file utilities module file utilities services.
//
//------------------------------------------------------------------------------

// Songbird file utilities defs.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbFileUtils)


//------------------------------------------------------------------------------
//
// Songbird file utilities module registration services.
//
//------------------------------------------------------------------------------

// Module component information.
static nsModuleComponentInfo sbFileUtilsComponents[] =
{
  // Songbird directory enumerator component info.
  {
    SB_DIRECTORYENUMERATOR_CLASSNAME,
    SB_DIRECTORYENUMERATOR_CID,
    SB_DIRECTORYENUMERATOR_CONTRACTID,
    sbDirectoryEnumeratorConstructor
  },

  // Songbird file utilities component info.
  {
    SB_FILEUTILS_CLASSNAME,
    SB_FILEUTILS_CID,
    SB_FILEUTILS_CONTRACTID,
    sbFileUtilsConstructor
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbFileUtilsModule, sbFileUtilsComponents)

