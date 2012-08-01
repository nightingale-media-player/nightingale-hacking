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

/* *****************************************************************************
 *******************************************************************************
 *
 * TagLib sbISeekableChannel file IO.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  TagLibChannelFileIO.cpp
* \brief Songbird TagLib sbISeekableChannel file I/O implementation.
*/

/* *****************************************************************************
 *
 * TagLib sbISeekable file I/O imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include <TaglibChannelFileIO.h>

/* Songbird imports. */
#include <nsServiceManagerUtils.h>
#include <SeekableChannel.h>

/* Mozilla imports. */
#include <prlog.h>
#include <nsAutoPtr.h>


/* *****************************************************************************
 *
 * TagLib sbISeekable file I/O logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("TagLibChannelFileIO");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif


/* *****************************************************************************
 *
 * TagLib sbISeekable file I/O FileIO interface public implementation.
 *
 ******************************************************************************/

/*!
 * Returns the file name in the local file system encoding.
 */

FileName TagLibChannelFileIO::name() const
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
        nsresult                    _result = NS_OK;

        _result = mpTagLibChannelFileIOManager->SetChannelRestart(mChannelID,
                                                                  PR_TRUE);
        if (NS_SUCCEEDED(_result))
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
 * Returns true if the file can be opened for reading.  If the file does not
 * exist, this will return false.
 *
 * \deprecated
 */
bool TagLibChannelFileIO::isReadable()
{
    LOG(("1: isReadable"));
    return (true);
}

/*!
 * Returns true if the file can be opened for writing.
 *
 * \deprecated
 */
bool TagLibChannelFileIO::isWritable()
{
    LOG(("1: isWritable"));

    return (false);
}


/*!
 * Move the I/O pointer to \a offset in the file from position \a p.  This
 * defaults to seeking from the beginning of the file.
 *
 * \see Position
 */

int TagLibChannelFileIO::seek(
    long                        offset,
    File::Position              p)
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
            case File::Beginning :
                channelPosition = offset;
                break;

            case File::Current :
                result = mpSeekableChannel->GetPos(&channelPosition);
                if (NS_SUCCEEDED(result))
                    channelPosition = channelPosition + offset;
                break;

            case File::End :
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
        mpTagLibChannelFileIOManager->SetChannelRestart(mChannelID, PR_TRUE);
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
        position = (long)channelPosition;

    return (position);
}


/*!
 * Returns the length of the file.
 */

long TagLibChannelFileIO::length()
{
    return (mChannelSize);
}

File* TagLibChannelFileIO::tempFile()
{
  // temporary files are not supported (since we shouldn't be using this
  // for any writable files; we should be using TagLib::LocalFileIO instead)
  return NULL;
}

bool TagLibChannelFileIO::closeTempFile( bool overwrite )
{
  // temporary files are not supported (since we shouldn't be using this
  // for any writable files; we should be using TagLib::LocalFileIO instead)
  return false;
}


/* *****************************************************************************
 *
 * TagLib sbISeekable file I/O FileIO interface protected implementation.
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


/* *****************************************************************************
 *
 * Public TagLib sbISeekable file I/O services.
 *
 ******************************************************************************/

/*
 * TagLibChannelFileIO
 *
 *   --> channelID              Channel identifier.
 *   --> pSeekableChannel       Channel to use to access metadata.
 *
 *   This function constructs a TagLib sbISeekable file I/O object that uses
 * the metadata channel specified by pSeekableChannel with the ID specified by
 * channelID to acccess metadata.
 */

TagLibChannelFileIO::TagLibChannelFileIO(
    nsCString                       channelID,
    sbISeekableChannel*             pSeekableChannel)
:
    mChannelID(channelID),
    mpSeekableChannel(pSeekableChannel),
    mChannelSize(0)
{
    /* Validate parameters. */
    NS_ASSERTION(pSeekableChannel, "pSeekableChannel is null");
}


/*
 * ~TagLibChannelFileIO
 *
 *   This function is the destructor for TagLib sbISeekable file I/O objects.
 */

TagLibChannelFileIO::~TagLibChannelFileIO()
{
}


/*
 * Initialize
 *
 *   This function initializes the TagLib sbISeekable file I/O object.
 */

nsresult TagLibChannelFileIO::Initialize()
{
    PRUint64                    channelSize;
    nsresult                    result = NS_OK;

    /* Get the TagLib sbISeekableChannel file IO manager. */
    mpTagLibChannelFileIOManager =
            do_GetService
                ("@songbirdnest.com/Songbird/sbTagLibChannelFileIOManager;1",
                 &result);

    /* Initialize the channel restart status. */
    if (NS_SUCCEEDED(result))
    {
        mpTagLibChannelFileIOManager->SetChannelRestart(mChannelID, PR_FALSE);
        mChannelRestart = PR_FALSE;
    }

    /* Get the channel size. */
    if (NS_SUCCEEDED(result))
    {
        result = mpTagLibChannelFileIOManager->GetChannelSize(mChannelID,
                                                              &channelSize);
    }
    if (NS_SUCCEEDED(result))
        mChannelSize = (PRUint32)channelSize;

    return (result);
}


