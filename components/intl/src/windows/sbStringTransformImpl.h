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

#ifndef __SB_STRINGTRANSFORMIMPL_H__
#define __SB_STRINGTRANSFORMIMPL_H__

#include <sbStringTransformDecl.h>
#include <nsTArray.h>

#include <windows.h>

#define C1 0
#define C2 1
#define C3 2

#define FIRSTTYPE C1
#define LASTTYPE C3
#define NTYPES (LASTTYPE-FIRSTTYPE+1)

class sbStringTransformImpl
{
public:
  sbStringTransformImpl();
  ~sbStringTransformImpl();

  NS_DECL_SBISTRINGTRANSFORM_IMPL

  nsresult Init();

  unsigned long MakeFlags(PRUint32 aFlags, 
                          nsTArray<WORD> aExcludeChars[NTYPES],
                          nsTArray<WORD> aIncludeChars[NTYPES]);
};

#endif /* __SB_STRINGTRANSFORMIMPL_H__ */
