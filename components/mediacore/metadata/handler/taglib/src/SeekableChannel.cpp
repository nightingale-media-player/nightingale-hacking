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

/*******************************************************************************
 *******************************************************************************
 *
 * Seekable channel.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  SeekableChannel.cpp
* \brief Songbird seekable channel component implementation.
*/

/*******************************************************************************
 *
 * Seekable channel imported services.
 *
 ******************************************************************************/

/* Local file imports. */
#include "SeekableChannel.h"

/* Mozilla imports. */
#include <nsIInputStream.h>
#include <nsIIOService.h>
#include <nsIResumableChannel.h>
#include <nsIURI.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <prlog.h>


/*******************************************************************************
 *
 * Seekable channel logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbSeekableChannel");
#define LOG(args) if (gLog) PR_LOG(gLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)   /* nothing */
#endif


/*******************************************************************************
 *
 * Seekable channel nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_ISUPPORTS5(sbSeekableChannel, sbISeekableChannel,
                                      nsIStreamListener,
                                      nsIRequestObserver,
                                      nsIChannelEventSink,
                                      nsIInterfaceRequestor)


/*******************************************************************************
 *
 * Seekable channel sbISeekableChannel implementation.
 *
 ******************************************************************************/

/*
 * Open
 *
 *   --> pChannel               Base channel.
 *   --> pListener              Listener for channel events.
 *
 *   This function opens a seekable channel using the base channel specified by
 * pChannel.  Events relating to the channel are delivered to the listener
 * specified by pListener.
 */

NS_IMETHODIMP sbSeekableChannel::Open(
    nsIChannel                  *pChannel,
    sbISeekableChannelListener  *pListener)
{
    nsCOMPtr<nsIRequest>        pRequest;
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    if (!pChannel || !pListener)
        result = NS_ERROR_NULL_POINTER;

    /* Ensure the channel is closed. */
    if (NS_SUCCEEDED(result))
        Close();

    /* Initialize the channel parameters. */
    if (NS_SUCCEEDED(result))
    {
        mpChannel = pChannel;
        mpListener = pListener;
        mContentLength = 0;
        mBasePos = 0;
        mPos = 0;
        mRestarting = PR_FALSE;
    }

    /* Set up the channel request load flags. */
    if (NS_SUCCEEDED(result))
    {
        pRequest = do_QueryInterface(mpChannel, &result);
        if (NS_SUCCEEDED(result))
        {
            result = pRequest->SetLoadFlags
                                    (  nsIRequest::INHIBIT_CACHING
                                     | nsIRequest::INHIBIT_PERSISTENT_CACHING
                                     | nsIRequest::LOAD_BYPASS_CACHE);
        }
        NS_ASSERTION(NS_SUCCEEDED(result),
                     "Setting load flags failed "
                     ":( Watch out, app will deadlock.");
    }

    /* Set up the channel notification callbacks. */
    if (NS_SUCCEEDED(result))
        result = mpChannel->SetNotificationCallbacks(this);

    /* Open the channel. */
    if (NS_SUCCEEDED(result))
        result = mpChannel->AsyncOpen(this, nsnull);

    return (result);
}


/*
 * Close
 *
 *   This function closes the seekable channel.
 */

NS_IMETHODIMP sbSeekableChannel::Close()
{
    DataSet::iterator           dataSetIterator;
    Segment                     *pSegment;
    PRBool                      pending;
    nsresult                    result = NS_OK;

    /* Cancel any pending base channel requests. */
    if (mpChannel)
    {
        pending = PR_FALSE;
        mpChannel->IsPending(&pending);
        if (pending)
            mpChannel->Cancel(NS_ERROR_ABORT);
        mpChannel->SetNotificationCallbacks(nsnull);
    }

    /* Empty the data set. */
    dataSetIterator = mChannelData.begin();
    while (dataSetIterator != mChannelData.end())
    {
        pSegment = *dataSetIterator++;
        mChannelData.erase(pSegment);
        delete (pSegment);
    }

    /* Reset the channel parameters. */
    mpChannel = nsnull;
    mpListener = nsnull;
    mContentLength = 0;
    mPos = 0;
    mBasePos = 0;
    mRestarting = PR_FALSE;

    return (result);
}


/*
 * Skip
 *
 *   --> numBytes               Number of bytes to skip.
 *
 *   This function skips forward in the channel by the number of bytes specified
 * by numBytes.
 */

NS_IMETHODIMP sbSeekableChannel::Skip(
    PRUint64                    numBytes)
{
    nsresult                    result = NS_OK;

    return (result);
}


