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

/*******************************************************************************
 *******************************************************************************
 *
 * Taglib metadata handler.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  MetadataHandlerTaglib.cpp
* \brief Songbird MetadataHandlerTaglib implementation.
*/

/*******************************************************************************
 *
 * Taglib metadata handler imported services.
 *
 ******************************************************************************/

/* Local file imports. */
#include "MetadataHandlerTaglib.h"

/* Local module imports. */
#include "TaglibChannelFileIO.h"

/* Mozilla imports. */
#include <necko/nsIURI.h>
#include <nsComponentManagerUtils.h>
#include <nsIFile.h>
#include <nsIStandardURL.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <unicharutil/nsUnicharUtils.h>

/* Taglib imports. */
#include <fileref.h>
#include <tfile.h>

#include <flacfile.h>
#include <mpcfile.h>
#include <mpegfile.h>
#include <mp4file.h>
#include <vorbisfile.h>

/* C++ std imports. */
#include <sstream>


/*******************************************************************************
 *
 * Taglib metadata handler logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbMetadataHandlerTaglib");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif


/*******************************************************************************
 *
 * Taglib metadata handler nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_ISUPPORTS2(sbMetadataHandlerTaglib, sbIMetadataHandler,
                                            sbISeekableChannelListener)


/*******************************************************************************
 *
 * Taglib metadata handler sbIMetadataHandler implementation.
 *
 ******************************************************************************/

/**
* \brief Vote to be the handler returned for the given url
*
* The sbIMetadataManager will instantiate one of every sbIMetadataHandler
* subclass and ask it to vote on the given url.  Whichever handler returns
* the highest vote will be used as the handler for the url.
*
* Values less than zero cause that handler to be ignored.
*
* At the moment, our handlers return -1, 0, or 1 (for "no," "maybe," and
* "yes").
*
* \param url The url upon which one should vote
* \return The vote
* \sa sbIMetadataManager
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::Vote(
    const nsAString             &url,
    PRInt32                     *pVote)
{
    nsString                    _url(url);
    PRInt32                     vote = 0;

    /* Convert the URL to lower case. */
    ToLowerCase(_url);

    /* Check for supported files. */
    if (   (_url.Find(".flac", PR_TRUE) != -1)
        || (_url.Find(".mov", PR_TRUE) != -1)
        || (_url.Find(".mpc", PR_TRUE) != -1)
        || (_url.Find(".mp3", PR_TRUE) != -1)
        || (_url.Find(".m4a", PR_TRUE) != -1)
        || (_url.Find(".m4p", PR_TRUE) != -1)
        || (_url.Find(".m4v", PR_TRUE) != -1)
        || (_url.Find(".ogg", PR_TRUE) != -1))
    {
        vote = 1;
    }

    /* Check for unsupported files. */
    else if (   (_url.Find(".avi", PR_TRUE) != -1)
             || (_url.Find(".wma", PR_TRUE) != -1)
             || (_url.Find(".wmv", PR_TRUE) != -1)
             || (_url.Find(".asf", PR_TRUE) != -1)
             || (_url.Find(".wav", PR_TRUE) != -1))
    {
        vote = -1;
    }

    /* Return results. */
    *pVote = vote;

    return (NS_OK);
}


