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

#ifndef __SB_STANDARD_DEVICE_PROPERTIES_H__
#define __SB_STANDARD_DEVICE_PROPERTIES_H__

#define SB_DEVICE_PROPERTY_BASE                   "http://songbirdnest.com/device/1.0#"

#define SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY   "http://songbirdnest.com/device/1.0#accessCompatibility"
#define SB_DEVICE_PROPERTY_ACCESS_COMPATIBILITY_MUTABLE "http://songbirdnest.com/device/1.0#accessCompatibilityMutable"
#define SB_DEVICE_PROPERTY_BATTERY_LEVEL          "http://songbirdnest.com/device/1.0#batteryLevel"
#define SB_DEVICE_PROPERTY_CAPACITY               "http://songbirdnest.com/device/1.0#capacity"
#define SB_DEVICE_PROPERTY_DEFAULT_NAME           "http://songbirdnest.com/device/1.0#defaultName"
#define SB_DEVICE_PROPERTY_FIRMWARE_VERSION       "http://songbirdnest.com/device/1.0#firmwareVersion"
#define SB_DEVICE_PROPERTY_FREE_SPACE             "http://songbirdnest.com/device/1.0#freeSpace"
#define SB_DEVICE_PROPERTY_MANUFACTURER           "http://songbirdnest.com/device/1.0#manufacturer"
#define SB_DEVICE_PROPERTY_MODEL                  "http://songbirdnest.com/device/1.0#model"
#define SB_DEVICE_PROPERTY_MUSIC_ITEM_COUNT       "http://songbirdnest.com/device/1.0#musicItemCount"
#define SB_DEVICE_PROPERTY_MUSIC_TOTAL_PLAY_TIME  "http://songbirdnest.com/device/1.0#musicTotalPlayTime"
#define SB_DEVICE_PROPERTY_MUSIC_USED_SPACE       "http://songbirdnest.com/device/1.0#musicUsedSpace"
#define SB_DEVICE_PROPERTY_NAME                   "http://songbirdnest.com/device/1.0#name"
#define SB_DEVICE_PROPERTY_POWER_SOURCE           "http://songbirdnest.com/device/1.0#powerSource"
#define SB_DEVICE_PROPERTY_SERIAL_NUMBER          "http://songbirdnest.com/device/1.0#serialNumber"
#define SB_DEVICE_PROPERTY_SUPPORTS_AUTO_LAUNCH   "http://songbirdnest.com/device/1.0#supportsAutoLaunch"
#define SB_DEVICE_PROPERTY_TOTAL_USED_SPACE       "http://songbirdnest.com/device/1.0#totalUsedSpace"
#define SB_DEVICE_PROPERTY_VIDEO_ITEM_COUNT       "http://songbirdnest.com/device/1.0#videoItemCount"
#define SB_DEVICE_PROPERTY_VIDEO_TOTAL_PLAY_TIME  "http://songbirdnest.com/device/1.0#videoTotalPlayTime"
#define SB_DEVICE_PROPERTY_VIDEO_USED_SPACE       "http://songbirdnest.com/device/1.0#videoUsedSpace"
#define SB_DEVICE_PROPERTY_IMAGE_ITEM_COUNT       "http://songbirdnest.com/device/1.0#imageItemCount"
#define SB_DEVICE_PROPERTY_IMAGE_USED_SPACE       "http://songbirdnest.com/device/1.0#imageUsedSpace"
#define SB_DEVICE_PROPERTY_EXCLUDED_FOLDERS       "http://songbirdnest.com/device/1.0#excludedFolders"
#define SB_DEVICE_PROPERTY_SUPPORTS_REFORMAT      "http://songbirdnest.com/device/1.0#supportsReformat"
#define SB_DEVICE_PROPERTY_USB_VENDOR_ID          "http://songbirdnest.com/device/1.0#usbVendorId"
#define SB_DEVICE_PROPERTY_USB_PRODUCT_ID         "http://songbirdnest.com/device/1.0#usbProductId"
#define SB_DEVICE_PROPERTY_HIDDEN                 "http://songbirdnest.com/device/1.0#hidden"

/* Whether the volume mount needs user action; is "1" if needed, "0" otherwise */
#define SB_DEVICE_PROPERTY_MOUNT_NEEDS_USER_ACTION \
                                                  "http://songbirdnest.com/device/1.0#mountNeedsUserAction"

/* Windows device instance ID. */
#define SB_DEVICE_PROPERTY_DEVICE_INSTANCE_ID     "http://songbirdnest.com/device/1.0#deviceInstanceID"

#endif /* __SB_STANDARD_DEVICE_PROPERTIES_H__ */