/*
 * Read
 * 
 *   --> buffer                 Buffer in which to place read data.
 *   --> length                 Number of bytes to read.
 *   <-- pBytesRead             Number of bytes read.
 *
 *   This function reads from the channel the number of bytes specified by
 * length and places the read data into the buffer specified by buffer.  The
 * number of bytes actually read is returned in pBytesRead.
 */

NS_IMETHODIMP sbSeekableChannel::Read(
    char                        *buffer,
    PRUint32                    length,
    PRUint32                    *pBytesRead)
{
    PRUint32                    readLength = length;
    DataSet::iterator           dataSetIterator;
    Segment                     findSegment;
    Segment                     *pSegment = NULL;
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    if (!buffer)
        result = NS_ERROR_NULL_POINTER;

    /* Check if channel is restarting. */
    if (NS_SUCCEEDED(result) && (mRestarting))
        result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;

    /* Prevent reading past end of file. */
    if (mPos >= mContentLength)
        result = NS_ERROR_UNEXPECTED;
    if (NS_SUCCEEDED(result) && ((mPos + readLength) > mContentLength))
        readLength = mContentLength - mPos;

    /* Find a segment containing the data to read. */
    if (NS_SUCCEEDED(result))
    {
        findSegment.offset = mPos;
        findSegment.length = 0;
        dataSetIterator = mChannelData.find(&findSegment);
        if (dataSetIterator == mChannelData.end())
            result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;
    }

    /* Check if data segment contains the requested amount of data. */
    /*zzz should return partial result. */
    if (NS_SUCCEEDED(result))
    {
        pSegment = *dataSetIterator;
        if ((mPos + readLength) > (pSegment->offset + pSegment->length))
            result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;
    }

    /* Copy data to read buffer. */
    if (NS_SUCCEEDED(result))
    {
        memcpy(buffer,
               pSegment->buffer + (mPos - pSegment->offset),
               readLength);
        mPos += readLength;
    }

    /* Restart channel if needed. */
    if (result == NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART)
    {
        /* If the requested data starts in an available segment, restart     */
        /* from the end of the segment.  Otherwise, restart from the current */
        /* channel position.                                                 */
        if (pSegment)
            Restart(pSegment->offset + pSegment->length);
        else
            Restart(mPos);
    }

    /* Return results. */
    if (NS_SUCCEEDED(result))
        *pBytesRead = readLength;

    return (result);
}


/*
 * ReadChar
 *
 *   <-- pChar                  Read character.
 *
 *   This function reads a character from the channel and returns it in pChar.
 */

NS_IMETHODIMP sbSeekableChannel::ReadChar(
    char                        *pChar)
{
    nsresult                    result = NS_OK;

    return (result);
}


/*
 * ReadInt32
 *
 *   <-- pInt32                 Read 32-bit integer.
 *
 *   This function reads a 32-bit integer from the channel and returns it in
 * pInt32.
 */

NS_IMETHODIMP sbSeekableChannel::ReadInt32(
    PRInt32                     *pInt32)
{
    nsresult                    result = NS_OK;

    return (result);
}


/*
 * ReadInt64
 *
 *   <-- pInt64                 Read 64-bit integer.
 *
 *   This function reads a 64-bit integer from the channel and returns it in
 * pInt64.
 */

NS_IMETHODIMP sbSeekableChannel::ReadInt64(
    PRInt64                     *pInt64)
{
    nsresult                    result = NS_OK;

    return (result);
}


/*
 * Getters/setters.
 */

NS_IMETHODIMP sbSeekableChannel::GetPos(
    PRUint64                    *pPos)
{
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    if (!pPos)
        result = NS_ERROR_NULL_POINTER;

    /* Return results. */
    if (NS_SUCCEEDED(result))
        *pPos = mPos;

    return (result);
}


NS_IMETHODIMP sbSeekableChannel::SetPos(
    PRUint64                    pos)
{
    DataSet::iterator           dataSetIterator;
    Segment                     findSegment;
    nsresult                    result = NS_OK;

    /* Check if channel is restarting. */
    if (mRestarting)
        result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;

    /* Find a segment containing the new position. */
    if (NS_SUCCEEDED(result) && (pos < mContentLength))
    {
        findSegment.offset = pos;
        findSegment.length = 0;
        dataSetIterator = mChannelData.find(&findSegment);
        if (dataSetIterator == mChannelData.end())
            result = NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART;
    }

    /* Restart channel if needed. */
    if (!mRestarting && (result == NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART))
        Restart(pos);

    /* Set the channel position. */
    if (NS_SUCCEEDED(result))
        mPos = pos;

    return (result);
}


