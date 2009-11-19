/* vim: set sw=2 :miv */
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

#ifndef __SB_MEDIACOREDATAREMOTES_H__
#define __SB_MEDIACOREDATAREMOTES_H__

/**
 * Faceplate DataRemotes
 */
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_BUFFERING     "faceplate.buffering"
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_PLAYING       "faceplate.playing"
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_PLAYINGVIDEO  "faceplate.playingvideo"
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_PAUSED        "faceplate.paused"
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_PLAY_URL      "faceplate.play.url"
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_SEENPLAYING   "faceplate.seenplaying"

/** 
 * Video Specific DataRemotes
 */
#define SB_MEDIACORE_DATAREMOTE_VIDEO_FULLSCREEN        "video.fullscreen"

/** 
 * Show remaining is special, when this data remote is set to true
 * it will cause the METADATA_LENGTH_STR dataremote to have it's value set
 * to the remaining amount of time left in the song.
 */
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_SHOWREMAINING "faceplate.showremainingtime"

#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_VOLUME      "faceplate.volume"
#define SB_MEDIACORE_DATAREMOTE_FACEPLATE_MUTE        "faceplate.mute"

/**
 * Equalizer DataRemotes
 */
#define SB_MEDIACORE_DATAREMOTE_EQ_ENABLED        "eq.enabled"
#define SB_MEDIACORE_DATAREMOTE_EQ_BAND_PREFIX    "eq.band."

/**
 * Metadata DataRemotes
 */
#define SB_MEDIACORE_DATAREMOTE_METADATA_ALBUM    "metadata.album"
#define SB_MEDIACORE_DATAREMOTE_METADATA_ARTIST   "metadata.artist"
#define SB_MEDIACORE_DATAREMOTE_METADATA_GENRE    "metadata.genre"
#define SB_MEDIACORE_DATAREMOTE_METADATA_TITLE    "metadata.title"
#define SB_MEDIACORE_DATAREMOTE_METADATA_URL      "metadata.url"
#define SB_MEDIACORE_DATAREMOTE_METADATA_IMAGEURL "metadata.imageURL"

#define SB_MEDIACORE_DATAREMOTE_METADATA_LENGTH       "metadata.length"
#define SB_MEDIACORE_DATAREMOTE_METADATA_LENGTH_STR   "metadata.length.str"

#define SB_MEDIACORE_DATAREMOTE_METADATA_POSITION     "metadata.position"
#define SB_MEDIACORE_DATAREMOTE_METADATA_POSITION_STR "metadata.position.str"

/**
 * Playlist DataRemotes
 */
#define SB_MEDIACORE_DATAREMOTE_PLAYLIST_SHUFFLE "playlist.shuffle"
#define SB_MEDIACORE_DATAREMOTE_PLAYLIST_REPEAT  "playlist.repeat"

#endif /*__SB_MEDIACOREDATAREMOTES_H__*/