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

#ifndef __SBLOCALDATABASEMEDIALISTENUMERATIONLISTENER_H__
#define __SBLOCALDATABASEMEDIALISTENUMERATIONLISTENER_H__

#include <nsIMutableArray.h>

#include <sbIMediaListListener.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

class sbLocalMediaListBaseEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
    NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

    sbLocalMediaListBaseEnumerationListener();

  nsresult Init();
  nsresult GetArray(nsIArray ** aArray);
  nsresult GetArrayLength(PRUint32 * aLength);
  
  nsresult SetHasItems(PRBool aHasItems);

private:
  ~sbLocalMediaListBaseEnumerationListener();

protected:
  nsCOMPtr<nsIMutableArray> mArray;
  PRPackedBool mHasItems;
};


#endif // __SBLOCALDATABASEMEDIALISTENUMERATIONLISTENER_H__