/**
* \brief Get the mimetypes supported by this handler (probably DEPRECATED)
* \todo Make sure it's safe to deprecate these.  Only the cores should need to
*       return mimetype/extension info, I think.
* \param pMIMECount The number of mimetypes returned in the array
* \param pMIMETypeList The array of mimetypes supported by this handler
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::SupportedMIMETypes(
    PRUint32                    *pMIMECount,
    PRUnichar                   ***pMIMETypeList)
{
    LOG(("1: SupportedMIMETypes\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
* \brief Get the extensions supported by this handler (probably DEPRECATED)
* \todo Make sure it's safe to deprecate these.  Only the cores should need to
*       return mimetype/extension info, I think.
* \param pExtCount The number of extensions returned in the array
* \param pExtList The array of extensions supported by this handler
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::SupportedFileExtensions(
    PRUint32                    *pExtCount,
    PRUnichar                   ***pExtList)
{
    LOG(("1: SupportedFileExtensions\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
* \brief Start the read operation
*
* After getting a handler from the sbIMetadataManager, the user code usually
* calls read upon it, immediately.  The implementor may choose to handle the
* request immediately or asynchronously.
*
* \return -1 if operating asynchronously, otherwise the number of metadata
*            values read (0 on failure)
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::Read(
    PRInt32                     *pReadCount)
{
    nsCOMPtr<nsIStandardURL>    pStandardURL;
    nsCOMPtr<nsIURI>            pURI;
    nsCOMPtr<nsIFile>           pFile;
    nsCString                   urlSpec;
    nsCString                   urlScheme;
    nsAutoString                filePath;
    PRInt32                     readCount = 0;
    nsresult                    result = NS_OK;

    /* Initialize the metadata values. */
    mpMetadataValues =
                do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1",
                                  &result);

    /* Get the channel URL info. */
    if (NS_SUCCEEDED(result))
        result = mpChannel->GetURI(getter_AddRefs(pURI));
    if (NS_SUCCEEDED(result))
    {
        pStandardURL = do_CreateInstance("@mozilla.org/network/standard-url;1",
                                         &result);
    }
    if (NS_SUCCEEDED(result))
    {
        result = pStandardURL->Init(pStandardURL->URLTYPE_STANDARD,
                                    0,
                                    NS_LITERAL_CSTRING(""),
                                    nsnull,
                                    pURI);
    }
    if (NS_SUCCEEDED(result))
        mpURL = do_QueryInterface(pStandardURL, &result);
    if (NS_SUCCEEDED(result))
        result = mpURL->GetSpec(urlSpec);
    if (NS_SUCCEEDED(result))
        result = mpURL->GetScheme(urlScheme);

    /* If the channel URL scheme is for a local file, try reading     */
    /* synchronously.  If successful, mCompleted will be set to true. */
    if (urlScheme.Equals(NS_LITERAL_CSTRING("file")))
    {
        /* Get the metadata local file path. */
        if (NS_SUCCEEDED(result))
        {
            result = mpFileProtocolHandler->GetFileFromURLSpec
                                                        (urlSpec,
                                                         getter_AddRefs(pFile));
        }
        if (NS_SUCCEEDED(result))
            result = pFile->GetNativePath(mMetadataPath);

        /* Read the metadata. */
        if (NS_SUCCEEDED(result))
            ReadMetadata();
    }

    /* If metadata reading is not complete, start an */
    /* asynchronous read with the metadata channel.  */
    if (NS_SUCCEEDED(result) && !mCompleted)
    {
        /* Create a metadata channel. */
        mpSeekableChannel =
            do_CreateInstance("@songbirdnest.com/Songbird/SeekableChannel;1",
                              &result);

        /* Add the metadata channel for use with the TagLib nsIChannel  */
        /* file I/O services using the metadata path as the channel ID. */
        if (NS_SUCCEEDED(result))
        {
            mMetadataPath = urlSpec;
            mMetadataChannelID = NS_ConvertUTF8toUTF16(mMetadataPath);
            TagLibChannelFileIO::AddChannel(mMetadataChannelID,
                                            mpSeekableChannel);
        }

        /* Open the metadata channel to start reading. */
        if (NS_SUCCEEDED(result))
            mpSeekableChannel->Open(mpChannel, this);

        /* Indicate that reading is being done asynchronously. */
        if (NS_SUCCEEDED(result))
            readCount = -1;
    }

    /* If the read operation is complete, get the number of read tags. */
    if (NS_SUCCEEDED(result) && mCompleted)
        result = mpMetadataValues->GetNumValues(&readCount);

    /* Read operation is complete on error. */
    if (!NS_SUCCEEDED(result))
    {
        mCompleted = PR_TRUE;
        readCount = 0;
    }

    /* Return results. */
    *pReadCount = readCount;

    return (NS_OK);
}


