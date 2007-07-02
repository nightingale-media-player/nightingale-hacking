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
 * TagLib nsIChannel file IO.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  TagLibChannelFileIO.cpp
* \brief Songbird TagLib nsIChannel file I/O implementation.
*/

/*******************************************************************************
 *
 * TagLib nsIChannel file I/O imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include "TaglibChannelFileIO.h"

/* Mozilla imports. */
#include <prlog.h>
#include <nsAutoPtr.h>

/* Songbird imports. */
#include "SeekableChannel.h"


/*******************************************************************************
 *
 * TagLib nsIChannel file I/O logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("TagLibChannelFileIO");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif


/*******************************************************************************
 *
 * TagLib nsIChannel file I/O FileIOTypeResolver interface public
 * implementation.
 *
 ******************************************************************************/

/*!
 * This method must be overriden to provide an additional file I/O type
 * resolver.  If the resolver is able to determine the file I/O type it
 * should return a valid File I/O object; if not it should return 0.
 *
 * \note The created file I/O is then owned by the File and should not be
 * deleted.  Deletion will happen automatically when the File passes out
 * of scope.
 */

FileIO *TagLibChannelFileIOTypeResolver::createFileIO(
    const char                  *fileName) const
{
    NS_ASSERTION(fileName, "fileName is null");

    nsCOMPtr<sbISeekableChannel>   pSeekableChannel;
    nsString                       channelID;
    nsAutoPtr<TagLibChannelFileIO> pTagLibChannelFileIO;
    nsresult                       result = NS_OK;

    /* Assume the file name is a channel ID. */
    channelID = NS_ConvertUTF8toUTF16(fileName);

    /* Get the metadata channel from the channel ID.  An error result */
    /* indicates that either the file name was not a channel ID or no */
    /* matching channels are available.                               */
    result = TagLibChannelFileIO::GetChannel(channelID,
                                             getter_AddRefs(pSeekableChannel));

    /* Create a channel file I/O object. */
    if (NS_SUCCEEDED(result))
    {
        pTagLibChannelFileIO = new TagLibChannelFileIO(channelID,
                                                       pSeekableChannel);
        if (!pTagLibChannelFileIO)
            result = NS_ERROR_UNEXPECTED;
    }
    if (NS_SUCCEEDED(result))
        result = pTagLibChannelFileIO->seek(0);

    return (pTagLibChannelFileIO.forget());
}


/*******************************************************************************
 *
 * TagLib nsIChannel file I/O FileIO interface public implementation.
 *
 ******************************************************************************/

/*!
 * Returns the file name in the local file system encoding.
 */

const char *TagLibChannelFileIO::name() const
{
    LOG(("1: name\n"));

    /* Fail immediately if restarting channel. */
    if (mChannelRestart)
        return ("");

    return ("");
}


/*!
 * Reads a block of size \a length at the current get pointer.
 */

ByteVector TagLibChannelFileIO::readBlock(
    TagLib::ulong               length)
{
    ByteVector                  byteVector;
    PRUint32                    bytesRead;
    nsresult                    result = NS_OK;

    /* Set up a byte vector to contain the read data. */
    byteVector.resize((TagLib::uint) length);

    /* Fail if restarting channel. */
    if (mChannelRestart)
        result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;

    /* Read the file data. */
    if (NS_SUCCEEDED(result))
    {
        result = mpSeekableChannel->Read(byteVector.data(),
                                         length,
                                         &bytesRead);
    }
    if (NS_SUCCEEDED(result))
        byteVector.resize(bytesRead);

    /* Check for channel restart. */
    if (result == NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART)
    {
        SetChannelRestart(mChannelID, PR_TRUE);
        mChannelRestart = PR_TRUE;
    }

    /* Clear read data on error. */
    if (!NS_SUCCEEDED(result))
        byteVector.resize(0);

    return (byteVector);
}


/*!
 * Attempts to write the block \a data at the current get pointer.  If the
 * file is currently only opened read only -- i.e. readOnly() returns true --
 * this attempts to reopen the file in read/write mode.
 *
 * \note This should be used instead of using the streaming output operator
 * for a ByteVector.  And even this function is significantly slower than
 * doing output with a char[].
 */

void TagLibChannelFileIO::writeBlock(
    const ByteVector            &data)
{
    LOG(("1: writeBlock\n"));
}