NS_IMETHODIMP sbSeekableChannel::GetSize(
    PRUint64                    *pSize)
{
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    if (!pSize)
        result = NS_ERROR_NULL_POINTER;

    /* Return results. */
    if (NS_SUCCEEDED(result))
        *pSize = mContentLength;

    return (result);
}


/*
 * GetCompleted
 *
 *   <-- pCompleted             True if channel reading is complete.
 *
 *   This function returns in pCompleted the channel reading complete state.  If
 * all the data for the channel has been read or if a channel read error
 * occured, this function returns true in pCompleted; otherwise, it returns
 * false.
 */

NS_IMETHODIMP sbSeekableChannel::GetCompleted(
    PRBool                      *pCompleted)
{
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    if (!pCompleted)
        result = NS_ERROR_NULL_POINTER;

    /* Return results. */
    *pCompleted = mCompleted;

    return (result);
}


/*******************************************************************************
 *
 * Seekable channel nsIStreamListener implementation.
 *
 ******************************************************************************/

/*
 * OnDataAvailable
 *
 *   --> pRequest               Channel request.
 *   --> pCtx                   Request context.
 *   --> pStream                Input stream.
 *   --> offset                 Offset within stream.
 *   --> numBytes               Number of stream data bytes available.
 *
 *   This function is called when data from the stream specified by pStream is
 * available to be read.  The data request is specified by pRequest and the
 * requestor context is specified by pCtx.
 *   The available data is located at the stream offset specified by offset and
 * is numBytes in length.
 */

NS_IMETHODIMP sbSeekableChannel::OnDataAvailable(
    nsIRequest                  *pRequest,
    nsISupports                 *pCtx,
    nsIInputStream              *pStream,
    PRUint32                    offset,
    PRUint32                    numBytes)
{
    nsresult                    result = NS_OK;

    /* Do nothing if channel is restarting. */
    if (mRestarting)
        return (NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART);

    /* Read the base channel data into a data segment. */
    if (numBytes > 0)
    {
        mDataReceivedSinceStart = PR_TRUE;
        ReadSegment(pStream, numBytes);
    }

    /* Call the OnChannelDataAvailable handler. */
    /*zzz should read minimum block size before calling handler. */
    if (mpListener)
        mpListener->OnChannelDataAvailable(this);

    return (result);
}


/*******************************************************************************
 *
 * Seekable channel nsIRequestObserver implementation.
 *
 ******************************************************************************/

/*
 * OnStartRequest
 *
 *   --> pRequest               Request being started.
 *   --> pCtx                   Request context.
 *
 *   This function is called when the request specified by pRequest is started.
 * The requestor context is specified by pCtx.
 */

NS_IMETHODIMP sbSeekableChannel::OnStartRequest(
    nsIRequest                  *pRequest,
    nsISupports                 *pCtx)
{
    NS_ENSURE_STATE(mpChannel);

    PRInt32                     contentLength;
    nsresult                    result = NS_OK;

    /* Channel restart is complete. */
    mRestarting = PR_FALSE;

    /* Indicate that no data has been received */
    /* since the start of the request.         */
    mDataReceivedSinceStart = PR_FALSE;

    /* Get the channel content length, preserving the sign. */
    if (!mContentLength)
    {
        result = mpChannel->GetContentLength(&contentLength);
        if (NS_SUCCEEDED(result) && (contentLength > 0))
            mContentLength = (PRInt64) contentLength;
    }

    return (result);
}


/*
 * OnStopRequest
 *
 *   --> pRequest               Request being stopped.
 *   --> pCtx                   Request context.
 *   --> status                 Request status.
 *
 *   This function is called when the request specified by pRequest is stopped.
 * The requestor context is specified by pCtx and the request status is
 * specified by status.
 */

NS_IMETHODIMP sbSeekableChannel::OnStopRequest(
    nsIRequest                  *pRequest,
    nsISupports                 *pCtx,
    nsresult                    status)
{
    nsresult                    result = NS_OK;

    /* Do nothing if channel is restarting. */
    if (mRestarting)
        return (NS_ERROR_SONGBIRD_SEEKABLE_CHANNEL_RESTART);

    /* Channel reading is complete on error or if no data */
    /* has been received since the request was started.   */
    if (NS_FAILED(status) || !mDataReceivedSinceStart)
        mCompleted = PR_TRUE;

    /* Call the OnChannelDataAvailable handler. */
    if (mpListener)
        mpListener->OnChannelDataAvailable(this);

    return (result);
}


