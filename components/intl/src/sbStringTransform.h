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

#ifndef __SB_STRINGTRANSFORM_H__
#define __SB_STRINGTRANSFORM_H__

#include <sbIStringTransform.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>

#include <sbStringTransformImpl.h>

#include "sbArticlesData.h"

class sbStringTransformImpl;
 
class sbStringTransform : public sbIStringTransform
{
public:
  sbStringTransform();

  NS_DECL_ISUPPORTS
  NS_DECL_SBISTRINGTRANSFORM

  nsresult Init();

protected:
  ~sbStringTransform();

private:
  sbStringTransformImpl* mImpl;
};

#define SB_STRINGTRANSFORM_CLASSNAME "sbStringTransform"


#endif /* __SB_STRINGTRANSFORM_H__ */
