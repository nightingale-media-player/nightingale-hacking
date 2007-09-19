/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2007 POTI, Inc.
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

#ifndef __SBSTRINGUTILS_H__
#define __SBSTRINGUTILS_H__

#include <nsStringGlue.h>

/// @see nsString::FindCharInSet
PRInt32 nsString_FindCharInSet(const nsAString& aString,
                               const char *aPattern,
                               PRInt32 aOffset = 0);

void AppendInt(nsAString& str, PRUint64 val);

PRUint64 ToInteger64(const nsAString& str, nsresult* rv);

#endif /* __SBSTRINGUTILS_H__ */

