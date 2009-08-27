/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#ifndef SBIDEFAULTBASEDEVICECAPABILITIESREGISTRAR_H_
#define SBIDEFAULTBASEDEVICECAPABILITIESREGISTRAR_H_

// Mozilla includes
#include <nsAutoLock.h>
#include <nsCOMArray.h>

// Songibrd includes
#include <sbIDeviceCapabilitiesRegistrar.h>

class sbIDevice;

/**
 *   This is a base class for implementing a default capabilities registrar for
 * a device.
 *   Asside from providing additional capabilities implementation it also
 * contains a rudimentary algorithm for choosing a transcoding profile. This
 * algorithm compares the source item's media format, the transcoding profiles
 * and the devices capabilities to find the profile that is closest match
 * between the item and the device. Well that's the ultimate goal. Right now
 * it just picks the first viable match.
 */
class sbDefaultBaseDeviceCapabilitiesRegistrar:
        public sbIDeviceCapabilitiesRegistrar
{
public:
  NS_DECL_SBIDEVICECAPABILITIESREGISTRAR

  sbDefaultBaseDeviceCapabilitiesRegistrar();

protected:
  virtual ~sbDefaultBaseDeviceCapabilitiesRegistrar();

private:
  typedef nsCOMArray<sbITranscodeProfile> TranscodeProfiles;

  PRLock * mLock;
  /**
   * Cached collection of profiles suitable for this device
   */
  TranscodeProfiles mTranscodeProfiles;
  /**
   * true if we've built the cache
   */
  bool mTranscodeProfilesBuilt;

  /**
   * Returns a list of transcode profiles that the device supports
   * \param aDevice the device to retrieve the profiles for.
   * \param aProfiles the list of profiles that were found
   * \return NS_OK if successful else some NS_ERROR value
   */
  nsresult GetSupportedTranscodeProfiles(sbIDevice * aDevice,
                                         TranscodeProfiles ** aProfiles);

  /** For each transcoding profile property in aPropertyArray, look up a
   *  preference in aDevice starting with aPrefNameBase, and set the property
   *  value to the preference value if any.
   */
  nsresult ApplyPropertyPreferencesToProfile(sbIDevice *aDevice,
                                             nsIArray *aPropertyArray,
                                             nsString aPrefNameBase);
};

/**
 * Map entry figuring out the container format and codec given an extension or
 * mime type
 */
struct sbExtensionToContentFormatEntry_t {
  char const * Extension;
  char const * MimeType;
  char const * ContainerFormat;
  char const * Codec;
  PRUint32 Type;
};

extern sbExtensionToContentFormatEntry_t const
  MAP_FILE_EXTENSION_CONTENT_FORMAT[];
extern PRUint32 const MAP_FILE_EXTENSION_CONTENT_FORMAT_LENGTH;

#endif /* SBIDEFAULTBASEDEVICECAPABILITIESREGISTRAR_H_ */

