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
 * \file Singleton.h
 * \brief
 */

#ifndef _SINGLETON_H_
#define _SINGLETON_H_

// NAMESPACES =================================================================
namespace sbCommon
{

// CLASSES ====================================================================
template < class T >
class CSingleton
{
public:
  static T * CreateSingleton() 
  {
    if(m_Instance == NULL)
    {
      m_Instance = new T;
    }

    return m_Instance;
  }
  static void DestroySingleton() 
  {
    if (m_Instance != NULL) {
      delete m_Instance;
      m_Instance = NULL;
    }
  }
  static T * GetSingleton()
  {
    return m_Instance;
  }
protected:
  static T * m_Instance;
};

template <class T> T *CSingleton< T >::m_Instance = NULL;

} //namespace sbCommon

#endif//_SINGLETON_H_
