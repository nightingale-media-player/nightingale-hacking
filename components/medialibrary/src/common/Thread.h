/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 * \file Thread.h
 * \brief
 */

#pragma once

// INCLUDES ===================================================================
#include <vector>
#include "prthread.h"
#include <nspr/prlock.h>

// NAMESPACES =================================================================
namespace sbCommon
{

// FORWARD REFERENCES =========================================================
class CThread;
class CThreadPool;

// TYPES ======================================================================
typedef void (PR_CALLBACK *threadfunction_t)(void *);

typedef struct threaddata_t
{
  CThread* pThread;
  void*    pData;
} threaddata_t;

// CLASSES ====================================================================
/**
 * \class CThread
 * \brief
 */
class CThread
{
public:
  CThread();
  virtual ~CThread();

public:
  PRStatus Create(threadfunction_t *pFunc, void *pArg, PRThreadPriority threadPriority = PR_PRIORITY_NORMAL, 
                  PRThreadType threadType = PR_USER_THREAD, PRThreadScope threadScope = PR_LOCAL_THREAD, PRThreadState threadState = PR_JOINABLE_THREAD);
  PRStatus Join();
  PRStatus Terminate();

  void SetTerminate(PRBool bTerminate);
  PRBool IsTerminating();

  PRThreadPriority GetPriority();
  void SetPriority(PRThreadPriority threadPriority);

  PRThread *GetThread();

private:
  PRBool m_Kill;

  PRThread *m_Thread;
  threadfunction_t *m_ThreadFunction;

  threaddata_t m_ThreadData;
};

/**
 * \class CThreadPool
 * \brief 
 */
class CThreadPool
{
public:
  CThreadPool();
  virtual ~CThreadPool();

public:
  PRInt32 Create(threadfunction_t *pFunc, void *pArg, const PRInt32 &nThreadCount = 1, PRThreadPriority threadPriority = PR_PRIORITY_NORMAL, 
    PRThreadType threadType = PR_USER_THREAD, PRThreadScope threadScope = PR_LOCAL_THREAD, PRThreadState threadState = PR_JOINABLE_THREAD);
  
  PRInt32 GetThreadCount();
  CThread *GetThread(PRInt32 nIndex);
  
  PRStatus Terminate(PRInt32 nIndex);
  PRInt32 TerminateAll();

  void SetTerminate(PRInt32 nIndex, PRBool bTerminate);
  PRInt32 SetTerminateAll(PRBool bTerminate);

private:
  PRBool m_KillAll;

  typedef std::vector<CThread *> threadpool_t;
  threadpool_t m_ThreadPool;

  PRLock* m_pThreadPoolLock;
};

} //namespace sbCommon