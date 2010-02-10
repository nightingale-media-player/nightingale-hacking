/* vim: set sw=2 : */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SB_TRANSCODEPROFILELOADER_H__
#define __SB_TRANSCODEPROFILELOADER_H__

#include <sbITranscodeProfile.h>

#include <nsIRunnable.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>

// forward declarations
class nsIDOMElement;
class nsIDOMNode;
class nsIDOMParser;
class nsIFile;
class sbTranscodeProfile;

class sbTranscodeProfileLoader : public sbITranscodeProfileLoader,
                                 public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBITRANSCODEPROFILELOADER
  NS_DECL_NSIRUNNABLE

  sbTranscodeProfileLoader();

private:
  ~sbTranscodeProfileLoader();

protected:
  enum ContainerType_t {
    CONTAINER_GENERIC,
    CONTAINER_AUDIO,
    CONTAINER_VIDEO
  };

  /**
   * Internal main-thread only implementation of LoadProfile
   * \param [in] mFile the file to load the profile from
   * \param [out] mProfile the profile loaded
   */
  nsresult LoadProfileInternal();

  /**
   * Get the transcode type specified in the XML node
   * \param [in] aTypeNode the DOM node with the type data
   * \return the type constant
   * \see sbITranscodeProfile::TRANSCODE_TYPE_*
   */
  nsresult GetType(nsIDOMNode* aTypeNode, PRUint32* _retval);
  /**
   * Process the given property node and fill out the property data
   * \param [in] aPropertyElement the XML element with property data to read
   * \return the parsed transcode property
   */
  nsresult ProcessProperty(nsIDOMElement* aPropertyElement,
                           sbITranscodeProfileProperty** _retval);
  /**
   * Process the given attribute node and fill out the attribute data
   * \param [in] aPropertyElement the XML element with attribute data to read
   * \return the parsed transcode attribute
   */
  nsresult ProcessAttribute(nsIDOMElement* aAttributeElement,
                            sbITranscodeProfileAttribute** _retval);
  /**
   * Process the container element and set the properties on the given profile
   * \param [in] aProfile the transcode profile to modify
   * \param [in] aContainerType the type of the container
   * \param [out] aContainer the container XML element to parse
   */
  nsresult ProcessContainer(sbTranscodeProfile* aProfile,
                            ContainerType_t aContainerType,
                            nsIDOMElement* aContainer);

  /**
   * The file to process
   * Used a parameter for LoadProfileInternal
   */
  nsCOMPtr<nsIFile> mFile;

  /**
   * The transcode profile to output
   * Used as a return value for LoadProfileInternal
   */
  nsRefPtr<sbTranscodeProfile> mProfile;

  /**
   * The XPCOM result of LoadProfileInternal
   */
  nsresult mResult;
};

#define SONGBIRD_TRANSCODEPROFILELOADER_CLASSNAME \
  "Songbird transcoding profile loader service"
#define SONGBIRD_TRANSCODEPROFILELOADER_CID \
  /* {C64F3BC9-263F-4c2a-8E4C-C6BFBD9AE1F0} */ \
  { 0xc64f3bc9, 0x263f, 0x4c2a, \
  { 0x8e, 0x4c, 0xc6, 0xbf, 0xbd, 0x9a, 0xe1, 0xf0 } }
#define SONGBIRD_TRANSCODEPROFILELOADER_CONTRACTID \
  "@songbirdnest.com/Songbird/Transcode/ProfileLoader;1"

#endif /* __SB_TRANSCODEPROFILELOADER_H__ */