/*!
 * Insert \a data at position \a start in the file overwriting \a replace
 * bytes of the original content.
 *
 * \note This method is slow since it requires rewriting all of the file
 * after the insertion point.
 */

void TagLibChannelFileIO::insert(
    const ByteVector            &data,
    TagLib::ulong               start,
    TagLib::ulong               replace)
{
    LOG(("1: insert\n"));
}


/*!
 * Removes a block of the file starting a \a start and continuing for
 * \a length bytes.
 *
 * \note This method is slow since it involves rewriting all of the file
 * after the removed portion.
 */

void TagLibChannelFileIO::removeBlock(
    TagLib::ulong               start,
    TagLib::ulong               length)
{
    LOG(("1: removeBlock\n"));
}


/*!
 * Returns true if the file is read only (or if the file can not be opened).
 */

bool TagLibChannelFileIO::readOnly() const
{
    LOG(("1: readOnly\n"));

    return (true);
}


/*!
 * Since the file can currently only be opened as an argument to the
 * constructor (sort-of by design), this returns if that open succeeded.
 */

bool TagLibChannelFileIO::isOpen() const
{
    bool                        isOpen = false;
    nsresult                    result = NS_OK;

    /* Fail if restarting channel. */
    if (mChannelRestart)
        result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;

    /* File I/O is open if a metadata channel is available. */
    if (NS_SUCCEEDED(result) && mpSeekableChannel)
        isOpen = true;

    return (isOpen);
}


/*!
 * Move the I/O pointer to \a offset in the file from position \a p.  This
 * defaults to seeking from the beginning of the file.
 *
 * \see Position
 */

int TagLibChannelFileIO::seek(
    long                        offset,
    Position                    p)
{
    PRUint64                    channelPosition;
    nsresult                    result = NS_OK;

    /* Fail if restarting channel. */
    if (mChannelRestart)
        result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;

    /* Compute new channel position. */
    if (NS_SUCCEEDED(result))
    {
        switch (p)
        {
            default :
            case Beginning :
                channelPosition = offset;
                break;

            case Current :
                result = mpSeekableChannel->GetPos(&channelPosition);
                if (NS_SUCCEEDED(result))
                    channelPosition = channelPosition + offset;
                break;

            case End :
                channelPosition = mChannelSize + offset;
                break;
        }
    }

    /* Set the new position. */
    if (NS_SUCCEEDED(result))
        result = mpSeekableChannel->SetPos(channelPosition);

    /* Check for channel restart. */
    if (result == NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART)
    {
        SetChannelRestart(mChannelID, PR_TRUE);
        mChannelRestart = PR_TRUE;
    }

    return (result);
}


/*!
 * Reset the end-of-file and error flags on the file.
 */

void TagLibChannelFileIO::clear()
{
    LOG(("1: clear\n"));
}


/*!
 * Returns the current offset withing the file.
 */

long TagLibChannelFileIO::tell() const
{
    PRUint64                    channelPosition;
    long                        position = -1;
    nsresult                    result = NS_OK;

    /* Fail if restarting channel. */
    if (mChannelRestart)
        result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;

    /* Get the current channel position. */
    if (NS_SUCCEEDED(result))
        result = mpSeekableChannel->GetPos(&channelPosition);

    /* Get results. */
    if (NS_SUCCEEDED(result))
        position = channelPosition;

    return (position);
}


/*!
 * Returns the length of the file.
 */

long TagLibChannelFileIO::length()
{
    return (mChannelSize);
}


/*******************************************************************************
 *
 * TagLib nsIChannel file I/O FileIO interface protected implementation.
 *
 ******************************************************************************/

/*!
 * Truncates the file to a \a length.
 */

void TagLibChannelFileIO::truncate(
    long                        length)
{
    LOG(("1: truncate\n"));
}


/*******************************************************************************
 *
 * Public TagLib nsIChannel file I/O services.
 *
 ******************************************************************************/

/*
 * TagLibChannelFileIO
 *
 *   --> channelID              Channel identifier.
 *   --> pSeekableChannel       Channel to use to access metadata.
 *
 *   This function constructs a TagLib nsIChannel file I/O object that uses
 * the metadata channel specified by pSeekableChannel with the ID specified by
 * channelID to acccess metadata.
 */