/**
* \brief Start the write operation
*
* After getting a handler from the sbIMetadataManager, the user code may set
* an sbIMetadataValues object into the handler and then call write to write
* the abstract metadata map into the specific metadata requirements of the
* file format supported by the handler.
*
* Note that the number of items written may not always equal the number of
* items in the sbIMetadataValues object if the underlying file format does
* not support the given keys.
*
* \todo Make anything actually work with a write operation
* \return -1 if operating asynchronously, otherwise the number of metadata
*            values written (0 on failure)
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::Write(
    PRInt32                     *pWriteCount)
{
    LOG(("1: Write\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/**
* \brief Be thou informst that one's sbIMetadataChannel has just received data
*
* Every time the underlying nsIChannel dumps data on the sbIMetadataChannel,
* plus once more for when the nsIChannel reports a stop condition.
*
* This is a chance for the handler code to attempt to parse the datastream.
*
* \param pChannel The sbIMetadataChannel delivering data.  You'll have to QI for
*                 it.
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::OnChannelData(
    nsISupports                 *pChannel)
{
    return (NS_OK);
}


/**
* \brief Close down the internals of the handler, stop any downloads, free any
*        allocations
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::Close()
{
    /* If a metadata channel is being used, remove it from */
    /* use with the TagLib nsIChannel file I/O services.   */
    if (mMetadataChannelID.Length() > 0)
        TagLibChannelFileIO::RemoveChannel(mMetadataChannelID);

    /* Close the metadata channel. */
    if (mpSeekableChannel)
    {
        mpSeekableChannel->Close();
        mpSeekableChannel = nsnull;
    }

    /* Read operation is complete. */
    mCompleted = PR_TRUE;

    return (NS_ERROR_NOT_IMPLEMENTED);
}


/*
 * Getters/setters
 */

