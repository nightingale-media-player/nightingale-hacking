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
 * \file  sbImageParserModule.cpp
 * \brief Songbird Image parser Module Component Factory and Main Entry Point.
 */

// Self imports.
#include "sbImageParser.h"

// Mozilla imports.
#include <nsIGenericFactory.h>

// Construct the sbImageParser object
NS_GENERIC_FACTORY_CONSTRUCTOR(sbImageParser)

// Module component information.
static const nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_IMAGEPARSER_CLASSNAME,
    SONGBIRD_IMAGEPARSER_CID,
    SONGBIRD_IMAGEPARSER_CONTRACTID,
    sbImageParserConstructor
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbImageParser, components)

