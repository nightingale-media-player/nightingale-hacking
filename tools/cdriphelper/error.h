/* vim: le=unix sw=2 : */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#ifndef _REGHELPER_ERROR_H__
#define _REGHELPER_ERROR_H__

#define RH_OK                        0
#define RH_ERROR_USER                1
#define RH_ERROR_INIT_CREATEKEY      2
#define RH_ERROR_QUERY_KEY           3
#define RH_ERROR_UNKNOWN_KEY_TYPE    4
#define RH_ERROR_SETVALUE            5
#define RH_ERROR_OPENKEY             6
#define RH_ERROR_NOIMPL              7
#define RH_ERROR_QUERY_VALUE         8
#define RH_ERROR_SET_KEY             9
#define RH_ERROR_INVALID_ARG         10
#define RH_ERROR_COPYFILE_FAILED     11
#define RH_ERROR_DELETEFILE_FAILED   12
#define RH_ERROR_DELETEKEY_FAILED    13

#define RH_SUCCESS_NOACTION          128
#define RH_SUCCESS_CDRIP_NOT_INSTALLED 129

#endif /* _REGHELPER_ERROR_H__ */
