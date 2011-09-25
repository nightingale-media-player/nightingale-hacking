/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef SBDEVICEAPI_H_
#define SBDEVICEAPI_H_

/**
 * Determine if we need to export or import. Presence of SB_EXPORT_DEVICE_API
 * means we need to export
 */
#ifdef SB_EXPORT_BASE_DEVICE_API
#define SB_API NS_EXPORT
#else
#define SB_API NS_IMPORT
#endif

#endif /*SBDEVICEAPI_H_*/
