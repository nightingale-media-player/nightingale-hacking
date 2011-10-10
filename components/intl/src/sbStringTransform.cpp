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

#include "sbStringTransform.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbStringTransform, 
                              sbIStringTransform)

sbStringTransform::sbStringTransform()
: mImpl(nsnull)
{
}

sbStringTransform::~sbStringTransform()
{
  if(mImpl) {
    delete mImpl;
  }
}

nsresult 
sbStringTransform::Init()
{
  mImpl = new sbStringTransformImpl();
  NS_ENSURE_TRUE(mImpl, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mImpl->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbArticlesData::EnsureLoaded();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbStringTransform::NormalizeString(const nsAString & aCharset, 
                                   PRUint32 aTransformFlags, 
                                   const nsAString & aInput, 
                                   nsAString & _retval)
{
  return mImpl->NormalizeString(aCharset, aTransformFlags, aInput, _retval);
}

NS_IMETHODIMP 
sbStringTransform::ConvertToCharset(const nsAString & aDestCharset, 
                                    const nsAString & aInput, 
                                    nsAString & _retval)
{
  return mImpl->ConvertToCharset(aDestCharset, aInput, _retval);
}

NS_IMETHODIMP 
sbStringTransform::GuessCharset(const nsAString & aInput, 
                                nsAString & _retval)
{
  return mImpl->GuessCharset(aInput, _retval);
}

NS_IMETHODIMP
sbStringTransform::RemoveArticles(const nsAString & aInput,
                                  const nsAString & aLanguage,
                                  nsAString &_retval)
{
  return sbArticlesData::RemoveArticles(aInput, aLanguage, _retval);
}

