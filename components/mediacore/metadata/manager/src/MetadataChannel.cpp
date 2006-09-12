/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/**
* \file MetadataChannel.h
* \brief 
*/

#pragma once

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>

#include <string/nsStringAPI.h>
#include <nsIInputStream.h>
#include <nsIChannel.h>

#include "MetadataChannel.h"

// DEFINES ====================================================================

// FUNCTIONS ==================================================================

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS3(sbMetadataChannel, sbIMetadataChannel, nsIStreamListener, nsIRequestObserver)

//-----------------------------------------------------------------------------
sbMetadataChannel::sbMetadataChannel() : m_Pos( 0 ), m_Buf( 0 ), m_Blocks(), m_Completed( false )
{
}

//-----------------------------------------------------------------------------
sbMetadataChannel::~sbMetadataChannel()
{
  Close();
}


/* void Open (in nsIChannel channel); */
NS_IMETHODIMP sbMetadataChannel::Open(nsIChannel *channel, sbIMetadataHandler *handler)
{
  if ( ! channel || ! handler )
    return NS_ERROR_NULL_POINTER;

  // Dump anything that might be there.
  Close();

  // Open the channel with ourselves as the listener and the handler as the context.
  m_Channel = channel;
  m_Handler = handler;

  {
    nsCOMPtr<nsIRequest> request = do_QueryInterface(m_Channel);
    PRUint32 loadFlags = nsIRequest::INHIBIT_CACHING | 
      nsIRequest::INHIBIT_PERSISTENT_CACHING |
      nsIRequest::LOAD_BYPASS_CACHE;
    nsresult rv = request->SetLoadFlags(loadFlags);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Setting load flags failed :( Watch out, app will deadlock.");
    if(NS_FAILED(rv))
      return rv;
  }

  return m_Channel->AsyncOpen( this, handler );
}

/* void Close (); */
NS_IMETHODIMP sbMetadataChannel::Close()
{
  if(m_Channel)
  {
    PRBool pendingRequest = PR_FALSE;
    m_Channel->IsPending(&pendingRequest);
    if(pendingRequest)
      m_Channel->Cancel(NS_ERROR_ABORT);
  }

  m_Pos = 0;
  m_Buf = 0;
  m_Blocks.clear();
  m_Channel = nsnull;
  m_Handler = nsnull;
  return NS_OK;
}

/* void SetPos (in PRUint64 pos); */
NS_IMETHODIMP sbMetadataChannel::SetPos(PRUint64 pos)
{
  // For now, disallow seeks past our buffering point.
  if ( m_Pos > m_Buf )
    return NS_ERROR_UNEXPECTED;

  m_Pos = pos;

  return NS_OK;
}

/* PRUint64 GetPos (); */
NS_IMETHODIMP sbMetadataChannel::GetPos(PRUint64 *_retval)
{
  if ( ! _retval )
    return NS_ERROR_NULL_POINTER;

  *_retval = m_Pos;

  return NS_OK;
}

/* PRUint64 GetBuf (); */
NS_IMETHODIMP sbMetadataChannel::GetBuf(PRUint64 *_retval)
{
  if ( ! _retval )
    return NS_ERROR_NULL_POINTER;

  *_retval = m_Buf;

  return NS_OK;
}

/* PRUint64 GetSize (); */
NS_IMETHODIMP sbMetadataChannel::GetSize(PRUint64 *_retval)
{
  if ( ! _retval )
    return NS_ERROR_NULL_POINTER;

  PRInt32 ret = 0;
  
  NS_ASSERTION(m_Channel, "sbMetadataChannel::GetSize called without m_Channel set!");
  
  if(m_Channel)
    m_Channel->GetContentLength( &ret ); 

  *_retval = (PRInt64)ret; // preserve sign.

  return NS_OK;
}

/* void Skip (in PRUint64 skip); */
NS_IMETHODIMP sbMetadataChannel::Skip(PRUint64 skip)
{
  return SetPos( m_Pos + skip );
}

/* PRUint32 Read (in charPtr buf, in PRUint32 len); */
NS_IMETHODIMP sbMetadataChannel::Read(char * out_buf, PRUint32 len, PRUint32 *_retval)
{
  // Sanity check
  if ( ! out_buf )
    return NS_ERROR_NULL_POINTER;
  if ( m_Pos + len >= m_Buf )
    return NS_ERROR_UNEXPECTED;

  *_retval = 0;
  // Write <len> bytes of data to the output buffer, from possibly more than one block.
  for ( PRUint32 remaining = len, count = -1; remaining && count; remaining -= count, m_Pos += count, out_buf += count, *_retval += count )
  {
    // Either to the end of the incoming read or the end of the current block.
    PRUint32 left = (PRUint32)( BLOCK_SIZE - POS(m_Pos) );
    count = std::min( remaining, left );
    char *buf = BUF(m_Pos); // Magic pointer-to-position method
    memcpy( out_buf, buf, count );
  }

  return NS_OK;
}

NS_IMETHODIMP sbMetadataChannel::ReadChar(char *_retval)
{
  // Sanity check
  if ( m_Pos + 1 >= m_Buf )
    return NS_ERROR_UNEXPECTED;

  PRUint32 count;
  Read( _retval, 1, &count );

  return NS_OK;
}

NS_IMETHODIMP sbMetadataChannel::ReadInt32(PRInt32 *_retval)
{
  // Sanity check
  if ( m_Pos + 4 >= m_Buf )
    return NS_ERROR_UNEXPECTED;

  PRUint32 count;
  Read( (char *)_retval, 4, &count );

#ifdef IS_BIG_ENDIAN
  // HMMMMM.  Endianness?
  *_retval = ( ( *_retval & 0xFF000000 ) >> 24 ) |
             ( ( *_retval & 0x00FF0000 ) >> 8 )  |
             ( ( *_retval & 0x0000FF00 ) << 8 )  |
             ( ( *_retval & 0x000000FF ) << 24 );
#endif  

  return NS_OK;
}

NS_IMETHODIMP sbMetadataChannel::ReadInt64(PRInt64 *_retval)
{
  // Sanity check
  if ( m_Pos + 8 >= m_Buf )
    return NS_ERROR_UNEXPECTED;

  PRUint32 count;
  Read( (char *)_retval, 8, &count );

#if 0
#ifdef IS_BIG_ENDIAN
  // HMMMMM.  Endianness?
  *_retval = ( ( *_retval & (long long)0xFF00000000000000 ) >> 56 ) |
             ( ( *_retval & (long long)0x00FF000000000000 ) >> 40 ) |
             ( ( *_retval & (long long)0x0000FF0000000000 ) >> 24 ) |
             ( ( *_retval & (long long)0x000000FF00000000 ) >> 8 )  |
             ( ( *_retval & 0x00000000FF000000 ) << 8 )  |
             ( ( *_retval & 0x0000000000FF0000 ) << 24 ) |
             ( ( *_retval & 0x000000000000FF00 ) << 40 ) |
             ( ( *_retval & 0x00000000000000FF ) << 56 );
#endif
#endif

  return NS_OK;
}

/* PRBool GetSeekable (); */
NS_IMETHODIMP sbMetadataChannel::GetSeekable(PRBool *_retval)
{
  if ( ! _retval )
    return NS_ERROR_NULL_POINTER;

  *_retval = false;

  return NS_OK;
}

/* PRBool GetSeekable (); */
NS_IMETHODIMP sbMetadataChannel::GetCompleted(PRBool *_retval)
{
  if ( ! _retval )
    return NS_ERROR_NULL_POINTER;

  *_retval = m_Completed;

  return NS_OK;
}

NS_IMETHODIMP
sbMetadataChannel::OnDataAvailable(nsIRequest *aRequest,
                                          nsISupports *ctxt,
                                          nsIInputStream *inStr,
                                          PRUint32 sourceOffset,
                                          PRUint32 count)
{
  // Sanity check
  if ( ! aRequest || ! ctxt || ! inStr )
    return NS_ERROR_NULL_POINTER;
  if ( m_Buf != sourceOffset )
    return NS_ERROR_UNEXPECTED;

  // Read <count> bytes of data from the input stream, into possibly more than one block.
  for ( PRUint32 remaining = count, read = -1; remaining && read; remaining -= read, m_Buf += read )
  {
    // Either to the end of the incoming read or the end of the current block.
    PRUint32 left = (PRUint32)( BLOCK_SIZE - POS(m_Buf) );
    PRUint32 len = std::min( remaining, left );
    char *buf = BUF(m_Buf); // Magic pointer-to-position method
    inStr->Read( buf, len, &read );
    buf++;
  }

  PRUint64 size;
  GetSize( &size );
  // Don't send until you get to the end or are over the block size or its broke or something
  if ( ( size == -1 ) || ( m_Buf >= size ) || ( m_Buf >= BLOCK_SIZE ) ) 
  {
    // Inform the handler that we read data.
    nsCOMPtr<sbIMetadataHandler> handler( do_QueryInterface(ctxt) );
    if ( handler.get() )
    {
      handler->OnChannelData( this );
    }
  }
  return NS_OK;
}

/** nsIRequestObserver methods **/

NS_IMETHODIMP
sbMetadataChannel::OnStartRequest(nsIRequest *aRequest,
                                         nsISupports *ctxt)
{
  nsresult result;
  aRequest->GetStatus( &result );
  return NS_OK;
}

NS_IMETHODIMP
sbMetadataChannel::OnStopRequest(nsIRequest *aRequest,
                                        nsISupports *ctxt,
                                        nsresult status)
{
  // Okay, we're done.
  m_Completed = true;
  // Inform the handler that we read data.
  nsCOMPtr<sbIMetadataHandler> handler( do_QueryInterface(ctxt) );
  if ( handler.get() )
  {
    handler->OnChannelData( this );
  }
  return NS_OK;
}
