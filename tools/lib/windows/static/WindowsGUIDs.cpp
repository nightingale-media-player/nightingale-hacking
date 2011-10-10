/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//
// Windows GUID instantiations.
//
//   Some Windows GUIDs (e.g., CLSID_VdsLoader) are not provided in any
// libraries from Microsoft and must be instantiated by client applications.
// This file instantiates these GUIDs for Nightingale.
//
//------------------------------------------------------------------------------

/**
 * \file  WindowsGUIDs.cpp
 * \brief Windows GUID Instantiation Source.
 */

// Import defs for instatiating GUIDs.
#include <initguid.h>

// Instantiate GUIDs.
#include <vds.h>