TagLibChannelFileIO::TagLibChannelFileIO(
    nsString                        channelID,
    sbISeekableChannel*             pSeekableChannel)
:
    mChannelID(channelID),
    mpSeekableChannel(pSeekableChannel),
    mChannelSize(0)
{
    NS_ASSERTION(pSeekableChannel, "pSeekableChannel is null");

    PRUint64                    channelSize;
    nsresult                    result = NS_OK;

    /* Initialize the channel restart status. */
    SetChannelRestart(mChannelID, PR_FALSE);
    mChannelRestart = PR_FALSE;

    /* Get the channel size. */
    result = GetSize(channelID, &channelSize);
    if (NS_SUCCEEDED(result))
        mChannelSize = channelSize;
}


/*
 * ~TagLibChannelFileIO
 *
 *   This function is the destructor for TagLib nsIChannel file I/O objects.
 */

TagLibChannelFileIO::~TagLibChannelFileIO()
{
}


/*******************************************************************************
 *
 * Public TagLib nsIChannel file I/O class services.
 *
 ******************************************************************************/

/*
 * Static field initializers.
 */

TagLibChannelFileIO::ChannelMap TagLibChannelFileIO::mChannelMap;


/*
 * AddChannel
 *
 *   --> channelID              Channel identifier.
 *   --> pSeekableChannel       Metadata channel.
 *
 *   This function adds the metadata channel component instance specified by
 * pSeekableChannel for use by TagLib nsIChannel file I/O objects.  The channel
 * may be retrieved by using the channel identifier specified by channelID.
 */

nsresult TagLibChannelFileIO::AddChannel(
    nsString                    channelID,
    sbISeekableChannel*         pSeekableChannel)
{
    nsAutoPtr<TagLibChannelFileIO::Channel> pChannel;
    nsresult                                result = NS_OK;

    /* Validate parameters. */
    NS_ASSERTION(pSeekableChannel, "pSeekableChannel is null");

    /* Create and initialize a new channel object.          */
    /* Initialize size to 0 because it's not available yet. */
    pChannel = new TagLibChannelFileIO::Channel();
    if (pChannel)
    {
        pChannel->pSeekableChannel = pSeekableChannel;
        pChannel->size = 0;
        pChannel->restart = PR_FALSE;
    }
    else
    {
        result = NS_ERROR_UNEXPECTED;
    }

    /* Add the channel to the channel map. */
    if (NS_SUCCEEDED(result))
        mChannelMap[channelID] = pChannel.forget();

    return (result);
}


/*
 * RemoveChannel
 *
 *   --> channelID              Channel identifier.
 *
 *   This function removes the metadata channel component instance specified by
 * channelID from use by TagLib nsIChannel file I/O objects.
 */

nsresult TagLibChannelFileIO::RemoveChannel(
    nsString                    channelID)
{
    TagLibChannelFileIO::Channel
                                *pChannel = nsnull;
    nsresult                    result = NS_OK;

    /* Get the channel map entry. */
    GetChannel(channelID, &pChannel);

    /* Erase the channel map entry. */
    mChannelMap.erase(channelID);

    /* Delete the channel. */
    if (pChannel)
        delete (pChannel);

    return (result);
}


/*
 * GetChannel
 *
 *   --> channelID              Channel identifier.
 *   <-- ppChannel              TagLib channel.
 *
 *   <-- NS_ERROR_NOT_AVAILABLE No channel with the specified identifier is
 *                              available.
 *
 *   This function returns in ppChannel a TagLib channel corresponding to the
 * channel identifier specified by channelID.  If no channels can be found, this
 * function returns NS_ERROR_NOT_AVAILABLE.
 */

nsresult TagLibChannelFileIO::GetChannel(
    nsString                    channelID,
    TagLibChannelFileIO::Channel
                                **ppChannel)
{
    NS_ASSERTION(ppChannel, "ppChannel is null");

    ChannelMap::iterator        mapEntry;
    TagLibChannelFileIO::Channel
                                *pChannel = NULL;
    nsresult                    result = NS_OK;

    /* Search for the channel in the channel map. */
    mapEntry = mChannelMap.find(channelID);
    if (mapEntry != mChannelMap.end())
        pChannel = mapEntry->second;
    else
        result = NS_ERROR_NOT_AVAILABLE;

    /* Return results. */
    *ppChannel = pChannel;

    return (result);
}


