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
 * \file Wait.h
 * \brief
 */

#pragma once

// INCLUDES ===================================================================
#include <xpcom/nscore.h>

#if defined(_WIN32)
#include <windows.h>
#else
#include <xpcom/nsAutoLock.h>
#endif

// DEFINES ====================================================================
#define SB_WAIT_SUCCESS (1)
#define SB_WAIT_FAILURE (0)

#define SB_WAIT_TIMEOUT (-1)

// NAMESPACES =================================================================
namespace sbCommon
{

// FORWARD REFERENCES =========================================================
class CWaitImpl;

// CLASSES ====================================================================
/**
 * \class CWait
 * \brief 
 */
class CWait
{
public:
  CWait();
  virtual ~CWait();

  PRBool Set();
  PRBool Reset();
  PRInt32 Wait(PRInt32 nMaxWaitTime = -1);

protected:
  CWaitImpl *m_Impl;

};

/** 
 * \class CWaitImpl
 * \brief 
 */
class CWaitImpl
{
public:
  CWaitImpl();
  virtual ~CWaitImpl();

  PRBool Set();
  PRBool Reset();
  
  PRInt32 Wait(PRInt32 nMaxWaitTime = -1); //in milliseconds.

protected:

#if defined(_WIN32)
  HANDLE m_hEvent;
#else
  PRMonitor *m_Monitor;
  PRBool m_IsSet;
#endif

};

} //namespace sbCommon