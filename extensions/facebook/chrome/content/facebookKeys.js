/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

if (typeof Facebook == 'undefined') {
  var Facebook = {};
}

// Our Facebook Application ID. If you are reusing this code for another purpose
// you should get another application id!
Facebook.appId = '149521478399279';

// The Facebook Application Permissions we are requesting.
Facebook.appPerms = 'publish_stream, offline_access, ' +
                    'user_interests, user_likes, ' +
                    'friends_interests,friends_likes';
