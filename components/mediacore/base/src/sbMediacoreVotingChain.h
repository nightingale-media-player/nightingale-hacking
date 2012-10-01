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

/** 
 * \file  sbMediacoreVotingChain.h
 * \brief Songbird Mediacore Voting Chain Definition.
 */

#ifndef __SB_MEDIACOREVOTINGCHAIN_H__
#define __SB_MEDIACOREVOTINGCHAIN_H__

#include <sbIMediacoreVotingChain.h>

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>

#include <map>

#include <sbIMediacore.h>

class sbMediacoreVotingChain : public sbIMediacoreVotingChain
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREVOTINGCHAIN

  sbMediacoreVotingChain();

  nsresult Init();

  nsresult AddVoteResult(PRUint32 aVoteResult, 
                         sbIMediacore *aMediacore);

protected:
  virtual ~sbMediacoreVotingChain();

  PRLock *mLock;
  
  typedef std::map<PRUint32, nsCOMPtr<sbIMediacore> > votingmap_t;
  votingmap_t mResults;
};

#endif /* __SB_MEDIACOREVOTINGCHAIN_H__ */
