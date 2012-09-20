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
    0x62f2be31,                                                                \
    0x54cf,                                                                    \
    0x4728,                                                                    \
    { 0x84, 0xe8, 0x92, 0x3f, 0x52, 0x15, 0xb9, 0x2a }                         \
}

/* *****************************************************************************
 *
 * Taglib metadata handler imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsCOMPtr.h>
#include <nsIChannel.h>
#include <nsICharsetDetectionObserver.h>
#include <nsIFileProtocolHandler.h>
#include <nsIResProtocolHandler.h>
#include <nsIURL.h>
#include <nsStringGlue.h>
#include <nsMemory.h>
#include <nsTArray.h>
#include <nsAutoPtr.h>

/* Songbird imports. */
#include <sbIMetadataHandler.h>
#include <sbISeekableChannel.h>
#include <sbITagLibChannelFileIOManager.h>

/* TagLib imports. */
#include <fileref.h>
#include <mpegfile.h>
#include <asffile.h>
#include <vorbisfile.h>
#include <id3v1tag.h>
#include <id3v2tag.h>
#include <mp4file.h>
#include <mp4tag.h>
#include <apetag.h>
#include <tag.h>
#include <tfile.h>
#include <xiphcomment.h>
#include <attachedpictureframe.h>

#include <nsAutoLock.h>

/* *****************************************************************************
 *
 * Taglib metadata handler classes.
 *
 ******************************************************************************/

/*
 * sbMetadataHandlerTaglib class
 */

class nsICharsetDetector;

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
     * mpMetadataPropertyArray  Read metadata property values.
     * mpChannel                Metadata file channel.
     * mpSeekableChannel        Metadata channel.
     * mpURL                    Metadata file channel URL.
     * mMetadataChannelID       Metadata channel ID.
     * mMetadataChannelRestart  True when metadata channel must be restarted.
     * mCompleted               True when metadata reading is complete.
     * mMetadataPath            Path used to access metadata.  Always UTF-8.
     *                          (This is atypical for Windows)
     */

private:
    nsCOMPtr<sbITagLibChannelFileIOManager>
                                mpTagLibChannelFileIOManager;
    nsCOMPtr<nsIFileProtocolHandler>
                                mpFileProtocolHandler;
    nsCOMPtr<nsIResProtocolHandler>
                                mpResourceProtocolHandler;
    nsCOMPtr<sbIMutablePropertyArray> mpMetadataPropertyArray;
    nsCOMPtr<nsIChannel>        mpChannel;
    nsCOMPtr<sbISeekableChannel>
                                mpSeekableChannel;
    nsCOMPtr<nsIURL>            mpURL;
    nsCString                   mMetadataChannelID;
    PRBool                      mMetadataChannelRestart;
    PRBool                      mCompleted;
    nsCString                   mMetadataPath;
    
    
    // When calling Read() keep a cache of any album art 
    // encountered, so that subsequent GetImageData calls
    // do not have to reload the file.
    // We keep the data rather than the Taglib::File 
    // because this lets us avoid the taglib lock.
    
    struct sbAlbumArt {
      PRInt32       type;
      nsCString     mimeType;
      PRUint32      dataLen;
      PRUint8       *data;
      
      sbAlbumArt() : type(0), dataLen(0), data(nsnull) {
        MOZ_COUNT_CTOR(sbAlbumArt);
      };
      ~sbAlbumArt() {
        MOZ_COUNT_DTOR(sbAlbumArt);

        // If nobody has claimed the image by the time
        // this object is destroyed, throw the image away
        if (dataLen > 0 && data) {
          NS_Free(data);
        }
      };
    };
    
    nsTArray<nsAutoPtr<sbAlbumArt> > mCachedAlbumArt;


    // Statics to help manage the single threading of taglib
    static PRLock* sTaglibLock;

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

    nsresult Init();

    static nsresult ModuleConstructor(nsIModule* aSelf);
    static void ModuleDestructor(nsIModule* aSelf);

    /* nsICharsetDetectionObserver */
    NS_IMETHOD Notify(const char* aCharset, nsDetectionConfident aConf);

    /*
     * Private taglib metadata handler class services.
     *
     *   sNextChannelID         Next channel ID to use.
     */

private:
    static PRUint32             sNextChannelID;