NS_IMETHODIMP sbMetadataHandlerTaglib::GetValues(
    sbIMetadataValues           **ppMetadataValues)
{
    NS_ENSURE_ARG_POINTER(ppMetadataValues);
    NS_ENSURE_STATE(mpMetadataValues);
    NS_ADDREF(*ppMetadataValues = mpMetadataValues);
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::SetValues(
    sbIMetadataValues           *valueList)
{
    LOG(("1: SetValues\n"));

    return (NS_ERROR_NOT_IMPLEMENTED);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::GetCompleted(
    PRBool                      *pCompleted)
{
    NS_ENSURE_ARG_POINTER(pCompleted);
    *pCompleted = mCompleted;
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::GetChannel(
    nsIChannel                  **ppChannel)
{
    NS_ENSURE_ARG_POINTER(ppChannel);
    NS_IF_ADDREF(*ppChannel = mpChannel);
    return (NS_OK);
}

NS_IMETHODIMP sbMetadataHandlerTaglib::SetChannel(
    nsIChannel                  *pChannel)
{
    mpChannel = pChannel;
    return (NS_OK);
}


/*******************************************************************************
 *
 * Taglib metadata handler sbISeekableChannel implementation.
 *
 ******************************************************************************/

/**
* \brief Be thou informst that one's sbISeekableChannel has just received data
*
* Every time the underlying nsIChannel dumps data on the sbISeekableChannel,
* plus once more for when the nsIChannel reports a stop condition.
*
* This is a chance for the handler code to attempt to parse the datastream.
*
* \param pChannel The sbISeekableChannel delivering data.  You'll have to QI for
*                 it.
*/

NS_IMETHODIMP sbMetadataHandlerTaglib::OnChannelDataAvailable(
    sbISeekableChannel          *pChannel)
{
    PRBool                      channelCompleted;
    nsresult                    result = NS_OK;

    /* Process channel data and complete processing on any exception. */
    try
    {
        /* Clear channel restart condition. */
        mMetadataChannelRestart = PR_FALSE;

        /* Read the metadata. */
        ReadMetadata();

        /* Check for metadata channel completion. */
        if (!mCompleted)
        {
            result = mpSeekableChannel->GetCompleted(&channelCompleted);
            if (NS_SUCCEEDED(result) && channelCompleted)
                mCompleted = PR_TRUE;
        }
    }
    catch(...)
    {
        printf("1: OnChannelDataAvailable exception\n");
        mCompleted = PR_TRUE;
    }

    return (result);
}


/*******************************************************************************
 *
 * Public taglib metadata handler services.
 *
 ******************************************************************************/

/*
 * sbMetadataHandlerTaglib
 *
 *   This method is the constructor for the taglib metadata handler class.
 */

sbMetadataHandlerTaglib::sbMetadataHandlerTaglib()
:
    mCompleted(PR_FALSE),
    mMetadataChannelRestart(PR_FALSE)
{
    nsresult                    result = NS_OK;

    /* Ensure that the class is initialized. */
    result = InitializeClass();

    /* Create a file protocol handler instance. */
    if (NS_SUCCEEDED(result))
    {
        mpFileProtocolHandler =
                do_CreateInstance("@mozilla.org/network/protocol;1?name=file",
                                  &result);
    }

    /* Check for success. */
    NS_ASSERTION(NS_SUCCEEDED(result),
                 "Failed to construct the taglib metadata handler.");
}


/*
 * ~sbMetadataHandlerTaglib
 *
 *   This method is the destructor for the taglib metadata handler class.
 */

sbMetadataHandlerTaglib::~sbMetadataHandlerTaglib()
{
    /* Ensure the taglib metadata handler is closed. */
    Close();
}


/*******************************************************************************
 *
 * Private taglib metadata handler class services.
 *
 ******************************************************************************/

/*
 * Static field initializers.
 */

PRBool sbMetadataHandlerTaglib::mInitialized = PR_FALSE;


/*
 * InitializeClass
 *
 *   This function initializes the sbMetadataHandlerTaglib class.
 */

nsresult sbMetadataHandlerTaglib::InitializeClass()
{
    TagLibChannelFileIOTypeResolver *pResolver;
    nsresult                        result = NS_OK;

    /* Do nothing if already initialized. */
    if (mInitialized)
        return (result);

    /* Add nsIChannel file I/O type resolver. */
    pResolver = new TagLibChannelFileIOTypeResolver();
    if (pResolver)
        File::addFileIOTypeResolver(pResolver);
    else
        result = NS_ERROR_UNEXPECTED;

    /* Class is now initialized. */
    if (NS_SUCCEEDED(result))
        mInitialized = PR_TRUE;

    return (result);
}


/*******************************************************************************
 *
 * Private taglib metadata handler ID3v2 services.
 *
 ******************************************************************************/

/*
 * ID3v2Map                     Map of ID3v2 tag names to Songbird metadata
 *                              names.
 */

static char *ID3v2Map[][2] =
{
    { "COMM", "comment" },
    { "TALB", "album" },
    { "TBPM", "bpm" },
    { "TCOM", "composer" },
    { "TCOP", "copyright_message" },
    { "TENC", "vendor" },
    { "TIME", "length" },
    { "TIT2", "title" },
    { "TIT3", "subtitle" },
    { "TKEY", "key" },
    { "TLAN", "language" },
    { "TLEN", "length" },
    { "TMED", "mediatype" },
    { "TOAL", "original_album" },
    { "TOPE", "original_artist" },
    { "TORY", "original_year" },
    { "TPE1", "artist" },
    { "TPE2", "accompaniment" },
    { "TPE3", "conductor" },
    { "TPE4", "interpreter_remixer" },
    { "TPOS", "set_collection" },
    { "TPUB", "publisher" },
    { "TRCK", "track_no" },
    { "TYER", "year" },
    { "UFID", "metadata_uuid" },
    { "USER", "terms_of_use" },
    { "USLT", "lyrics" },
    { "WCOM", "commercialinfo_url" },
    { "WCOP", "copyright_url" },
    { "WOAR", "artist_url" },
    { "WOAS", "source_url" },
    { "WORS", "netradio_url" },
    { "WPAY", "payment_url" },
    { "WPUB", "publisher_url" },
    { "WXXX", "user_url" }
};


/*
 * ReadID3v2Tags
 *
 *   --> pTag                   ID3v2 tag object.
 *
 *   This function reads ID3v2 tags from the ID3v2 tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadID3v2Tags(
    TagLib::ID3v2::Tag          *pTag)
{
    TagLib::ID3v2::FrameListMap frameListMap;
    int                         numMetadataEntries;
    int                         i;

    /* Do nothing if no tags are present. */
    if (!pTag)
        return;

    /* Get the frame list map. */
    frameListMap = pTag->frameListMap();

    /* Add the metadata entries. */
    numMetadataEntries = sizeof(ID3v2Map) / sizeof(ID3v2Map[0]);
    for (i = 0; i < numMetadataEntries; i++)
        AddID3v2Tag(frameListMap, ID3v2Map[i][0], ID3v2Map[i][1]);
}


/*
 * AddID3v2Tag
 *
 *   --> frameListMap           Frame list map.
 *   --> frameID                ID3v2 frame ID.
 *   --> metadataName           Metadata name.
 *
 *   This function adds the ID3v2 tag with the ID3v2 frame ID specified by
 * frameID from the frame list map specified by frameListMap to the set of
 * metadata values with the metadata name specified by metadataName.
 */

void sbMetadataHandlerTaglib::AddID3v2Tag(
    TagLib::ID3v2::FrameListMap &frameListMap,
    const char                  *frameID,
    const char                  *metadataName)
{
    TagLib::ID3v2::FrameList    frameList;

    /* Add the frame metadata. */
    frameList = frameListMap[frameID];
    if (!frameList.isEmpty())
        AddMetadataValue(metadataName, frameList.front()->toString());
}


/*******************************************************************************
 *
 * Private taglib metadata handler Xiph services.
 *
 ******************************************************************************/

/*
 * XiphMap                      Map of Xiph tag names to Songbird metadata
 *                              names.
 */

static char *XiphMap[][2] =
{
    { "TRACKNUMBER", "track_no" },
    { "DATE", "year" }
};


/*
 * ReadXiphTags
 *
 *   --> pTag                   Xiph tag object.
 *
 *   This function reads Xiph tags from the Xiph tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadXiphTags(
    TagLib::Ogg::XiphComment    *pTag)
{
    TagLib::Ogg::FieldListMap   fieldListMap;
    int                         numMetadataEntries;
    int                         i;

    /* Do nothing if no tags are present. */
    if (!pTag)
        return;

    fieldListMap = pTag->fieldListMap();

    /* Add the metadata entries. */
    numMetadataEntries = sizeof(XiphMap) / sizeof(XiphMap[0]);
    for (i = 0; i < numMetadataEntries; i++)
        AddXiphTag(fieldListMap, XiphMap[i][0], XiphMap[i][1]);
}


/*
 * AddXiphTag
 *
 *   --> fieldListMap           Field list map.
 *   --> fieldName              Xiph field name.
 *   --> metadataName           Metadata name.
 *
 *   This function adds the Xiph tag with the Xiph field name specified by
 * fieldName from the field list map specified by fieldListMap to the set of
 * metadata values with the metadata name specified by metadataName.
 */

void sbMetadataHandlerTaglib::AddXiphTag(
    TagLib::Ogg::FieldListMap   &fieldListMap,
    const char                  *fieldName,
    const char                  *metadataName)
{
    TagLib::StringList          fieldList;

    /* Add the field metadata. */
    fieldList = fieldListMap[fieldName];
    if (!fieldList.isEmpty())
        AddMetadataValue(metadataName, fieldList.front());
}


/*******************************************************************************
 *
 * Private taglib metadata handler mp4 services.
 *
 ******************************************************************************/

/*
 * ReadMP4Tags
 *
 *   --> pTag                   MP4 tag object.
 *
 *   This function reads MP4 tags from the MP4 tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 *   This function only reads the MP4 specific tags.
 */

void sbMetadataHandlerTaglib::ReadMP4Tags(
    TagLib::MP4::Tag            *pTag)
{
    /* Add MP4 specific tags. */
    if (!pTag->composer().isEmpty())
        AddMetadataValue("composer", pTag->composer());
    if (pTag->numTracks())
        AddMetadataValue("track_total", pTag->numTracks());
    if (pTag->disk())
        AddMetadataValue("disc_no", pTag->disk());
    if (pTag->numDisks())
        AddMetadataValue("disc_total", pTag->numDisks());
    if (pTag->bpm())
        AddMetadataValue("bpm", pTag->bpm());
}


/*******************************************************************************
 *
 * Private taglib metadata handler services.
 *
 ******************************************************************************/

/*
 * ReadMetadata
 *
 *   This function attempts to read the metadata.  If the metadata is available,
 * this function extracts the metadata values and sets the metadata reading
 * complete condition to true.
 */

nsresult sbMetadataHandlerTaglib::ReadMetadata()
{
    const char                  *metadataPathCStr;
    nsCString                   fileExt;
    PRBool                      isValid = PR_FALSE;
    PRBool                      decodedFileExt = PR_FALSE;
    nsresult                    result = NS_OK;

    /* Get the metadata local file path. */
    metadataPathCStr = mMetadataPath.get();

    /* Get the metadata file extension. */
    result = mpURL->GetFileExtension(fileExt);
    if (NS_SUCCEEDED(result))
        ToLowerCase(fileExt);

    /* Read the metadata using the file extension */
    /* to determine the metadata format.          */
    if (NS_SUCCEEDED(result))
    {
        decodedFileExt = PR_TRUE;
        if (fileExt.Equals(NS_LITERAL_CSTRING("flac")))
            isValid = ReadFLACFile(metadataPathCStr);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("mpc")))
            isValid = ReadMPCFile(metadataPathCStr);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("mp3")))
            isValid = ReadMPEGFile(metadataPathCStr);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("m4a")))
            isValid = ReadMP4File(metadataPathCStr);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("m4p")))
            isValid = ReadMP4File(metadataPathCStr);
        else if (fileExt.Equals(NS_LITERAL_CSTRING("ogg")))
            isValid = ReadOGGFile(metadataPathCStr);
        else
            decodedFileExt = PR_FALSE;
    }

    /* If the file extension was not decoded, try */
    /* reading the metadata in different formats. */
    if (   NS_SUCCEEDED(result)
        && !decodedFileExt
        && !isValid
        && !mMetadataChannelRestart)
    {
        isValid = ReadMPEGFile(metadataPathCStr);
    }

    /* Fix up track and disc number metadata. */
    if (isValid && !mMetadataChannelRestart)
    {
        FixTrackDiscNumber(NS_LITERAL_STRING("track_no"),
                           NS_LITERAL_STRING("track_total"));
        FixTrackDiscNumber(NS_LITERAL_STRING("disc_no"),
                           NS_LITERAL_STRING("disc_total"));
    }

    /* Check if the metadata reading is complete. */
    if (isValid && !mMetadataChannelRestart)
        mCompleted = PR_TRUE;

    /* Metadata reading is complete upon error. */
    if (!NS_SUCCEEDED(result))
        mCompleted = PR_TRUE;

    return (result);
}


