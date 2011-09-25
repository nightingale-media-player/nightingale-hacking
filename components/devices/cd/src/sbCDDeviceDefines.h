/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbCDDeviceDefines_h_
#define sbCDDeviceDefines_h_

//------------------------------------------------------------------------------
// CD device sbIDeviceMarshall Defines

#define SB_CDDEVICE_MARSHALL_NAME  \
  "sbCDDeviceMarshall"

#define SB_CDDEVICE_MARSHALL_CONTRACTID \
  "@getnightingale.com/Nightingale/CDDeviceMarshall;1"

#define SB_CDDEVICE_MARSHALL_CLASSNAME \
  "NightingaleCDDeviceMarshall"

#define SB_CDDEVICE_MARSHALL_DESC \
  "Nightingale CD Device Marshall"

#define SB_CDDEVICE_MARSHALL_CID \
  {0x1115bc18, 0x1dd2, 0x11b2, {0xb4, 0x41, 0xaa, 0x8a, 0x5f, 0x59, 0x75, 0x1d}}

#define SB_CDDEVICE_MARSHALL_IID \
  {0x203c2e84, 0x1dd2, 0x11b2, {0xb6, 0xaf, 0xb2, 0xf6, 0x44, 0x87, 0xcf, 0x9d}}

//------------------------------------------------------------------------------
// CD device sbIDeviceController Defines

#define SB_CDDEVICE_CONTROLLER_CONTRACTID \
  "@getnightingale.com/Nightingale/CDDeviceController;1"

#define SB_CDDEVICE_CONTROLLER_CLASSNAME \
  "NightingaleCDDeviceController"

#define SB_CDDEVICE_CONTROLLER_DESC \
  "Nightingale CD Device Controller"

#define SB_CDDEVICE_CONTROLLER_CID \
  {0x45684bb8, 0x1dd2, 0x11b2, {0xac, 0xb3, 0x95, 0xcc, 0x54, 0x81, 0x87, 0x17}}

#define NO_CD_INFO_FOUND_DIALOG_URI \
  "chrome://nightingale/content/xul/device/cdInfoNotFoundDialog.xul"
#define MULTI_CD_INFO_FOUND_DIALOG_URI \
  "chrome://nightingale/content/xul/device/multiCDLookupResultsDialog.xul"

#endif  // sbCDDeviceDefines_h_

