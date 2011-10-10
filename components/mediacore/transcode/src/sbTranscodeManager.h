/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

/**
 * \file sbTranscodeManager.h
 * \brief 
 */

#ifndef __TRANSCODE_MANAGER_H__
#define __TRANSCODE_MANAGER_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <prlock.h>
#include "sbITranscodeManager.h"
#include "sbITranscodeJob.h"
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#include <set>
#include <list>

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
class sbTranscodeManager : public sbITranscodeManager
{
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODEMANAGER

  sbTranscodeManager();
  virtual ~sbTranscodeManager();

  static sbTranscodeManager *GetSingleton();
  static void DestroySingleton();

private:
  typedef std::list<nsCString> contractlist_t;
  contractlist_t m_ContractList;
  PRLock *m_pContractListLock;
};

extern sbTranscodeManager *gTranscodeManager;

#endif // __TRANSCODE_MANAGER_H__

