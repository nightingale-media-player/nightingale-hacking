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
 * \file Lock.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "stdafx.h"
#include "Lock.h"

// NAMESPACES =================================================================
namespace sbCommon
{

// CLASSES ====================================================================
//=============================================================================
// CLock
//=============================================================================
//-----------------------------------------------------------------------------
CLock::CLock()
: m_pLock(NULL)
{
  m_pLock = new CLockImpl;
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CLock::~CLock()
{
  if(m_pLock) delete m_pLock;
} //dtor

//-----------------------------------------------------------------------------
PRStatus CLock::Lock()
{
  PRStatus status = PR_FAILURE;
  if(m_pLock)
  {
    status = m_pLock->Lock();
  }

  return status;
} //Lock

//-----------------------------------------------------------------------------
PRStatus CLock::Unlock()
{
  PRStatus status = PR_FAILURE;
  if(m_pLock)
  {
    status = m_pLock->Unlock();
  }

  return status;
} //Unlock

//=============================================================================
// CScopedLock Class
//=============================================================================
//-----------------------------------------------------------------------------
CScopedLock::CScopedLock(CLock *lock)
: m_pLock(NULL)
{
  if(lock)
  {
    m_pLock = lock;
    m_pLock->Lock();
  }
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CScopedLock::~CScopedLock()
{
  if(m_pLock)
  {
    m_pLock->Unlock();
  }
} //dtor


#if USE_WINDOWS

//=============================================================================
//  CLockImpl Class (Windows 32)
//=============================================================================
//-----------------------------------------------------------------------------
CLockImpl::CLockImpl()
{
  InitializeCriticalSection(&m_Lock);
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CLockImpl::~CLockImpl()
{
  DeleteCriticalSection(&m_Lock);
} //dtor

//-----------------------------------------------------------------------------
PRStatus CLockImpl::Lock()
{
  EnterCriticalSection(&m_Lock);
  return PR_SUCCESS;
} //Lock

//-----------------------------------------------------------------------------
PRStatus CLockImpl::Unlock()
{
  LeaveCriticalSection(&m_Lock);
  return PR_SUCCESS;
} //Unlock

#else

//=============================================================================
//  CLockImpl Class (Netscape Portable Runtime)
//=============================================================================
//-----------------------------------------------------------------------------
CLockImpl::CLockImpl()
: m_pMonitor(PR_NewMonitor())
{
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CLockImpl::~CLockImpl()
{
  if (this->m_pMonitor)
    PR_DestroyMonitor(m_pMonitor);
} //dtor

//-----------------------------------------------------------------------------
PRStatus CLockImpl::Lock()
{
  if (!m_pMonitor)
    return PR_FAILURE;
  PR_EnterMonitor(m_pMonitor);
  return PR_SUCCESS;
} //Lock

//-----------------------------------------------------------------------------
PRStatus CLockImpl::Unlock()
{
  if (!m_pMonitor)
    return PR_FAILURE;
  return PR_ExitMonitor(m_pMonitor);
} //Unlock

#endif

} //namespace sbCommon