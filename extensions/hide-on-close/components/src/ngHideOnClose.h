/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2014
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#ifndef _HIDEONCLOSE_H_
#define _HIDEONCLOSE_H_

#include "ngIHideOnClose.h"

#include <gtk/gtk.h>

#define HIDEONCLOSE_CONTRACTID "@getnightingale.com/Nightingale/ngHideOnClose;1"
#define HIDEONCLOSE_CLASSNAME  "hide-on-close"

// ab61d3be-cfba-11e3-91bb-5404a6486fb4
#define HIDEONCLOSE_CID   \
  { 0xab61d3be, 0xcfba, 0x11e3, \
  { 0x91, 0xbb, 0x54, 0x04,     \
    0xa6, 0x48, 0x6f, 0xb4 } }

class ngHideOnClose : public ngIHideOnClose
{              
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NGIHIDEONCLOSE

    ngHideOnClose();
    virtual ~ngHideOnClose();
};

#endif //_HIDEONCLOSE_H_