/*******************************************************************************
 *
 * Seekable channel nsIChannelEventSink implementation.
 *
 ******************************************************************************/

/*
 * OnChannelRedirect
 *
 *   --> pOrigChannel           Original channel.
 *   --> pRedirectedChannel     Redirected channel.
 *   --> flags                  Redirection flags.
 *
 *   This function handles redirection of the channel specified by pOrigChannel
 * to the channel specified by pRedirectedChannel.  Flags relating to the
 * redirection are specified by flags.
 */

NS_IMETHODIMP sbSeekableChannel::OnChannelRedirect(
    nsIChannel                  *pOrigChannel,
    nsIChannel                  *pRedirectedChannel,
    PRUint32                    flags)
{
    nsresult                    result = NS_OK;

    /* Redirect channel. */
    mpChannel = pRedirectedChannel;

    return (result);
}


/*******************************************************************************
 *
 * Seekable channel nsIInterfaceRequestor implementation.
 *
 ******************************************************************************/

/*
 * GetInterface
 *
 *   --> iid                    Interface to get.
 *   <-- ppInterface            Requested interface.
 *
 *   This function returns in ppInterface an object implementing the interface
 * specified by iid for the component.
 */

NS_IMETHODIMP sbSeekableChannel::GetInterface(
    const nsIID                 &iid,
    void                        **ppInterface)
{
    void                        *pInterface = nsnull;
    nsresult                    result = NS_OK;

    /* Validate parameters. */
    NS_ENSURE_ARG_POINTER(ppInterface);

    /* Query component for the interface. */
    result = QueryInterface(iid, &pInterface);

    /* Return results. */
    *ppInterface = pInterface;

    return (result);
}


/*******************************************************************************
 *
 * Public seekable channel services.
 *
 ******************************************************************************/

/*
 * sbSeekableChannel
 *
 *   This method is the constructor for the seekable channel class.
 */

sbSeekableChannel::sbSeekableChannel()
:
    mCompleted(PR_FALSE)
{
}


/*
 * ~sbSeekableChannel
 *
 *   This method is the destructor for the seekable channel class.
 */

sbSeekableChannel::~sbSeekableChannel()
{
}


/*******************************************************************************
 *
 * Private seekable channel services.
 *
 ******************************************************************************/

/*
 * ReadSegment
 *
 *   --> pStream                Stream from which to read.
 *   --> numBytes               Number of bytes to read.
 *
 *   This function reads a segment of data from the stream specified by pStream.
 * The size of the data segment is specified by numBytes.
 */

nsresult sbSeekableChannel::ReadSegment(
    nsIInputStream              *pStream,
    PRUint32                    numBytes)
{
    Segment                     *pSegment = NULL;
    char                        *buffer = NULL;
    PRUint64                    readOffset;
    PRUint32                    bytesRead;
    nsresult                    result = NS_OK;

    /* Allocate a data segment buffer. */
    buffer = (char *) nsMemory::Alloc(numBytes);
    if (!buffer)
        result = NS_ERROR_OUT_OF_MEMORY;

    /* Read data from the stream. */
    if (NS_SUCCEEDED(result))
    {
        result = pStream->Read(buffer, numBytes, &bytesRead);
        if (NS_SUCCEEDED(result))
        {
            readOffset = mBasePos;
            mBasePos += bytesRead;
            if (mBasePos > mContentLength)
                mContentLength = mBasePos;
        }
    }

    /* Create a new data segment buffer. */
    if (NS_SUCCEEDED(result))
    {
        pSegment = new Segment();
        if (pSegment)
        {
            pSegment->offset = readOffset;
            pSegment->length = bytesRead;
            pSegment->buffer = buffer;
            buffer = NULL;
        }
        else
        {
            result = NS_ERROR_UNEXPECTED;
        }
    }

    /* Insert the segment into the channel data set. */
    if (NS_SUCCEEDED(result))
        result = InsertSegment(pSegment);
    if (NS_SUCCEEDED(result))
        pSegment = NULL;

    /* Channel reading is complete if an error occured */
    /* or if all of the channel data has been read.    */
    if (NS_FAILED(result) || AllDataRead())
        mCompleted = PR_TRUE;

    /* Clean up on error. */
    if (!NS_SUCCEEDED(result))
    {
        if (pSegment)
            delete (pSegment);
        if (buffer)
            nsMemory::Free(buffer);
    }

    return (result);
}