/*
 * GetChannel
 *
 *   --> channelID              Channel identifier.
 *   <-- ppSeekableChannel      Metadata channel.
 *
 *   <-- NS_ERROR_NOT_AVAILABLE No metadata channel with the specified
 *                              identifier is available.
 *
 *   This function returns in ppSeekableChannel a metadata channel component
 * instance corresponding to the channel identifier specified by channelID.  If
 * no metadata channels can be found, this function returns
 * NS_ERROR_NOT_AVAILABLE.
 */

nsresult TagLibChannelFileIO::GetChannel(
    nsString                    channelID,
    sbISeekableChannel          **ppSeekableChannel)
{
    NS_ASSERTION(ppSeekableChannel, "ppSeekableChannel is null");

    ChannelMap::iterator        mapEntry;
    TagLibChannelFileIO::Channel
                                *pChannel;
    nsCOMPtr<sbISeekableChannel>
                                pSeekableChannel;
    nsresult                    result = NS_OK;

    /* Get the metadata channel. */
    result = GetChannel(channelID, &pChannel);
    if (NS_SUCCEEDED(result))
        pSeekableChannel = pChannel->pSeekableChannel;

    /* Return results. */
    if (NS_SUCCEEDED(result))
        NS_ADDREF(*ppSeekableChannel = pSeekableChannel);

    return (result);
}


/*
 * GetSize
 *
 *   --> channelID              Channel identifier.
 *   <-- pSize                  Channel size.
 *
 *   <-- NS_ERROR_NOT_AVAILABLE No channel with the specified identifier is
 *                              available.
 *
 *   This function returns in pSize the size of the channel media corresponding
 * to the channel identifier specified by channelID.  If no matching channels
 * can be found, this function returns NS_ERROR_NOT_AVAILABLE.
 *
 *   Because the channel size information is not always available at the time of
 * the TagLib channel creation, this function reads the channel size if it
 * hasn't been read yet.
 */

nsresult TagLibChannelFileIO::GetSize(
    nsString                    channelID,
    PRUint64                    *pSize)
{
    NS_ASSERTION(pSize, "pSize is null");

    ChannelMap::iterator        mapEntry;
    TagLibChannelFileIO::Channel
                                *pChannel;
    PRUint64                    size = 0;
    nsresult                    result = NS_OK;

    /* Get the channel size. */
    result = GetChannel(channelID, &pChannel);
    if (NS_SUCCEEDED(result))
        size = pChannel->size;

    /* If the size is uninitialized, read it from the metadata channel. */
    if (NS_SUCCEEDED(result) && (size == 0))
    {
        result = pChannel->pSeekableChannel->GetSize(&size);
        if (NS_SUCCEEDED(result))
            pChannel->size = size;
    }

    /* Return results. */
    *pSize = size;

    return (result);
}


/*
 * GetChannelRestart
 *
 *   --> channelID              Channel identifier.
 *
 *   <--                        Channel restart flag.
 *
 *   This function returns the restart flag for the metadata channel specified
 * by channelID.
 */

PRBool TagLibChannelFileIO::GetChannelRestart(
    nsString                    channelID)
{
    TagLibChannelFileIO::Channel
                                *pChannel;
    PRBool                      restart = PR_FALSE;
    nsresult                    result = NS_OK;

    /* Get the channel restart flag. */
    result = GetChannel(channelID, &pChannel);
    if (NS_SUCCEEDED(result))
        restart = pChannel->restart;

    return (restart);
}


/*
 * SetChannelRestart
 *
 *   --> channelID              Channel identifier.
 *   --> restart                Channel restart status.
 *
 *   This function sets the channel restart status for the channel specified by
 * channelID to the value specified by restart.
 */

void TagLibChannelFileIO::SetChannelRestart(
    nsString                    channelID,
    PRBool                      restart)
{
    TagLibChannelFileIO::Channel
                                *pChannel;
    nsresult                    result = NS_OK;

    /* Set the channel restart flag. */
    result = GetChannel(channelID, &pChannel);
    if (NS_SUCCEEDED(result))
        pChannel->restart = restart;
}


