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

#ifndef __TAGLIB_CHANNEL_FILE_IO_H__
#define __TAGLIB_CHANNEL_FILE_IO_H__

/*******************************************************************************
 *******************************************************************************
 *
 * TagLib sbISeekableChannel file IO.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  TagLibChannelFileIO.h
* \brief Songbird TagLib sbISeekableChannel file IO class.
*/

/*******************************************************************************
 *
 * TagLib sbISeekableChannel file IO imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <sbITagLibChannelFileIOManager.h>

/* Songbird imports. */
#include <sbISeekableChannel.h>

/* TagLib imports. */
#include <tfile.h>

using namespace TagLib;


/*******************************************************************************
 *
 * TagLib sbISeekableChannel file IO classes.
 *
 ******************************************************************************/

/*
 * TagLibChannelFileIO class
 */

class TagLibChannelFileIO
{
    /*
     * Public TagLib sbISeekableChannel file I/O services.
     */

public:
    TagLibChannelFileIO(
        nsCString                       channelID,
        sbISeekableChannel*             pSeekableChannel);

    virtual ~TagLibChannelFileIO();

    nsresult Initialize();


    /*
     * Private TagLib sbISeekable file I/O services.
     *
     *   mChannelID             ID for metadata channel.
     *   mpSeekableChannel      Channel for accessing metadata.
     *   mpTagLibChannelFileIOManager
     *                          TagLib sbISeekableChannel file IO manager.
     *   mChannelSize           Size of channel in bytes.
     *   mChannelRestart        True if channel needs to be restarted.
     */

private:
    nsCString                   mChannelID;
    nsCOMPtr<sbISeekableChannel>
                                mpSeekableChannel;
    nsCOMPtr<sbITagLibChannelFileIOManager>
                                mpTagLibChannelFileIOManager;
    PRUint32                    mChannelSize;
    PRBool                      mChannelRestart;


    /*
     * TagLib::FileIO methods.
     */

public:
    virtual FileName name() const;

    virtual ByteVector readBlock(
        TagLib::ulong               length);

    virtual void writeBlock(
        const ByteVector            &data);

    virtual void insert(
        const ByteVector            &data,
        TagLib::ulong               start = 0,
        TagLib::ulong               replace = 0);

    virtual void removeBlock(
        TagLib::ulong               start = 0,
        TagLib::ulong               length = 0);

    virtual bool readOnly() const;

    virtual bool isOpen() const;

    virtual bool isReadable();

    virtual bool isWritable();

    virtual int seek(
        long                        offset,
        File::Position              p = File::Beginning);

    virtual void clear();

    virtual long tell() const;

    virtual long length();

    /*!
    * Return a temporary file to use, creating it if necessary
    */
    virtual File* tempFile();

    /*!
    * Close any previously allocated temporary files
    * \param overwrite If true, will attempt to replace this file
    */
    virtual bool closeTempFile( bool overwrite );

protected:
    virtual void truncate(
        long                        length);
};


#endif /* __TAGLIB_CHANNEL_FILE_IO_H__ */