private:
    nsresult ReadInternal(
        PRInt32                     *pReadCount);
    nsresult WriteInternal(
        PRInt32                     *pWriteCount);

    nsresult GetImageDataInternal(
        PRInt32                     aType,
        nsACString                  &aMimeType,
        PRUint32                    *aDataLen,
        PRUint8                     **aData);

    nsresult SetImageDataInternal(
        PRInt32                     aType,
        const nsAString             &aURL);

    nsresult RemoveAllImagesMP3(
        TagLib::MPEG::File          *aMPEGFile,
        PRInt32                     imageType);

    nsresult RemoveAllImagesOGG(
        TagLib::Ogg::Vorbis::File   *aMPEGFile,
        PRInt32                     imageType);

    nsresult ReadImageFile(
        const nsAString             &imageSpec,
        PRUint8*                    &imageData,
        PRUint32                    &imageDataSize,
        nsCString                   &imageMimeType);

    nsresult WriteMP3Image(
        TagLib::MPEG::File          *aFile,
        PRInt32                     aType,
        const nsAString             &imageSpec);

    nsresult WriteOGGImage(
        TagLib::Ogg::Vorbis::File   *aFile,
        PRInt32                     aType,
        const nsAString             &imageSpec);

    nsresult WriteMP4Image(
        TagLib::MP4::File           *aFile,
        PRInt32                     aType,
        const nsAString             &imageSpec);

    nsresult ReadImageID3v2(
        TagLib::ID3v2::Tag          *aTag,
        PRInt32                     aType,
        nsACString                  &aMimeType,
        PRUint32                    *aDataLen,
        PRUint8                     **aData);

    nsresult ReadImageITunes(
        TagLib::MP4::Tag            *aTag,
        nsACString                  &aMimeType,
        PRUint32                    *aDataLen,
        PRUint8                     **aData);

    nsresult ReadImageOgg(
        TagLib::Ogg::XiphComment    *aTag,
        PRInt32                     aType,
        nsACString                  &aMimeType,
        PRUint32                    *aDataLen,
        PRUint8                     **aData);

    /*
     * Private taglib metadata handler ID3v2 services.
     */
    void ReadID3v2Tags(
        TagLib::ID3v2::Tag          *pTag,
        const char                  *charset = 0);

    void AddGracenoteMetadataMP3(
        TagLib::MPEG::File          *MPEGFile);

    /*
     * Private taglib metadata handler APE services.
     */

private:
    void ReadAPETags(
        TagLib::APE::Tag          *pTag);

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

    void AddGracenoteMetadataXiph(
        TagLib::Ogg::Vorbis::File          *oggFile);

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
    // please be careful to call these appropriately.
    // don't call release without first doing an acquire
    // and don't try to acquire twice in a thread without releasing.
    nsresult AcquireTaglibLock();
    nsresult ReleaseTaglibLock();
    
    nsresult CheckChannelRestart();

    nsresult ReadMetadata();

    void GuessCharset(
        TagLib::Tag                 *pTag,
        nsACString&                 _retval);

    nsresult RunCharsetDetector(
        nsICharsetDetector          *aDetector,
        TagLib::String              &aContent);

    void ConvertCharset(
        TagLib::String              aString,
        const char                  *aCharset,
        nsAString&                  aResult);

    void CompleteRead();

    PRBool ReadFile(
        TagLib::File                *pTagFile,
        const char                  *aCharset = 0);

    PRBool ReadMPEGFile();

    PRBool ReadASFFile();

    PRBool ReadMP4File();

    PRBool ReadOGGFile();

    PRBool ReadOGAFile();

    PRBool ReadFLACFile();

    PRBool ReadMPCFile();

    nsresult AddMetadataValue(
        const char                  *name,
        TagLib::String              value,
        const char                  *charset);

    nsresult AddMetadataValue(
        const char                  *name,
        bool                        value);

    nsresult AddMetadataValue(
        const char                  *name,
        PRUint64                    value);

    nsresult AddMetadataValue(
        const char                   *name, 
        const nsAString             &value);

    void FixTrackDiscNumber(
        nsString                    numberKey,
        nsString                    totalKey);

private:
    // Functions to write Metadata to tags
    nsresult WriteBasic(
      TagLib::PropertyMap           *properties);
    nsresult WriteSeparatedNumbers(
      TagLib::PropertyMap           *properties);
    nsresult WriteAPE(
        TagLib::APE::Tag            *tag);
    nsresult WriteASF(
        TagLib::ASF::Tag            *tag);
    nsresult WriteID3v1(
        TagLib::ID3v1::Tag          *tag);
    nsresult WriteID3v2(
        TagLib::ID3v2::Tag          *tag);
    nsresult WriteMP4(
        TagLib::MP4::Tag            *tag);
    nsresult WriteXiphComment(
        TagLib::Ogg::XiphComment    *tag);

    /*
     * Private charset detector members
     */

private:
    // these apply only to the last thing being detected
    nsCString mLastCharset;
    nsDetectionConfident mLastConfidence;
    
private:
    // Base64 en-/decoding
    std::string base64_decode(std::string const& encoded_string);
    std::string base64_encode(unsigned char const* bytes_to_encode,
                              unsigned int in_len);
};


#endif /* __METADATA_HANDLER_TAGLIB_H__ */
