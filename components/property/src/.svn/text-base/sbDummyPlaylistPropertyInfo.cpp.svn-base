/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#include "sbDummyPlaylistPropertyInfo.h"
#include "sbStandardOperators.h"

#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <nsITreeView.h>
#include <nsAutoPtr.h>

NS_IMPL_ISUPPORTS_INHERITED0(sbDummyPlaylistPropertyInfo, 
                             sbDummyPropertyInfo);

sbDummyPlaylistPropertyInfo::sbDummyPlaylistPropertyInfo()
{
}

nsresult
sbDummyPlaylistPropertyInfo::Init()
{
  nsresult rv;

  rv = sbDummyPropertyInfo::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  SetType(NS_LITERAL_STRING("smartmedialist_playlist"));

  nsAutoString op;
  nsRefPtr<sbPropertyOperator> propOp;
  
  rv = sbPropertyInfo::GetOPERATOR_EQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op,
                                  NS_LITERAL_STRING("&smart.playlist.is"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbPropertyInfo::GetOPERATOR_NOTEQUALS(op);
  NS_ENSURE_SUCCESS(rv, rv);
  propOp = new sbPropertyOperator(op,
                                  NS_LITERAL_STRING("&smart.playlist.isnot"));
  NS_ENSURE_TRUE(propOp, NS_ERROR_OUT_OF_MEMORY);
  rv = mOperators.AppendObject(propOp);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