/*
 * ReadFile
 *
 *   --> pTagFile               File from which to read.
 *
 *   <--                        True if file has valid metadata.
 *
 *   This function reads a base set of metadata from the file specified by
 * pTagFile.
 */

PRBool sbMetadataHandlerTaglib::ReadFile(
    TagLib::File                *pTagFile)
{
    TagLib::Tag                 *pTag;
    TagLib::AudioProperties     *pAudioProperties;
    PRBool                      isValid = PR_TRUE;

    /* Check for NULL file. */
    if (!pTagFile)
        isValid = PR_FALSE;

    /* Check if the file metadata is valid. */
    if (isValid)
    {
        if (!pTagFile->isValid())
            isValid = PR_FALSE;
    }

    /* Get the tag info. */
    if (isValid)
    {
        pTag = pTagFile->tag();
        AddMetadataValue("album", pTag->album());
        AddMetadataValue("artist", pTag->artist());
        AddMetadataValue("comment", pTag->comment());
        AddMetadataValue("genre", pTag->genre());
        AddMetadataValue("title", pTag->title());
        AddMetadataValue("track_no", pTag->track());
        AddMetadataValue("year", pTag->year());
    }

    /* Get the audio properties. */
    if (isValid)
    {
        pAudioProperties = pTagFile->audioProperties();
        if (pAudioProperties)
        {
            AddMetadataValue("bitrate", pAudioProperties->bitrate());
            AddMetadataValue("frequency", pAudioProperties->sampleRate());
            AddMetadataValue("length", pAudioProperties->length() * 1000);
        }
    }

    return (isValid);
}


