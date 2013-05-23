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
 * \file  sbBaseMediacoreFactory.h
 * \brief Songbird Base Mediacore Factory Definition.
 */

#ifndef __SB_BASEMEDIACOREFACTORY_H__
#define __SB_BASEMEDIACOREFACTORY_H__

#include <sbIMediacoreFactory.h>

#include <mozilla/ReentrantMonitor.h>
#include <nsStringGlue.h>

class sbBaseMediacoreFactory : public sbIMediacoreFactory
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIACOREFACTORY

  sbBaseMediacoreFactory();

  nsresult InitBaseMediacoreFactory();

  nsresult SetContractID(const nsAString &aContractID);
  nsresult SetName(const nsAString &aName);

  //Override me, see cpp file for implementation notes.
  virtual nsresult OnInitBaseMediacoreFactory();

  // Override me, see cpp file for implementation notes.
  virtual nsresult OnGetCapabilities(sbIMediacoreCapabilities **aCapabilities);
  
  // Override me, see cpp file for implementation notes.
  virtual nsresult OnCreate(const nsAString &aInstanceName, 
                            sbIMediacore **_retval);

protected:
  virtual ~sbBaseMediacoreFactory();

  mozilla::ReentrantMonitor mMonitor;
  
  nsString mContractID;
  nsString mName;
};

#endif /* __SB_BASEMEDIACOREFACTORY_H__ */
