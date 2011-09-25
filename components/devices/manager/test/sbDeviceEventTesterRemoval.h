/* vim: set sw=2 :miv */
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

#include <nsIRunnable.h>
#include "sbIDeviceEventListener.h"

#include <nsAutoPtr.h>
#include <nsTArray.h>
#include <nsDataHashtable.h>

/*
 * event listener testing class - test removals
 */

class sbDeviceEventTesterRemovalHelper;

class sbDeviceEventTesterRemoval: public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE
  
  sbDeviceEventTesterRemoval();

private:
  ~sbDeviceEventTesterRemoval();

protected:
  nsTArray<nsRefPtr<sbDeviceEventTesterRemovalHelper> > mListeners;
};

class sbDeviceEventTesterRemovalHelper: public sbIDeviceEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEEVENTLISTENER

  enum ACTION_TYPE {
    ACTION_ADDREMOVE,   // add or remove listener
    ACTION_SET_FLAG,    // set a flag (to true or false)
                        // flags default to false
    ACTION_CHECK_FLAG,  // check the flag
                        // for set=true:  if flag is false, abort
                        // for set=false: if flag is set, do next (n) actions
                        //                else skip next (n) actions (do rest)
    ACTION_DISPATCH
  };
  
  struct ACTION {
    ACTION_TYPE type;
    PRBool set;
    nsCOMPtr<sbIDeviceEventListener> listener;
    PRUint32 flag;    // for set/check, which flag to use
    PRUint32 count;

    ACTION(ACTION_TYPE aType, PRBool aSet, sbIDeviceEventListener* aListener)
     : type(aType),
       set(aSet),
       listener(aListener)
    {}
    
    ACTION(ACTION_TYPE aType, PRBool aSet, PRUint32 aFlag, PRUint32 aCount)
     : type(aType),
       set(aSet),
       flag(aFlag),
       count(aCount)
    {}
  };
  
  sbDeviceEventTesterRemovalHelper(const char aName);
  
  nsresult AddAction(ACTION_TYPE aType,
                     PRBool aSet,
                     sbIDeviceEventListener* aListener)
  { return AddAction(ACTION(aType, aSet, aListener)); }
  nsresult AddAction(ACTION_TYPE aType, PRBool aSet, int aFlag, int aCount)
  { return AddAction(ACTION(aType, aSet, aFlag, aCount)); }
  nsresult AddAction(ACTION aAction);
  nsresult SetFlag(PRUint32 aFlag, PRBool aSet);

private:
  ~sbDeviceEventTesterRemovalHelper() {}

protected:
  char mName;
  nsTArray<ACTION> mActions;
  nsDataHashtableMT<nsUint32HashKey, PRBool> mFlags;
};