/*
 * ReadFLACFile
 *
 *   --> filePath               Path to FLAC file.
 *
 *   <--                        True if file has valid FLAC metadata.
 *
 *   This function reads metadata from the FLAC file with the file path
 * specified by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadFLACFile(
    const char                  *filePath)
{
    TagLib::FLAC::File          *pTagFile = NULL;
    PRBool                      isValid = PR_TRUE;

    /* Open the metadata file. */
    pTagFile = new TagLib::FLAC::File(filePath);

    /* Check for channel restart. */
    mMetadataChannelRestart =
                    TagLibChannelFileIO::GetChannelRestart(mMetadataChannelID);
    if (mMetadataChannelRestart)
        isValid = PR_FALSE;

    /* Read the base file metadata. */
    if (isValid)
        isValid = ReadFile(pTagFile);

    /* Read the ID3v2 metadata. */
    if (isValid)
        ReadID3v2Tags(pTagFile->ID3v2Tag());

    /* Clean up. */
    if (pTagFile)
        delete(pTagFile);

    return (isValid);
}


/*
 * ReadMPCFile
 *
 *   --> filePath               Path to MPC file.
 *
 *   <--                        True if file has valid MPC metadata.
 *
 *   This function reads metadata from the MPC file with the file path specified
 * by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadMPCFile(
    const char                  *filePath)
{
    TagLib::MPC::File           *pTagFile = NULL;
    PRBool                      isValid = PR_TRUE;

    /* Open the metadata file. */
    pTagFile = new TagLib::MPC::File(filePath);

    /* Check for channel restart. */
    mMetadataChannelRestart =
                    TagLibChannelFileIO::GetChannelRestart(mMetadataChannelID);
    if (mMetadataChannelRestart)
        isValid = PR_FALSE;

    /* Read the base file metadata. */
    if (isValid)
        isValid = ReadFile(pTagFile);

    /* Clean up. */
    if (pTagFile)
        delete(pTagFile);

    return (isValid);
}


