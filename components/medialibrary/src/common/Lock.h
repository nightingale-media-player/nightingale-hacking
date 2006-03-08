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
 * \file Lock.h
 * \brief 
 */

#pragma once

// Set this to 1 for the WIN32 specific code, 0 for NSPR
#define USE_WINDOWS 1

// INCLUDES ===================================================================
#if USE_WINDOWS
#include <windows.h>
#endif
#include "prmon.h"



// NAMESPACES =================================================================
namespace sbCommon
{

class CLockImpl;

// CLASSES ====================================================================
/**
 * \class CLock
 * \brief
 */
class CLock
{
public:
  CLock();
  virtual ~CLock();

  PRStatus Lock();
  PRStatus Unlock();

protected: 
  CLockImpl *m_pLock;
};

/**
 * \class CScopedLock
 * \brief 
 */
class CScopedLock
{
public:
  CScopedLock(CLock *lock);
  virtual ~CScopedLock();

protected:
  CLock *m_pLock;
};

/**
* \class CLockImpl
* \brief
*/
class CLockImpl
{
public:
  CLockImpl();
  virtual ~CLockImpl();

  PRStatus Lock();
  PRStatus Unlock();

protected:

#if USE_WINDOWS
  CRITICAL_SECTION m_Lock;
#else
  PRMonitor *m_pMonitor;
#endif

};

} //namespace sbCommon