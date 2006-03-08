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
 * \file Wait.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "stdafx.h"
#include "Wait.h"

// NAMESPACES =================================================================
namespace sbCommon
{

// CLASSES ====================================================================
//=============================================================================
// CWait Class
//=============================================================================
//-----------------------------------------------------------------------------
CWait::CWait()
: m_Impl(NULL)
{
  m_Impl = new CWaitImpl();
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CWait::~CWait()
{
  if(m_Impl) delete m_Impl;
} //dtor

//-----------------------------------------------------------------------------
PRBool CWait::Set()
{
  if(m_Impl) return m_Impl->Set();
  return PR_FALSE;
} //Set

//-----------------------------------------------------------------------------
PRBool CWait::Reset()
{
  if(m_Impl) return m_Impl->Reset();
  return PR_FALSE;
} //Reset

//-----------------------------------------------------------------------------
PRInt32 CWait::Wait(PRInt32 nMaxWaitTime /*= -1*/)
{
  if(m_Impl) return m_Impl->Wait(nMaxWaitTime);
  return SB_WAIT_FAILURE;
} //Wait

#if defined (_WIN32)
//=============================================================================
// CWaitImpl Class
//=============================================================================
//-----------------------------------------------------------------------------
CWaitImpl::CWaitImpl()
: m_hEvent(NULL)
{
  HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
  if(hEvent != INVALID_HANDLE_VALUE &&
     hEvent != NULL)
  {
    m_hEvent = hEvent;
  }
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CWaitImpl::~CWaitImpl()
{
  if(m_hEvent != INVALID_HANDLE_VALUE && 
     m_hEvent != NULL)
  {
    CloseHandle(m_hEvent);
    m_hEvent = NULL;
  }
} //dtor

//-----------------------------------------------------------------------------
PRBool CWaitImpl::Set()
{
  PRBool ret = PR_FALSE;

  if(m_hEvent)
  {
    ret = SetEvent(m_hEvent);
  }

  return ret;
} //Set

//-----------------------------------------------------------------------------
PRBool CWaitImpl::Reset()
{
  PRBool ret = PR_FALSE;

  if(m_hEvent)
  {
    ret = ResetEvent(m_hEvent);
  }

  return ret;
} //Reset

//-----------------------------------------------------------------------------
PRInt32 CWaitImpl::Wait(PRInt32 nMaxWaitTime /*= -1*/)
{
  PRInt32 ret = 0;

  if(m_hEvent)
  {
    DWORD nWaitTime = nMaxWaitTime;
    if(nMaxWaitTime == -1) nWaitTime = INFINITE;

    DWORD nRet = WaitForSingleObject(m_hEvent, nWaitTime);

    switch(nRet)
    {
      case WAIT_ABANDONED: ret = SB_WAIT_FAILURE; break;
      case WAIT_TIMEOUT: ret = SB_WAIT_TIMEOUT; break;
      default: ret = SB_WAIT_SUCCESS;
    }

  }
  
  return ret;
} //Wait

#else
//=============================================================================
// CWaitImpl Class (The Rest).
//=============================================================================
//-----------------------------------------------------------------------------
CWaitImpl::CWaitImpl()
: m_Monitor(NULL)
, m_IsSet(PR_FALSE)
{
  PRMonitor *mon = PR_NewMonitor();
  if(mon)
  {
    m_Monitor = mon;
  }
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CWaitImpl::~CWaitImpl()
{
  if(m_Monitor)
  {
    PR_DestroyMonitor(m_Monitor);
  }
} //dtor

//-----------------------------------------------------------------------------
PRBool CWaitImpl::Set()
{
  PRBool ret = PR_FALSE;
  
  if(m_Monitor)
  {
    if(!PR_NotifyAll(m_Monitor)) m_IsSet = PR_TRUE;
  }
  
  return ret;
} //Set

//-----------------------------------------------------------------------------
PRBool CWaitImpl::Reset()
{
  PRBool ret = PR_FALSE;
  
  if(m_Monitor)
  {
    m_IsSet = PR_FALSE;
    ret = PR_TRUE;
  }
  
  return ret;
} //Reset

//-----------------------------------------------------------------------------
PRInt32 CWaitImpl::Wait(PRInt32 nMaxWaitTime /*= -1*/)
{
  PRInt32 ret = 0;

  if(m_Monitor && !m_IsSet)
  {
    PRUint32 nTheWait = nMaxWaitTime == -1 ? PR_INTERVAL_NO_TIMEOUT : nMaxWaitTime;
    ret = PR_Wait(m_Monitor, nTheWait);
    
    if(ret == PR_SUCCESS) ret = 1;
    else if(ret == PR_FAILURE) ret = 0;
  }
  
  return ret;
} //Wait

#endif

} //namespace sbCommon