/*
 * ReadMPEGFile
 *
 *   --> filePath               Path to MPEG file.
 *
 *   <--                        True if file has valid MPEG metadata.
 *
 *   This function reads metadata from the MPEG file with the file path
 * specified by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadMPEGFile(
    const char                  *filePath)
{
    TagLib::MPEG::File          *pTagFile = NULL;
    PRBool                      isValid = PR_TRUE;

    /* Open the metadata file. */
    pTagFile = new TagLib::MPEG::File(filePath);

    /* Check for channel restart. */
    mMetadataChannelRestart =
                    TagLibChannelFileIO::GetChannelRestart(mMetadataChannelID);
    if (mMetadataChannelRestart)
        isValid = PR_FALSE;

    /* Read the base file metadata. */
    if (isValid)
        isValid = ReadFile(pTagFile);

    /* Read the ID3v2 metadata. */
    if (isValid)
        ReadID3v2Tags(pTagFile->ID3v2Tag());

    /* Clean up. */
    if (pTagFile)
        delete(pTagFile);

    return (isValid);
}


/*
 * ReadMP4File
 *
 *   --> filePath               Path to MP4 file.
 *
 *   <--                        True if file has valid MP4 metadata.
 *
 *   This function reads metadata from the MP4 file with the file path specified
 * by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadMP4File(
    const char                  *filePath)
{
    TagLib::MP4::File           *pTagFile = NULL;
    PRBool                      isValid = PR_TRUE;

    /* Open the metadata file. */
    pTagFile = new TagLib::MP4::File(filePath);

    /* Check for channel restart. */
    mMetadataChannelRestart =
                    TagLibChannelFileIO::GetChannelRestart(mMetadataChannelID);
    if (mMetadataChannelRestart)
        isValid = PR_FALSE;

    /* Read the base file metadata. */
    if (isValid)
        isValid = ReadFile(pTagFile);

    /* Read the MP4 specific metadata. */
    if (isValid)
        ReadMP4Tags(static_cast<TagLib::MP4::Tag *>(pTagFile->tag()));

    /* Clean up. */
    if (pTagFile)
        delete(pTagFile);

    return (isValid);
}


