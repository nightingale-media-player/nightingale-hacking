/* vim: set sw=2 :*/
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#include <nsIComponentManager.h>
#include <nsIObserver.h>

struct nsModuleComponentInfo;

class sbDistHelperEnvWriter : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  sbDistHelperEnvWriter();
  
  static NS_METHOD RegisterSelf(nsIComponentManager*         aCompMgr,
                                nsIFile*                     aPath,
                                const char*                  aLoaderStr,
                                const char*                  aType,
                                const nsModuleComponentInfo* aInfo);

  static NS_METHOD UnregisterSelf(nsIComponentManager *aCompMgr,
                                  nsIFile *aPath,
                                  const char *aLoaderStr,
                                  const nsModuleComponentInfo *aInfo);

protected:
  /**
   * This method is called when there is an update pending; it does the bulk
   * of the work preparing the file used to pass data to disthelper.
   */
  nsresult OnUpdatePending(nsIFile *aUpdateDir);

private:
  ~sbDistHelperEnvWriter();

protected:
};

#define SB_DISTHELPER_ENV_WRITER_CID                  \
  /* {6325FDE4-F1EE-47f9-A40E-306B877A2BC2} */        \
  { 0x6325fde4, 0xf1ee, 0x47f9,                       \
    { 0xa4, 0xe, 0x30, 0x6b, 0x87, 0x7a, 0x2b, 0xc2 } \
  }

#define SB_DISTHELPER_ENV_WRITER_CONTRACTID           \
  "@getnightingale.com/tools/disthelper/update/env;1"
