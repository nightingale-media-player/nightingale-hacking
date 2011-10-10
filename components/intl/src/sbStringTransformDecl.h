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

#ifndef __SB_STRINGTRANSFORMDECL_H__
#define __SB_STRINGTRANSFORMDECL_H__

#include <sbIStringTransform.h>

/* Use this macro when declaring classes that implement this interface. */
#define NS_DECL_SBISTRINGTRANSFORM_IMPL \
  NS_SCRIPTABLE NS_IMETHOD NormalizeString(const nsAString & aCharset, PRUint32 aTransformFlags, const nsAString & aInput, nsAString & _retval); \
  NS_SCRIPTABLE NS_IMETHOD ConvertToCharset(const nsAString & aDestCharset, const nsAString & aInput, nsAString & _retval); \
  NS_SCRIPTABLE NS_IMETHOD GuessCharset(const nsAString & aInput, nsAString & _retval); 

#endif /* __SB_STRINGTRANSFORMDECL_H__ */
