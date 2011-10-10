/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef sbTestMediacoreEventCreator_h
#define sbTestMediacoreEventCreator_h

#include "sbITestMediacoreEventCreator.h"

class sbTestMediacoreEventCreator : public sbITestMediacoreEventCreator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITESTMEDIACOREEVENTCREATOR
};

#define SB_TEST_MEDIACORE_EVENT_CREATOR_DESCRIPTION              \
  "Nightingale Test Mediacore Event Creator"
#define SB_TEST_MEDIACORE_EVENT_CREATOR_CONTRACTID               \
  "@getnightingale.com/mediacore/sbTestMediacoreEventCreator;1"
#define SB_TEST_MEDIACORE_EVENT_CREATOR_CLASSNAME                \
  "sbTestMediacoreEventCreator"

#define SB_TEST_MEDIACORE_EVENT_CREATOR_CID                \
{ /* 77CF73E7-FC6B-458b-969A-97945F4160B7 */               \
  0x77cf73E7,                                              \
  0xFc6b,                                                  \
  0x458b,                                                  \
  { 0x96, 0x9a, 0x97, 0x94, 0x5f, 0x41, 0x60, 0xb7 }       \
}

#endif /* sbTestMediacoreEventCreator_H_ */
