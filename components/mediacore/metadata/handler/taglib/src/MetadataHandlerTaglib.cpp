// vim: indent=4
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
#include <nsICharsetDetector.h>
#include <nsIUTF8ConverterService.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <prprf.h>
#include <unicharutil/nsUnicharUtils.h>
#include <nsMemory.h>

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

/* Windows Specific */
#if defined(XP_WIN)
  #include <windows.h>
#endif

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


// the minimum number of chracters to feed into the charset detector
#define GUESS_CHARSET_MIN_CHAR_COUNT 256

/*******************************************************************************
 *
 * Taglib metadata handler nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_ISUPPORTS3(sbMetadataHandlerTaglib, sbIMetadataHandler,
                                            sbISeekableChannelListener,
                                            nsICharsetDetectionObserver)


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

    /* Get the TagLib sbISeekableChannel file IO manager. */
    mpTagLibChannelFileIOManager =
            do_GetService
                ("@songbirdnest.com/Songbird/sbTagLibChannelFileIOManager;1",
                 &result);

    /* Initialize the metadata values. */
    if (NS_SUCCEEDED(result))
    {
        mpMetadataValues =
                do_CreateInstance("@songbirdnest.com/Songbird/MetadataValues;1",
                                  &result);
    }

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

#ifdef MOZ_CRASHREPORTER
    /* Add crash reporter annotation. */
    if (NS_SUCCEEDED(result))
    {
        mpCrashReporter = do_GetService("@mozilla.org/xre/app-info;1", &result);
        if (NS_FAILED(result))
        {
            mpCrashReporter = nsnull;
            result = NS_OK;
        }
        if (mpCrashReporter)
        {
            mpCrashReporter->AnnotateCrashReport
                                        (NS_LITERAL_CSTRING("TaglibReadSpec"),
                                         urlSpec);
        }
    }
