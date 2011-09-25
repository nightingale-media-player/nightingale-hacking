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

#ifndef SBITUNESIMPORTERCOMMON_H_
#define SBITUNESIMPORTERCOMMON_H_

#include <nsCOMPtr.h>

#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbIDataRemote.h>

// Common Mozilla contract ID's
#define NS_LOCALFILE_CONTRACTID "@mozilla.org/file/local;1"

// Common Nightingale contract ID's
#define SB_DATAREMOTE_CONTRACTID "@getnightingale.com/Nightingale/DataRemote;1"

class sbIDatabaseQuery;
class sbILibrary;
class sbIDataRemote;

// Common typedefs
typedef nsCOMPtr<sbIDatabaseQuery> sbIDatabaseQueryPtr;
typedef nsCOMPtr<sbILibrary> sbILibraryPtr;
typedef nsCOMPtr<sbIDataRemote> sbIDataRemotePtr;

#endif /* SBITUNESIMPORTERCOMMON_H_ */
