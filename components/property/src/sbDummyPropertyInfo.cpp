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

#include "sbDummyPropertyInfo.h"
#include "sbStandardOperators.h"
#include "sbPropertyOperator.h"
#include "sbPropertyInfo.h"
#include <nsAutoPtr.h>

#include <nsIStringBundle.h>

#include <nsArrayEnumerator.h>
#include <sbIPropertyManager.h>
#include <sbLockUtils.h>

NS_IMPL_ISUPPORTS_INHERITED1(sbDummyPropertyInfo, sbPropertyInfo, sbIDummyPropertyInfo)

sbDummyPropertyInfo::sbDummyPropertyInfo()
{
}

sbDummyPropertyInfo::~sbDummyPropertyInfo() {
}

nsresult
sbDummyPropertyInfo::Init() {
  nsresult rv;

  rv = sbPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  SetUserViewable(PR_FALSE);
  SetUserEditable(PR_FALSE);
  SetRemoteReadable(PR_FALSE);
  SetRemoteWritable(PR_FALSE);
  
  return NS_OK;
}

NS_IMETHODIMP 
sbDummyPropertyInfo::Validate(const nsAString & aValue, PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbDummyPropertyInfo::Sanitize(const nsAString & aValue, nsAString & _retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDummyPropertyInfo::Format(const nsAString & aValue, nsAString & _retval)
{
  _retval = aValue;
  return NS_OK;
}

NS_IMETHODIMP
sbDummyPropertyInfo::MakeSearchable(const nsAString & aValue, nsAString & _retval)
{
  _retval = aValue;
  return NS_OK;
}