/*
 * InsertSegment
 *
 *   --> pInsertSegment         Data segment to insert.
 *
 *   This function inserts the data segment specified by pInsertSegment into the
 * channel data set.  If the insert segment intersects any segments in the data
 * set, this function merges them.
 */

nsresult sbSeekableChannel::InsertSegment(
    Segment                     *pInsertSegment)
{
    DataSet::iterator           dataSetIterator;
    Segment                     *pSegment = NULL;
    nsresult                    result = NS_OK;

    /* Find the first intersecting data segment and merge with it.  If new    */
    /* data segment does not intersect any other data segments, simply insert */
    /* it into data set.                                                      */
    dataSetIterator = mChannelData.find(pInsertSegment);
    if (dataSetIterator != mChannelData.end())
    {
        pSegment = *dataSetIterator;
        mChannelData.erase(pSegment);
        result = MergeSegments(pSegment, pInsertSegment, &pSegment);
        InsertSegment(pSegment);
    }
    else
    {
        mChannelData.insert(pInsertSegment);
    }

    return (result);
}


/*
 * MergeSegments
 *
 *   --> pSegment1,             Segments to merge.
 *       pSegment2
 *   <-- ppMergedSegment        Merged segment.
 *
 *   This function merges the segments pSegment1 and pSegment2 and returns the
 * merged segment in ppMergedSegment.  The caller should not use pSegment1 or
 * pSegment2 after this function returns; this function handles all required
 * clean up.
 */

nsresult sbSeekableChannel::MergeSegments(
    Segment                     *pSegment1,
    Segment                     *pSegment2,
    Segment                     **ppMergedSegment)
{
    Segment                     *pToSegment;
    Segment                     *pFromSegment;
    PRUint64                    mergedLength;
    PRUint64                    mergeFromOffset;
    nsresult                    result = NS_OK;

    /* Use the segment with the smallest offset as the segment to merge to. */
    if (pSegment1->offset <= pSegment2->offset)
    {
        pToSegment = pSegment1;
        pFromSegment = pSegment2;
    }
    else
    {
        pToSegment = pSegment2;
        pFromSegment = pSegment1;
    }

    /* Determine the starting merge offset within the segment to merge from. */
    mergeFromOffset =   pToSegment->offset + pToSegment->length
                      - pFromSegment->offset;

    /* Merge if the segment to merge from is not   */
    /* fully contained in the segment to merge to. */
    if (mergeFromOffset < pFromSegment->length)
    {
        /* Compute the merged segment length. */
        mergedLength =   pFromSegment->offset + pFromSegment->length
                       - pToSegment->offset;

        /* Reallocate the segment buffer to merge to. */
        pToSegment->buffer =
            (char *) nsMemory::Realloc(pToSegment->buffer, mergedLength);
        if (!pToSegment->buffer)
            result = NS_ERROR_OUT_OF_MEMORY;

        /* Copy the merge data. */
        if (NS_SUCCEEDED(result))
        {
            memcpy(pToSegment->buffer + pToSegment->length,
                   pFromSegment->buffer + mergeFromOffset,
                   pFromSegment->length - mergeFromOffset);
            pToSegment->length = mergedLength;
        }
    }

    /* Dispose of the segment merged from. */
    delete(pFromSegment);

    /* Dispose of the segment merged to on error. */
    if (!NS_SUCCEEDED(result))
        delete(pToSegment);

    /* Return results. */
    if (NS_SUCCEEDED(result))
        *ppMergedSegment = pToSegment;

    return (result);
}


/*
 * AllDataRead
 *
 *   <--                        True if all channel data has been read.
 *
 *   This function returns true if all of the channel data has been read.
 */

PRBool sbSeekableChannel::AllDataRead()
{
    DataSet::iterator           dataSetIterator;
    Segment                     *pFirstSegment;
    PRBool                      allDataRead = PR_FALSE;

    /* When all data has been read, the data set will consist of */
    /* a single segment containing all of the channel content.   */
    dataSetIterator = mChannelData.begin();
    if (dataSetIterator != mChannelData.end())
    {
        pFirstSegment = *dataSetIterator;
        if (   (pFirstSegment->offset == 0)
            && (pFirstSegment->length == mContentLength))
        {
            allDataRead = PR_TRUE;
        }
    }

    return (allDataRead);
}


