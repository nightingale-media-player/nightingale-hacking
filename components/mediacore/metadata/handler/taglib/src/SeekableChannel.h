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

#ifndef __SEEKABLE_CHANNEL_H__
#define __SEEKABLE_CHANNEL_H__

/*******************************************************************************
 *******************************************************************************
 *
 * Seekable channel.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  SeekableChannel.h
* \brief Songbird Seekable Channel Component Definition.
*/

/*******************************************************************************
 *
 * Seekable channel configuration.
 *
 ******************************************************************************/

/*
 * Seekable channel XPCOM component definitions.
 */

#define SONGBIRD_SEEKABLECHANNEL_CONTRACTID                                    \
                        "@songbirdnest.com/Songbird/SeekableChannel;1"
#define SONGBIRD_SEEKABLECHANNEL_CLASSNAME                                     \
                                    "Songbird Seekable Channel Component"
#define SONGBIRD_SEEKABLECHANNEL_CID                                           \
{                                                                              \
    0x2030739B,                                                                \
    0x5E60,                                                                    \
    0x492B,                                                                    \
    { 0xA1, 0xA7, 0x74, 0x70, 0x67, 0x47, 0x5B, 0xFD }                         \
}


/*******************************************************************************
 *
 * Seekable channel imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include <sbISeekableChannel.h>

/* Mozilla imports. */
#include <nsCOMPtr.h>
#include <nsIChannel.h>
#include <nsIChannelEventSink.h>
#include <nsIInterfaceRequestor.h>
#include <nsIStreamListener.h>

/* C++ std imports. */
#include <set>


/*******************************************************************************
 *
 * Seekable channel constants.
 *
 ******************************************************************************/

#define NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART                             \
                        NS_ERROR_GENERATE_FAILURE(NS_ERROR_MODULE_GENERAL, 1)


/*******************************************************************************
 *
 * Seekable channel classes.
 *
 ******************************************************************************/

/*
 * sbSeekableChannel class
 */

class sbSeekableChannel : public sbISeekableChannel,
                          public nsIStreamListener,
                          public nsIChannelEventSink,
                          public nsIInterfaceRequestor
{
    /***************************************************************************
     *
     * Public interface.
     *
     **************************************************************************/

    public:

    /*
     * Inherited interfaces.
     */

    NS_DECL_ISUPPORTS
    NS_DECL_SBISEEKABLECHANNEL
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSICHANNELEVENTSINK
    NS_DECL_NSIINTERFACEREQUESTOR


    /*
     * Public seekable channel services.
     */

    sbSeekableChannel();

    virtual ~sbSeekableChannel();


    /***************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * Segment
     *
     *   This class is used to hold channel data segments.
     */

    class Segment
    {
        public:

        /*
         * offset                   Offset of data within the channel.
         * length                   Length of data.
         * buffer                   Buffer containing data.
         */

        PRUint64                    offset;
        PRUint64                    length;
        char                        *buffer;


        /*
         * Public data segment services.
         */

        bool operator()(
            const Segment           *pSegment1,
            const Segment           *pSegment2) const;
    };


    /*
     * DataSet                      Channel data set type.
     */

    typedef std::set<Segment *, Segment> DataSet;


    /*
     * mpChannel                    Base channel.
     * mpListener                   Channel listener.
     * mChannelData                 Set of data read from channel.
     * mContentLength               Channel content length;
     * mPos                         Current position within channel.
     * mBasePos                     Current position within base channel.
     * mRestarting                  True if restarting channel.
     */

    nsCOMPtr<nsIChannel>            mpChannel;
    nsCOMPtr<sbISeekableChannelListener>
                                    mpListener;
    DataSet                         mChannelData;
    PRUint64                        mContentLength;
    PRUint64                        mPos;
    PRUint64                        mBasePos;
    PRBool                          mRestarting;


    /*
     * Private seekable channel services.
     */

    nsresult ReadSegment(
        nsIInputStream              *pStream,
        PRUint32                    numBytes);

    nsresult InsertSegment(
        Segment                     *pSegment);

    nsresult MergeSegments(
        Segment                     *pSegment1,
        Segment                     *pSegment2,
        Segment                     **ppMergedSegment);

    nsresult Restart(
        PRUint64                    pos);

    void DumpChannelData();
};


#endif /* __SEEKABLE_CHANNEL_H__ */
