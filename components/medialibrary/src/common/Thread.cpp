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
 * \file Thread.cpp
 * \brief
 */

// INCLUDES ===================================================================
#include "stdafx.h"
#include "nscore.h"
#include "Thread.h" 
#include <xpcom/nsAutoLock.h>

// NAMESPACES =================================================================
namespace sbCommon
{

// CLASSES ====================================================================
//-----------------------------------------------------------------------------
CThread::CThread()
: m_Kill(PR_FALSE)
, m_Thread(nsnull)
, m_ThreadFunction(nsnull)
{
  m_ThreadData.pData = 0;
  m_ThreadData.pThread = nsnull;
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CThread::~CThread()
{
  Terminate();
} //dtor

//-----------------------------------------------------------------------------
PRStatus CThread::Create(threadfunction_t *pFunc, void *pArg, PRThreadPriority threadPriority /*= PR_PRIORITY_NORMAL*/, 
                PRThreadType threadType /*= PR_USER_THREAD*/, PRThreadScope threadScope /*= PR_USER_THREAD*/, PRThreadState threadState /*= PR_JOINABLE_THREAD*/)
{
  PRStatus status = PR_FAILURE;

  if(pFunc) 
  {
    m_ThreadData.pData = pArg;
    m_ThreadData.pThread = this;

    PRThread *pThread = PR_CreateThread(threadType, (void (PR_CALLBACK *)(void *))pFunc, (void *) &m_ThreadData, threadPriority, threadScope, threadState, 0);

    if(pThread)
    {
      m_Thread = pThread;
      status = PR_SUCCESS;
    }
  }

  return status;
} //Create

//-----------------------------------------------------------------------------
PRStatus CThread::Join()
{
  PRStatus status = PR_FAILURE;
  
  if(m_Thread)
  {
    status = PR_JoinThread(m_Thread);
    m_Thread = nsnull;
  }

  return status;
} //Join

//-----------------------------------------------------------------------------
PRStatus CThread::Terminate()
{
  m_Kill = PR_TRUE;
  return Join();
} //Terminate

//-----------------------------------------------------------------------------
void CThread::SetTerminate(PRBool bTerminate)
{
  m_Kill = bTerminate;
} //SetTerminate

//-----------------------------------------------------------------------------
PRBool CThread::IsTerminating()
{
  return m_Kill;
} //IsTerminating

//-----------------------------------------------------------------------------
PRThreadPriority CThread::GetPriority()
{
  if(m_Thread != nsnull)
  {
    return PR_GetThreadPriority(m_Thread);
  }

  return PR_PRIORITY_FIRST;
} //GetPriority

//-----------------------------------------------------------------------------
void CThread::SetPriority(PRThreadPriority threadPriority)
{
  if(m_Thread != nsnull)
  PR_SetThreadPriority(m_Thread, threadPriority);
} //SetPriority

//-----------------------------------------------------------------------------
PRThread *CThread::GetThread()
{
  return m_Thread;
} //GetThread

//-----------------------------------------------------------------------------
CThreadPool::CThreadPool()
: m_KillAll(PR_FALSE)
, m_pThreadPoolLock(PR_NewLock())
{
  NS_ASSERTION(m_pThreadPoolLock, "ThreadPool.m_pThreadPoolLock failed");
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CThreadPool::~CThreadPool()
{
  TerminateAll();
  if (m_pThreadPoolLock)
    PR_DestroyLock(m_pThreadPoolLock);
} //dtor

//-----------------------------------------------------------------------------
PRInt32 CThreadPool::Create(threadfunction_t *pFunc, void *pArg, const PRInt32 &nThreadCount /*= 1*/, PRThreadPriority threadPriority /*= PR_PRIORITY_NORMAL*/, 
    PRThreadType threadType /*= PR_USER_THREAD*/, PRThreadScope threadScope /*= PR_LOCAL_THREAD*/, PRThreadState threadState /*= PR_JOINABLE_THREAD*/)
{
  PRInt32 nSuccess = 0;

  for(PRInt32 nCurrent = 0; nCurrent < nThreadCount; nCurrent++)
  {
    CThread *pThread = new CThread();

    if(pThread)
    {
      PRStatus status = pThread->Create(pFunc, pArg, threadPriority, threadType, threadScope, threadState);
      if(status != PR_FAILURE)
      {
        nsAutoLock lock(m_pThreadPoolLock);
        m_ThreadPool.push_back(pThread);
        nSuccess++;
      }
    }
  }

  return nSuccess;
} //Create

//-----------------------------------------------------------------------------
PRInt32 CThreadPool::GetThreadCount()
{
  nsAutoLock lock(m_pThreadPoolLock);
  return m_ThreadPool.size();
} //GetThreadCount

//-----------------------------------------------------------------------------
CThread* CThreadPool::GetThread(PRInt32 nIndex)
{
  CThread *pThread = nsnull;
  nsAutoLock lock(m_pThreadPoolLock);

  if(nIndex < m_ThreadPool.size() &&
     m_ThreadPool[nIndex] != nsnull)
  {
    pThread = m_ThreadPool[nIndex];
  }

  return pThread;
} //GetThread

//-----------------------------------------------------------------------------
PRStatus CThreadPool::Terminate(PRInt32 nIndex)
{
  PRStatus status = PR_FAILURE;
  nsAutoLock lock(m_pThreadPoolLock);

  if(nIndex < m_ThreadPool.size() &&
     m_ThreadPool[nIndex] != nsnull)
  {
    status = m_ThreadPool[nIndex]->Terminate();
  }

  return status;
} //Terminate

//-----------------------------------------------------------------------------
PRInt32 CThreadPool::TerminateAll()
{
  PRInt32 nSuccess = 0;
  nsAutoLock lock(m_pThreadPoolLock);
  
  PRInt32 nThreadCount = m_ThreadPool.size();
  PRInt32 nThread = 0;

  for(; nThread < nThreadCount; nThread++)
  {
    if(m_ThreadPool[nThread] != nsnull)
    {
      PRStatus status = m_ThreadPool[nThread]->Terminate();
      if(status != PR_FAILURE)
      {
        delete m_ThreadPool[nThread];
        m_ThreadPool[nThread] = nsnull;

        nSuccess++;
      }
    }
  }

  return nSuccess;
} //TerminateAll

//-----------------------------------------------------------------------------
void CThreadPool::SetTerminate(PRInt32 nIndex, PRBool bTerminate)
{
  PRStatus status = PR_FAILURE;
  nsAutoLock lock(m_pThreadPoolLock);

  if(nIndex < m_ThreadPool.size() &&
     m_ThreadPool[nIndex] != nsnull)
  {
    m_ThreadPool[nIndex]->SetTerminate(bTerminate);
  }

  return;
} //SetTerminate

//-----------------------------------------------------------------------------
PRInt32 CThreadPool::SetTerminateAll(PRBool bTerminate)
{
  PRInt32 nSuccess = 0;
  nsAutoLock lock(m_pThreadPoolLock);

  PRInt32 nThreadCount = m_ThreadPool.size();
  PRInt32 nThread = 0;

  for(; nThread < nThreadCount; nThread++)
  {
    if(m_ThreadPool[nThread] != nsnull)
    {
      m_ThreadPool[nThread]->SetTerminate(bTerminate);
      nSuccess++;
    }
  }

  return nThreadCount;
} //SetTerminateAll

} //namespace sbCommon