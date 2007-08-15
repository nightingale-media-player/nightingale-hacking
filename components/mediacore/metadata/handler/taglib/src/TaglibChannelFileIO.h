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

#ifndef __TAGLIB_CHANNEL_FILE_IO_H__
#define __TAGLIB_CHANNEL_FILE_IO_H__

/*******************************************************************************
 *******************************************************************************
 *
 * TagLib nsIChannel file IO.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  TagLibChannelFileIO.h
* \brief Songbird TagLib nsIChannel file IO class.
*/

/*******************************************************************************
 *
 * TagLib nsIChannel file IO imported services.
 *
 ******************************************************************************/

/* C++ std imports. */
#include <map>

/* Mozilla imports. */
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

/* Songbird imports. */
#include <sbISeekableChannel.h>

/* TagLib imports. */
#include <tfile.h>

using namespace TagLib;


/*******************************************************************************
 *
 * TagLib nsIChannel file IO classes.
 *
 ******************************************************************************/

/*
 * TagLibChannelFileIO class
 */

class TagLibChannelFileIO : public TagLib::FileIO
{
    /*
     * Public TagLib nsIChannel file I/O services.
     */

public:
    TagLibChannelFileIO(
        nsString                        channelID,
        sbISeekableChannel*             pSeekableChannel);

    virtual ~TagLibChannelFileIO();


    /*
     * Private TagLib nsIChannel file I/O services.
     *
     *   mChannelID             ID for metadata channel.
     *   mpSeekableChannel      Channel for accessing metadata.
     *   mChannelSize           Size of channel in bytes.
     *   mChannelRestart        True if channel needs to be restarted.
     */

private:
    nsString                    mChannelID;
    nsCOMPtr<sbISeekableChannel>
                                mpSeekableChannel;
    PRUint32                    mChannelSize;
    PRBool                      mChannelRestart;


    /*
     * TagLib::FileIO methods.
     */

public:
    virtual const char *name() const;

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

    virtual int seek(
        long                        offset,
        Position                    p = Beginning);

    virtual void clear();

    virtual long tell() const;

    virtual long length();

protected:
    virtual void truncate(
        long                        length);


    /*
     * Private TagLib nsIChannel file I/O internal classes.
     */

private:
    class Channel : public nsISupports
    {
        /*
         * pSeekableChannel         Seekable channel component.
         * size                     Size of channel media.
         * restart                  Flag indicating that the channel needs to be
         *                          restarted.
         */

    public:
        /* Inherited interfaces. */
        NS_DECL_ISUPPORTS

        nsCOMPtr<sbISeekableChannel>
                                    pSeekableChannel;
        PRUint64                    size;
        PRBool                      restart;
    };


    /*
     * Public TagLib nsIChannel file I/O class services.
     */

public:
    static nsresult AddChannel(
        nsString                    channelID,
        sbISeekableChannel*         pSeekableChannel);

    static nsresult RemoveChannel(
        nsString                    channelID);

    static nsresult GetChannel(
        nsString                    channelID,
        TagLibChannelFileIO::Channel
                                    **ppChannel);

    static nsresult GetChannel(
        nsString                    channelID,
        sbISeekableChannel          **ppSeekableChannel);

    static nsresult GetSize(
        nsString                    channelID,
        PRUint64                    *pSize);

    static PRBool GetChannelRestart(
        nsString                    channelID);

    static void SetChannelRestart(
        nsString                    channelID,
        PRBool                      restart);


    /*
     * Private TagLib nsIChannel file I/O class services.
     *
     *   mChannelMap            TagLib channel map.
     */

private:
    class ChannelMap :
        public std::map<nsString, nsRefPtr<TagLibChannelFileIO::Channel> > {};
    static ChannelMap mChannelMap;
};


/*
 * TagLibChannelFileIOTypeResolver class
 */

class TagLibChannelFileIOTypeResolver : public File::FileIOTypeResolver
{
public:
    virtual FileIO *createFileIO(
        const char                  *fileName) const;
};


#endif /* __TAGLIB_CHANNEL_FILE_IO_H__ */