/*
 * Restart
 *
 *   --> pos                    New channel position.
 *
 *   This function restarts the base channel at the position specified by pos.
 */

nsresult sbSeekableChannel::Restart(
    PRUint64                    pos)
{
    nsCOMPtr<nsIResumableChannel>
                                pResumableChannel;
    nsCOMPtr<nsIURI>            pURI;
    nsCOMPtr<nsIIOService>      pIOService;
    nsCOMPtr<nsIRequest>        pRequest;
    nsresult                    result = NS_OK;

    /* Do nothing if already restarting or if the  */
    /* base channel position is not being changed. */
    if ((mRestarting) || (mBasePos == pos))
        return (result);

    /* Check if channel is resumable. */
    pResumableChannel = do_QueryInterface(mpChannel, &result);

    /* Get the channel URI. */
    if (NS_SUCCEEDED(result))
        result = mpChannel->GetURI(getter_AddRefs(pURI));

    /* Shut down channel. */
    if (NS_SUCCEEDED(result))
    {
        mpChannel->Cancel(NS_ERROR_ABORT);
        mpChannel = nsnull;
    }

    /* Get a new channel. */
    if (NS_SUCCEEDED(result))
    {
        pIOService = do_GetService("@mozilla.org/network/io-service;1",
                                   &result);
    }
    if (NS_SUCCEEDED(result))
        result = pIOService->NewChannelFromURI(pURI, getter_AddRefs(mpChannel));

    /* Set up the channel request load flags. */
    if (NS_SUCCEEDED(result))
    {
        pRequest = do_QueryInterface(mpChannel, &result);
        if (NS_SUCCEEDED(result))
        {
            result = pRequest->SetLoadFlags
                                    (  nsIRequest::INHIBIT_CACHING
                                     | nsIRequest::INHIBIT_PERSISTENT_CACHING
                                     | nsIRequest::LOAD_BYPASS_CACHE);
        }
        NS_ASSERTION(NS_SUCCEEDED(result),
                     "Setting load flags failed "
                     ":( Watch out, app will deadlock.");
    }

    /* Resume channel. */
    if (NS_SUCCEEDED(result))
        pResumableChannel = do_QueryInterface(mpChannel, &result);
    if (NS_SUCCEEDED(result))
        pResumableChannel->ResumeAt(pos, NS_LITERAL_CSTRING(""));
    if (NS_SUCCEEDED(result))
        mpChannel->AsyncOpen(this, nsnull);

    /* Set new base channel position and restarting indication. */
    if (NS_SUCCEEDED(result))
    {
        mBasePos = pos;
        mRestarting = PR_TRUE;
    }

    return (result);
}


/*
 * DumpChannelData
 *
 *   This function dumps the channel data set.
 */

void sbSeekableChannel::DumpChannelData()
{
    DataSet::iterator           segmentIterator;
    Segment                     *pSegment;

    for (segmentIterator = mChannelData.begin();
         segmentIterator != mChannelData.end();
         segmentIterator++)
    {
        pSegment = *segmentIterator;
        LOG(("Segment 0x%08x 0x%08x\n",
             (PRUint32) (pSegment->offset & 0xFFFFFFFF),
             (PRUint32) (pSegment->length & 0xFFFFFFFF)));
        LOG(("    0x%08x\n", ((PRUint32 *) pSegment->buffer)[0]));
    }
}


/*******************************************************************************
 *
 * Private seekable channel data segment services.
 *
 ******************************************************************************/


/*
 * Segment
 *
 *   This method is the constructor for the Segment class.
 */

sbSeekableChannel::Segment::Segment()
:
    offset(0),
    length(0),
    buffer(NULL)
{
}


/*
 * ~Segment
 *
 *   This method is the destructor for the Segment class.
 */

sbSeekableChannel::Segment::~Segment()
{
    if (buffer)
        nsMemory::Free(buffer);
}


/*
 * operator
 *
 *   --> pSegment1,         Segments to compare.
 *       pSegment2
 *
 *   This function implements the comparison operator for a std::set.
 * If pSegment1 comes before pSegment2, this function returns true.
 *   A data segment is considered to come before another if its offset
 * is the lesser of the two and if the two segments do not intersect.
 * Thus, intersecting segments are considered to be equivalent.  This
 * property is useful for segment merging.
 */

bool sbSeekableChannel::Segment::operator()(
    const Segment           *pSegment1,
    const Segment           *pSegment2) const
{
    return (  (pSegment1->offset + pSegment1->length)
            < pSegment2->offset);
}


