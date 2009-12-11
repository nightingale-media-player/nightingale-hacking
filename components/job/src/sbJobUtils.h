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

#ifndef _SB_JOB_UTILS_H_
#define _SB_JOB_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird job utility defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbJobUtils.h
 * \brief Songbird Job Utility Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird job imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbMemoryUtils.h>


//------------------------------------------------------------------------------
//
// Songbird job services.
//
//------------------------------------------------------------------------------

//
// sbAutoJobCancel              Wrapper to auto-cancel a job.
//

SB_AUTO_NULL_CLASS(sbAutoJobCancel, sbIJobCancelable*, mValue->Cancel());


#endif /* _SB_JOB_UTILS_H_ */