/*
 * ReadOGGFile
 *
 *   --> filePath               Path to OGG file.
 *
 *   <--                        True if file has valid OGG metadata.
 *
 *   This function reads metadata from the OGG file with the file path specified
 * by filePath.
 */

PRBool sbMetadataHandlerTaglib::ReadOGGFile(
    const char                  *filePath)
{
    TagLib::Vorbis::File        *pTagFile = NULL;
    PRBool                      isValid = PR_TRUE;

    /* Open the metadata file. */
    pTagFile = new TagLib::Vorbis::File(filePath);

    /* Check for channel restart. */
    mMetadataChannelRestart =
                    TagLibChannelFileIO::GetChannelRestart(mMetadataChannelID);
    if (mMetadataChannelRestart)
        isValid = PR_FALSE;

    /* Read the base file metadata. */
    if (isValid)
        isValid = ReadFile(pTagFile);

    /* Read the Xiph metadata. */
    if (isValid)
        ReadXiphTags(pTagFile->tag());

    /* Clean up. */
    if (pTagFile)
        delete(pTagFile);

    return (isValid);
}


/*
 * AddMetadataValue
 *
 *   --> name                   Metadata name.
 *   --> value                  Metadata value.
 *
 *   This function adds the metadata value specified by value with the name
 * specified by name to the set of metadata values.
 */

nsresult sbMetadataHandlerTaglib::AddMetadataValue(
    const char                  *name,
    TagLib::String              value)
{
    nsresult                    result = NS_OK;

    /* Do nothing if no metadata value available. */
    if (value == TagLib::String::null)
        return (result);

    /* Add the metadata value. */
    result = mpMetadataValues->SetValue
                                (NS_ConvertUTF8toUTF16(name),
                                 NS_ConvertUTF8toUTF16(value.toCString(true)),
                                 0);

    return (result);
}


/*
 * AddMetadataValue
 *
 *   --> name                   Metadata name.
 *   --> value                  Metadata value.
 *
 *   This function adds the metadata value specified by value with the name
 * specified by name to the set of metadata values.
 */

nsresult sbMetadataHandlerTaglib::AddMetadataValue(
    const char                  *name,
    TagLib::uint                value)
{
    nsAutoString                valueString;
    nsresult                    result = NS_OK;

    /* Convert the integer value into a string. */
    valueString.AppendInt(value);

    /* Add the metadata value. */
    result = mpMetadataValues->SetValue(NS_ConvertUTF8toUTF16(name),
                                        valueString,
                                        0);

    return (result);
}


/*
 * FixTrackDiscNumber
 *
 *   --> numberKey              Number metadata key.
 *   --> totalKey               Total metadata key.
 *
 *   This function fixes up track and disc number metadata values that contain
 * both the number and total values (e.g., "1 of 12" or "1/12").  The track or
 * disc number key is specified by numberKey and the total key is specified by
 * totalKey.
 */

void sbMetadataHandlerTaglib::FixTrackDiscNumber(
    nsString                    numberKey,
    nsString                    totalKey)
{
    nsAutoString                numberValue;
    nsAutoString                numberField;
    nsAutoString                totalField;
    int                         numberInt;
    int                         totalInt;
    PRInt32                     mark;
    nsresult                    result = NS_OK;

    /* Get the number value. */
    result = mpMetadataValues->GetValue(numberKey, numberValue);

    /* Search for "of" or "/". */
    if ((mark = numberValue.Find("of", PR_TRUE)) == -1)
        mark = numberValue.Find("/", PR_TRUE);

    /* If number value contains both the number and the total, split it up. */
    if (mark != -1)
    {
        /* Get the number and total fields. */
        numberField = StringHead(numberValue, mark);
        totalField = StringTail(numberValue, numberValue.Length() - mark - 1);

        /* Convert the fields to integers. */
        numberInt = atoi(NS_ConvertUTF16toUTF8(numberField).get());
        totalInt = atoi(NS_ConvertUTF16toUTF8(totalField).get());

        /* Add the number and total metadata values. */
        AddMetadataValue(NS_ConvertUTF16toUTF8(numberKey).get(),
                         numberInt);
        AddMetadataValue(NS_ConvertUTF16toUTF8(totalKey).get(),
                         totalInt);
    }
}


