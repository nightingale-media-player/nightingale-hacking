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

#ifndef __SB_GSTREAMERMEDIACOREFACTORY_H__
#define __SB_GSTREAMERMEDIACOREFACTORY_H__

#include <nsAutoPtr.h>
#include <nsIObserver.h>
#include <sbIMediacoreFactory.h>

#include <sbBaseMediacoreFactory.h>

class sbMediacoreCapabilities;

class sbGStreamerMediacoreFactory : public sbBaseMediacoreFactory,
                                    public nsIObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER
  NS_FORWARD_SBIMEDIACOREFACTORY(sbBaseMediacoreFactory::)

  sbGStreamerMediacoreFactory();

  // Mandatory Init method.
  nsresult Init();

  // sbBaseMediacoreFactory overrides
  virtual nsresult OnInitBaseMediacoreFactory();

  virtual nsresult OnGetCapabilities(sbIMediacoreCapabilities **aCapabilities);

  virtual nsresult OnCreate(const nsAString &aInstanceName, 
                            sbIMediacore **_retval);

protected:
  virtual ~sbGStreamerMediacoreFactory();
  virtual nsresult Shutdown();

  nsRefPtr<sbMediacoreCapabilities> mCapabilities;

};

#endif /* __SB_GSTREAMERMEDIACOREFACTORY_H__ */