#endif

    /* If the channel URL scheme is for a local file, try reading  */
    /* synchronously.  If successful, metadata read operation will */
    /* be completed.                                               */
    if (urlScheme.Equals(NS_LITERAL_CSTRING("file")))
    {
        /* Get the metadata local file path. */
        if (NS_SUCCEEDED(result))
        {
            result = mpFileProtocolHandler->GetFileFromURLSpec
                                                        (urlSpec,
                                                         getter_AddRefs(pFile));
        }
        
        if (NS_SUCCEEDED(result)) {
          
          nsAutoString path;
          result = pFile->GetPath(path);

          mMetadataPath = NS_ConvertUTF16toUTF8(path);
        }

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

        /* Add the metadata channel for use with the */
        /* TagLib nsIChannel  file I/O services.     */
        if (NS_SUCCEEDED(result))
        {
            /* Allocate a metadata channel ID    */
            /* and use it for the metadata path. */
            PR_AtomicIncrement((PRInt32 *) &sNextChannelID);
            mMetadataPath.AssignLiteral("metadata_channel://");
            mMetadataPath.AppendInt(sNextChannelID);
            mMetadataChannelID = NS_ConvertUTF8toUTF16(mMetadataPath);

            /* Add the metadata channel. */
            result = mpTagLibChannelFileIOManager->AddChannel
                                                            (mMetadataChannelID,
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

    /* Complete read operation on error. */
    if (!NS_SUCCEEDED(result))
    {
        CompleteRead();
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
    if (!mMetadataChannelID.IsEmpty())
    {
        mpTagLibChannelFileIOManager->RemoveChannel(mMetadataChannelID);
        mMetadataChannelID.Truncate();
    }

    /* Close the metadata channel. */
    if (mpSeekableChannel)
    {
        mpSeekableChannel->Close();
        mpSeekableChannel = nsnull;
    }

    /* Complete read operation. */
    CompleteRead();

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

    /* Do nothing if the metadata reading is complete. */
    if (mCompleted)
        return (result);

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
                CompleteRead();
        }
    }
    catch(...)
    {
        printf("1: OnChannelDataAvailable exception\n");
        CompleteRead();
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
    mMetadataChannelRestart(PR_FALSE),
    mCompleted(PR_FALSE)
{
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


/*
 * FactoryInit
 *
 *   This function is called by the component factory to initialize the
 * component.
 */

nsresult sbMetadataHandlerTaglib::FactoryInit()
{
    nsresult                    rv;

    /* Create a file protocol handler instance. */
    mpFileProtocolHandler =
            do_CreateInstance("@mozilla.org/network/protocol;1?name=file", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    return (NS_OK);
}


/*******************************************************************************
 *
 * Private taglib metadata handler class services.
 *
 ******************************************************************************/

/*
 * Static field initializers.
 */

PRUint32 sbMetadataHandlerTaglib::sNextChannelID = 0;


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
 *   --> aCharset               The character set the tags are assumed to be in
 *                              Pass in null to do no conversion
 *
 *   This function reads ID3v2 tags from the ID3v2 tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadID3v2Tags(
    TagLib::ID3v2::Tag          *pTag,
    const char                  *aCharset)
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
        AddID3v2Tag(frameListMap, ID3v2Map[i][0], ID3v2Map[i][1], aCharset);
}


/*
 * AddID3v2Tag
 *
 *   --> frameListMap           Frame list map.
 *   --> frameID                ID3v2 frame ID.
 *   --> metadataName           Metadata name.
 *   --> charset                The character set the tags are assumed to be in
 *                              Pass in null to do no conversion
 *
 *   This function adds the ID3v2 tag with the ID3v2 frame ID specified by
 * frameID from the frame list map specified by frameListMap to the set of
 * metadata values with the metadata name specified by metadataName.
 */

void sbMetadataHandlerTaglib::AddID3v2Tag(
    TagLib::ID3v2::FrameListMap &frameListMap,
    const char                  *frameID,
    const char                  *metadataName,
    const char                  *charset)
{
    TagLib::ID3v2::FrameList    frameList;

    /* Add the frame metadata. */
    frameList = frameListMap[frameID];
    if (!frameList.isEmpty())
    {
        /* Convert length to microseconds. */
        if (!strcmp(metadataName, "length"))
        {
            PRInt32                     length;

            PR_sscanf(frameList.front()->toString().toCString(true),
                      "%d",
                      &length);
            length *= 1000;
            AddMetadataValue(metadataName, length);
            return;
        }

        /* Just copy all other metadata. */
        TagLib::String value = ConvertCharset(frameList.front()->toString(),
                                              charset);
        AddMetadataValue(metadataName, value);
    }
}


/*******************************************************************************
 *
 * Private taglib metadata handler APE services.
 *
 ******************************************************************************/

/*
 * APEMap                     Map of APE tag names to Songbird metadata
 *                            names.
 */

// see http://wiki.hydrogenaudio.org/index.php?title=APE_key
static char *APEMap[][2] =
{
    { "Title", "trackName" },
    { "Subtitle", "subtitle" },
    { "Artist", "artistName" },
    { "Album", "albumName" },
    { "Debut album", "original_album" },
    { "Publisher", "recordLabelName" },
    { "Conductor", "conductor" },
    { "Track", "trackNumber" },
    { "Composer", "composerName" },
    { "Comment", "comment" },
    { "Copyright", "copyright_message" },
    { "File", "source_url" },
    { "Year", "year" },
    { "Genre", "genre" }
};


/*
 * ReadAPETags
 *
 *   --> pTag                   APE tag object.
 *
 *   This function reads APE tags from the APE tag object specified by pTag.
 * The read tags are added to the set of metadata values.
 */

void sbMetadataHandlerTaglib::ReadAPETags(
    TagLib::APE::Tag          *pTag)
{
    TagLib::APE::ItemListMap  itemListMap;
    int                       numMetadataEntries;
    int                       i;

    /* Do nothing if no tags are present. */
    if (!pTag) {
        return;
    }

    /* Get the item list map. */
    itemListMap = pTag->itemListMap();

    /* Add the metadata entries. */
    numMetadataEntries = sizeof(APEMap) / sizeof(APEMap[0]);
    for (i = 0; i < numMetadataEntries; i++)
        AddAPETag(itemListMap, APEMap[i][0], APEMap[i][1]);
}


/*
 * AddAPETag
 *
 *   --> itemListMap            Item list map.
 *   --> itemID                 APE item ID.
 *   --> metadataName           Metadata name.
 *
 *   This function adds the APE tag with the APE frame ID specified by
 * frameID from the frame list map specified by frameListMap to the set of
 * metadata values with the metadata name specified by metadataName.
 */

void sbMetadataHandlerTaglib::AddAPETag(
    TagLib::APE::ItemListMap    &itemListMap,
    const char                  *itemID,
    const char                  *metadataName)
{
    TagLib::APE::Item item;

    /* Add the frame metadata. */
    item = itemListMap[itemID];
    if (!item.isEmpty())
    {
        /* Convert length to microseconds. */
        if (!strcmp(metadataName, "length"))
        {
            PRInt32                     length;

            PR_sscanf(item.toString().toCString(true),
                      "%d",
                      &length);
            length *= 1000;
            AddMetadataValue(metadataName, length);
            return;
        }

        /* Just copy all other metadata. */
        AddMetadataValue(metadataName, item.toString());
    }
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
        CompleteRead();

    /* Complete metadata reading upon error. */
    if (!NS_SUCCEEDED(result))
        CompleteRead();

    return (result);
}


/*
 * CompleteRead
 *
 *   This function completes a metadata read operation.
 */

void sbMetadataHandlerTaglib::CompleteRead()
{
    /* Indicate that the metadata read operation has completed. */
    mCompleted = PR_TRUE;

#ifdef MOZ_CRASHREPORTER
    /* Remove crash reporter annotation. */
    if (mpCrashReporter)
    {
        mpCrashReporter->AnnotateCrashReport
                                        (NS_LITERAL_CSTRING("TaglibReadSpec"),
                                         NS_LITERAL_CSTRING(""));
    }
#endif
}


/*
 * ReadFile
 *
 *   --> pTagFile               File from which to read.
 *   --> aCharset               The character encoding to use for reading
 *                              May be empty string (to mean no conversion)
 *                              On Windows, may also be "CP_ACP"
 *
 *   <--                        True if file has valid metadata.
 *
 *   This function reads a base set of metadata from the file specified by
 * pTagFile.
 */

PRBool sbMetadataHandlerTaglib::ReadFile(
    TagLib::File                *pTagFile,
    const char                  *aCharset)
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
        // yay random charset guessing!
        AddMetadataValue("album", ConvertCharset(pTag->album(), aCharset));
        AddMetadataValue("artist", ConvertCharset(pTag->artist(), aCharset));
        AddMetadataValue("comment", ConvertCharset(pTag->comment(), aCharset));
        AddMetadataValue("genre", ConvertCharset(pTag->genre(), aCharset));
        AddMetadataValue("title", ConvertCharset(pTag->title(), aCharset));
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
            AddMetadataValue("length", pAudioProperties->length() * 1000000);
        }
    }

    return (isValid);
}


/*
 * GuessCharset
 *
 *   --> pTag                   The tags to look at
 *
 *   <--                        The best guess at the encoding the tags are in
 *
 *   This function looks at a a set of tags and tries to guess what charset 
 *   (encoding) the tags are in.  Returns empty string to mean don't convert
 *   (valid Unicode), and the literal "CP_ACP" if on Windows and best guess is
 *   whatever the system locale is.
 */

nsCString sbMetadataHandlerTaglib::GuessCharset(
    TagLib::Tag*                pTag)
{
    nsresult rv = NS_ERROR_FAILURE;
    
    // first, build a string consisting of some tags
    TagLib::String tagString;
    tagString += pTag->album();
    tagString += pTag->artist();
    tagString += pTag->title();
    // comment and genre can end up confusing the detection; ignore them
    //tagString += pTag->comment();
    //tagString += pTag->genre();
    
    // first, figure out if this was from unicode - we do this by scanning
    // the string for things that had more than 8 bits
    TagLib::String::ConstIterator it;
    for (it = tagString.begin(); it != tagString.end(); ++it) {
        if (*it & ~0xFF) {
            // this was Unicode; don't do any more guessing
            return EmptyCString();
        }
    }

    // look at the raw bytes
    const char* data = tagString.toCString(false); // raw data
    nsCString dataUTF8;
    
    // see if it's valid utf8; if yes, assume it _is_ indeed utf8
    nsCOMPtr<nsIUTF8ConverterService> utf8Service =
        do_GetService("@mozilla.org/intl/utf8converterservice;1");
    if (utf8Service) {
        rv = utf8Service->ConvertStringToUTF8(nsDependentCString(data, tagString.size()),
                                              "utf-8",
                                              PR_FALSE,
                                              dataUTF8);
        if (NS_SUCCEEDED(rv)) {
            // this was utf8
            return NS_LITERAL_CSTRING("utf-8");
        }
    }

    // the metadata is in some 8-bit encoding; try to guess
    mLastConfidence = eNoAnswerYet;
    nsCOMPtr<nsICharsetDetector> detector = do_CreateInstance(
        NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector");
    if (detector) {
        nsCOMPtr<nsICharsetDetectionObserver> observer =
            do_QueryInterface( NS_ISUPPORTS_CAST(nsICharsetDetectionObserver*, this) );
        rv = detector->Init(observer);
    }
    if (detector && NS_SUCCEEDED(rv)) {
        PRBool isDone = PR_FALSE;
        // artificially inflate the buffer by repeating it a lot; this does
        // in fact help with the detection
        const PRUint32 chunkSize = tagString.size();
        PRUint32 currentSize = 0;
        while (currentSize < GUESS_CHARSET_MIN_CHAR_COUNT) {
            rv = detector->DoIt(data, chunkSize, &isDone);
            if (NS_FAILED(rv) || isDone) {
                break;
            }
            currentSize += chunkSize;
        }
        if (NS_SUCCEEDED(rv)) {
            rv = detector->Done();
        }
        if (NS_SUCCEEDED(rv)) {
            if (eSureAnswer == mLastConfidence || eBestAnswer == mLastConfidence) {
                return nsDependentCString(mLastCharset); // copied on return
            }
        }
    }

#if XP_WIN
    // we have no idea what charset this is, but we know it's bad.
    // for Windows only, assume CP_ACP
    int len = tagString.size();

    // make the call fail if it's not valid CP_ACP
    int size = MultiByteToWideChar( CP_ACP, MB_ERR_INVALID_CHARS, data, len, nsnull, 0 );
    if (size) {
        // okay, so CP_ACP is usable
        return NS_LITERAL_CSTRING("CP_ACP");
    }
#endif

    // we truely know nothing
    return EmptyCString();
}

/* nsICharsetDetectionObserver */
NS_IMETHODIMP sbMetadataHandlerTaglib::Notify(
    const char                  *aCharset,
    nsDetectionConfident        aConf)
{
    unsigned int len = strlen(aCharset);
    if (len > sizeof(mLastCharset) - 1) {
        len = sizeof(mLastCharset) - 1;
    }
    memcpy(mLastCharset, aCharset, len);
    mLastCharset[len] = '\0';
    mLastConfidence = aConf;
    return NS_OK;
}

TagLib::String sbMetadataHandlerTaglib::ConvertCharset(
    TagLib::String              aString,
    const char                  *aCharset)
{
    if (!aCharset || !*aCharset) {
        // no conversion
        return aString;
    }

    const char* data = aString.toCString(false);
#if XP_WIN
    if (NS_LITERAL_CSTRING("CP_ACP").Equals(aCharset)) {
        // convert to CP_ACP
        int size = ::MultiByteToWideChar( CP_ACP,
                                          MB_ERR_INVALID_CHARS,
                                          data,
                                          aString.size(),
                                          nsnull,
                                          0 );
        if (size) {
            // convert to CP_ACP
            PRUnichar *wstr = reinterpret_cast< PRUnichar * >( NS_Alloc( (size + 1) * sizeof( PRUnichar ) ) );
            if (wstr) {
                int read = MultiByteToWideChar( CP_ACP,
                                                MB_ERR_INVALID_CHARS,
                                                data,
                                                aString.size(),
                                                wstr,
                                                size );
                NS_ASSERTION(size == read, "Win32 Current Codepage conversion failed.");
                
                wstr[size] = '\0';
                
                TagLib::String strValue(wstr);
                NS_Free(wstr);
                return strValue;
            }
        }
    }
#endif

    // convert via Mozilla
    nsCOMPtr<nsIUTF8ConverterService> utf8Service =
        do_GetService("@mozilla.org/intl/utf8converterservice;1");
    if (utf8Service) {
        nsDependentCString raw(data, aString.size());
        nsCString converted;
        nsresult rv = utf8Service->ConvertStringToUTF8(
            raw, aCharset, PR_FALSE, converted);
        if (NS_SUCCEEDED(rv))
            return TagLib::String(converted.BeginReading(), TagLib::String::UTF8);
    }

    // failed to convert :(
    return aString;
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
    nsAutoPtr<TagLib::FLAC::File>   pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Open and read the metadata file. */
    pTagFile = new TagLib::FLAC::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath);
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }
    
    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the ID3v2 metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadXiphTags(pTagFile->xiphComment());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

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
    nsAutoPtr<TagLib::MPC::File>    pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Open and read the metadata file. */
    pTagFile = new TagLib::MPC::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath);
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the APE metadata */
    if (NS_SUCCEEDED(result) && isValid)
        ReadAPETags(pTagFile->APETag());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

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
    nsAutoPtr<TagLib::MPEG::File>   pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Open and read the metadata file. */
    pTagFile = new TagLib::MPEG::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath);
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Guess the charset */
    nsCString charset = GuessCharset(pTagFile->tag());

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile, charset.BeginReading());

    /* Read the ID3v2 metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadID3v2Tags(pTagFile->ID3v2Tag(), charset.BeginReading());

    /* Read the APE metadata */
    if (NS_SUCCEEDED(result) && isValid)
        ReadAPETags(pTagFile->APETag());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

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
    nsAutoPtr<TagLib::MP4::File>    pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Open and read the metadata file. */
    pTagFile = new TagLib::MP4::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath);
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the MP4 specific metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadMP4Tags(static_cast<TagLib::MP4::Tag *>(pTagFile->tag()));

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

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
    nsAutoPtr<TagLib::Vorbis::File> pTagFile;
    PRBool                          restart;
    PRBool                          isValid = PR_TRUE;
    nsresult                        result = NS_OK;

    /* Open and read the metadata file. */
    pTagFile = new TagLib::Vorbis::File();
    if (!pTagFile)
        result = NS_ERROR_OUT_OF_MEMORY;
    if (NS_SUCCEEDED(result))
        pTagFile->setMaxScanBytes(MAX_SCAN_BYTES);
    if (NS_SUCCEEDED(result))
        pTagFile->open(filePath);
    if (NS_SUCCEEDED(result))
        pTagFile->read();

    /* Check for channel restart. */
    if (NS_SUCCEEDED(result) && !mMetadataChannelID.IsEmpty())
    {
        result =
            mpTagLibChannelFileIOManager->GetChannelRestart(mMetadataChannelID,
                                                            &restart);
        if (NS_SUCCEEDED(result))
        {
            mMetadataChannelRestart = restart;
            if (mMetadataChannelRestart)
                isValid = PR_FALSE;
        }
    }

    /* Read the base file metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        isValid = ReadFile(pTagFile);

    /* Read the Xiph metadata. */
    if (NS_SUCCEEDED(result) && isValid)
        ReadXiphTags(pTagFile->tag());

    /* File is invalid on any error. */
    if (NS_FAILED(result))
        isValid = PR_FALSE;

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


