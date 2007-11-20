/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

#ifndef __METADATA_HANDLER_TAGLIB_H__
#define __METADATA_HANDLER_TAGLIB_H__

/* *****************************************************************************
 *******************************************************************************
 *
 * Taglib metadata handler.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  MetadataHandlerTaglib.h
* \brief Songbird MetadataHandlerTaglib Component Definition.
*/

/* *****************************************************************************
 *
 * Taglib metadata handler configuration.
 *
 ******************************************************************************/

/*
 * Taglib metadata handler XPCOM component definitions.
 */

#define SONGBIRD_METADATAHANDLERTAGLIB_CONTRACTID                              \
                        "@songbirdnest.com/Songbird/MetadataHandler/Taglib;1"
#define SONGBIRD_METADATAHANDLERTAGLIB_CLASSNAME                               \
                                    "Songbird Taglib Metadata Handler Component"
#define SONGBIRD_METADATAHANDLERTAGLIB_CID                                     \
{                                                                              \
    0x42C6FDBB,                                                                \
    0x59BB,                                                                    \
    0x48BE,                                                                    \
    { 0x82, 0x51, 0xD8, 0xF3, 0xBD, 0xAE, 0x0F, 0x63 }                         \
}


/* *****************************************************************************
 *
 * Taglib metadata handler imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsCOMPtr.h>
#include <nsIChannel.h>
#ifdef MOZ_CRASHREPORTER
#include <nsICrashReporter.h>
#endif
#include <nsICharsetDetectionObserver.h>
#include <nsIFileProtocolHandler.h>
#include <nsIURL.h>
#include <nsStringGlue.h>

/* Songbird imports. */
#include <sbIMetadataHandler.h>
#include <sbIMetadataValues.h>
#include <sbISeekableChannel.h>
#include <sbITagLibChannelFileIOManager.h>

/* TagLib imports. */
#include <id3v2tag.h>
#include <mp4itunestag.h>
#include <apetag.h>
#include <tag.h>
#include <tfile.h>
#include <xiphcomment.h>


/* *****************************************************************************
 *
 * Taglib metadata handler classes.
 *
 ******************************************************************************/

/*
 * sbMetadataHandlerTaglib class
 */

class sbMetadataHandlerTaglib : public sbIMetadataHandler,
                                public sbISeekableChannelListener,
                                public nsICharsetDetectionObserver
{
    /*
     * Taglib metadata handler configuration.
     *
     *   MAX_SCAN_BYTES         Maximum number of bytes to scan for tags.
     *
     *   The taglib library scans files looking for various markers such as tag
     * markers and frame markers.  In order to prevent scanning the entirety of
     * files that are corrupt or not of the expected format, the number of
     * scanned bytes is limited to the value specified by MAX_SCAN_BYTES.  If
     * this value is set to 0, the entire file will be scanned until a valid
     * marker is found.
     */

    static const long MAX_SCAN_BYTES = 100000;

    /*
     * mpTagLibChannelFileIOManager
     *                          TagLib sbISeekableChannel file IO manager.
     * mpFileProtocolHandler    File protocol handler instance.
     * mpMetadataValues         Read metadata values.
     * mpChannel                Metadata file channel.
     * mpSeekableChannel        Metadata channel.
     * mpURL                    Metadata file channel URL.
     * mMetadataChannelID       Metadata channel ID.
     * mMetadataChannelRestart  True when metadata channel must be restarted.
     * mCompleted               True when metadata reading is complete.
     * mMetadataPath            Path used to access metadata.
     * mpCrashReporter          Crash reporter service.
     */

private:
    nsCOMPtr<sbITagLibChannelFileIOManager>
                                mpTagLibChannelFileIOManager;
    nsCOMPtr<nsIFileProtocolHandler>
                                mpFileProtocolHandler;
    nsCOMPtr<sbIMetadataValues> mpMetadataValues;
    nsCOMPtr<nsIChannel>        mpChannel;
    nsCOMPtr<sbISeekableChannel>
                                mpSeekableChannel;
    nsCOMPtr<nsIURL>            mpURL;
    nsString                    mMetadataChannelID;
    PRBool                      mMetadataChannelRestart;
    PRBool                      mCompleted;
    nsCString                   mMetadataPath;
#ifdef MOZ_CRASHREPORTER
    nsCOMPtr<nsICrashReporter>  mpCrashReporter;
#endif


    /* Inherited interfaces. */
public:
    NS_DECL_ISUPPORTS
    NS_DECL_SBIMETADATAHANDLER
    NS_DECL_SBISEEKABLECHANNELLISTENER


    /*
     * Public taglib metadata handler services.
     */

    sbMetadataHandlerTaglib();

    virtual ~sbMetadataHandlerTaglib();

    nsresult FactoryInit();

    /* nsICharsetDetectionObserver */
    NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);

    /*
     * Private taglib metadata handler class services.
     *
     *   sNextChannelID         Next channel ID to use.
     */

private:
    static PRUint32             sNextChannelID;


    /*
     * Private taglib metadata handler ID3v2 services.
     */

private:
    void ReadID3v2Tags(
        TagLib::ID3v2::Tag          *pTag,
        const char                  *charset = 0);

    void AddID3v2Tag(
        TagLib::ID3v2::FrameListMap &frameListMap,
        const char                  *frameID,
        const char                  *metadataName,
        const char                  *charset = 0);


    /*
     * Private taglib metadata handler APE services.
     */

private:
    void ReadAPETags(
        TagLib::APE::Tag          *pTag);

    void AddAPETag(
        TagLib::APE::ItemListMap &itemListMap,
        const char                  *frameID,
        const char                  *metadataName);


    /*
     * Private taglib metadata handler Xiph services.
     */

private:
    void ReadXiphTags(
        TagLib::Ogg::XiphComment    *pTag);

    void AddXiphTag(
        TagLib::Ogg::FieldListMap   &fieldListMap,
        const char                  *fieldName,
        const char                  *metadataName);


    /*
     * Private taglib metadata handler MP4 services.
     */

private:
    void ReadMP4Tags(
        TagLib::MP4::Tag            *pTag);


    /*
     * Private taglib metadata handler services.
     */

private:
    nsresult ReadMetadata();

    void GuessCharset(
        TagLib::Tag                 *pTag,
        nsACString&                 _retval);

    TagLib::String ConvertCharset(
        TagLib::String              aString,
        const char                  *aCharset);

    void CompleteRead();

    PRBool ReadFile(
        TagLib::File                *pTagFile,
        const char                  *aCharset = 0);

    PRBool ReadMPEGFile(
        const char                  *filePath);

    PRBool ReadMP4File(
        const char                  *filePath);

    PRBool ReadOGGFile(
        const char                  *filePath);

    PRBool ReadFLACFile(
        const char                  *filePath);

    PRBool ReadMPCFile(
        const char                  *filePath);

    nsresult AddMetadataValue(
        const char                  *name,
        TagLib::String              value);

    nsresult AddMetadataValue(
        const char                  *name,
        TagLib::uint                value);

    void FixTrackDiscNumber(
        nsString                    numberKey,
        nsString                    totalKey);

    /*
     * Private charset detector members
     */

private:
    // these apply only to the last thing being detected
    nsCString mLastCharset;
    nsDetectionConfident mLastConfidence;
};


#endif /* __METADATA_HANDLER_TAGLIB_H__ */
