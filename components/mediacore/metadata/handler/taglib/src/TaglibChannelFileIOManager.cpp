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
 * TagLib sbISeekableChannel file IO manager.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  TagLibChannelFileIOManager.cpp
* \brief Songbird TagLib sbISeekableChannel file I/O manager implementation.
*/

/* *****************************************************************************
 *
 * TagLib sbISeekable file I/O manager imported services.
 *
 ******************************************************************************/

/* Self imports. */
#include <TaglibChannelFileIOManager.h>

/* Mozilla imports. */
#include <nsServiceManagerUtils.h>
#include <prlog.h>


/* *****************************************************************************
 *
 * TagLib sbISeekable file I/O manager logging services.
 *
 ******************************************************************************/

#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = PR_NewLogModule("sbTagLibChannelFileIOManager");
#define TRACE(args) PR_LOG(gLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif


/*******************************************************************************
 *
 * TagLib sbISeekableChannel file I/O manager nsISupports implementation.
 *
 ******************************************************************************/

NS_IMPL_THREADSAFE_ISUPPORTS1(sbTagLibChannelFileIOManager,
                              sbITagLibChannelFileIOManager)


/*******************************************************************************
 *
 * TagLib sbISeekableChannel file I/O manager sbITagLibChannelFileIOManager
 * implementation.
 *
 ******************************************************************************/

/**
* \brief Add an sbISeekableChannel to be used for TagLib file IO.
*
* \param aChannelID Channel identifier.
* \param aChannel Metadata channel.
*/

NS_IMETHODIMP sbTagLibChannelFileIOManager::AddChannel(
    const nsACString            &aChannelID,
    sbISeekableChannel          *aChannel)
{
    nsAutoPtr<sbTagLibChannelFileIOManager::Channel> pChannel;
    PRBool                                           success;

    /* Validate parameters. */
    NS_ENSURE_FALSE(aChannelID.IsEmpty(), NS_ERROR_INVALID_ARG);
    NS_ENSURE_ARG_POINTER(aChannel);

    /* Create a new channel object. */
    pChannel = new sbTagLibChannelFileIOManager::Channel(aChannel);
    NS_ENSURE_TRUE(pChannel, NS_ERROR_OUT_OF_MEMORY);

    /* Add the channel to the channel map. */
    success = mChannelMap.Put(aChannelID, pChannel);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    pChannel.forget();

    return (NS_OK);
}


/**
* \brief Remove an sbISeekableChannel used for TagLib file IO.
*
* \param aChannelID Identifier for channel to be removed.
*/

NS_IMETHODIMP sbTagLibChannelFileIOManager::RemoveChannel(
    const nsACString            &aChannelID)
{
    /* Validate parameters. */
    NS_ENSURE_FALSE(aChannelID.IsEmpty(), NS_ERROR_INVALID_ARG);

    /* Remove the channel map entry. */
    mChannelMap.Remove(aChannelID);

    return (NS_OK);
}


/**
* \brief Get an sbISeekableChannel used for TagLib file IO.
*
* \param aChannelID Identifier for channel to get.
* \return The sbISeekableChannel for the specified identifier.
*/

NS_IMETHODIMP sbTagLibChannelFileIOManager::GetChannel(
    const nsACString            &aChannelID,
    sbISeekableChannel          **_retval)
{
    sbTagLibChannelFileIOManager::Channel   *pChannel;
    nsresult                                rv;

    /* Validate parameters. */
    NS_ENSURE_FALSE(aChannelID.IsEmpty(), NS_ERROR_INVALID_ARG);
    NS_ENSURE_ARG_POINTER(_retval);

    /* Get the metadata channel. */
    rv = GetChannel(aChannelID, &pChannel);
    if (NS_FAILED(rv))
        return (rv);

    /* Return results. */
    NS_ADDREF(*_retval = pChannel->pSeekableChannel);

    return (NS_OK);
}


/**
* \brief Get the size of the channel media.
*
* \param aChannelID Identifier for channel for which to get size.
* \return The channel media size.
*
*   Because the channel size information is not always available at the time of
* the TagLib channel creation, this function reads the channel size if it
* hasn't been read yet.
*/

NS_IMETHODIMP sbTagLibChannelFileIOManager::GetChannelSize(
    const nsACString            &aChannelID,
    PRUint64                    *_retval)
{
    sbTagLibChannelFileIOManager::Channel   *pChannel;
    PRUint64                                size = 0;
    nsresult                                rv;

    /* Validate parameters. */
    NS_ENSURE_FALSE(aChannelID.IsEmpty(), NS_ERROR_INVALID_ARG);
    NS_ENSURE_ARG_POINTER(_retval);

    /* Get the channel size. */
    rv = GetChannel(aChannelID, &pChannel);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = pChannel->pSeekableChannel->GetSize(&size);
    NS_ENSURE_SUCCESS(rv, rv);

    /* Return results. */
    *_retval = size;

    return (NS_OK);
}


/**
* \brief Get the restart flag for the channel.
*
* \param aChannelID Identifier for channel for which to get restart flag.
* \return Restart flag value.
*/

NS_IMETHODIMP sbTagLibChannelFileIOManager::GetChannelRestart(
    const nsACString            &aChannelID,
    PRBool                      *_retval)
{
    sbTagLibChannelFileIOManager::Channel   *pChannel;
    PRBool                                  restart;
    nsresult                                rv;

    /* Validate parameters. */
    NS_ENSURE_FALSE(aChannelID.IsEmpty(), NS_ERROR_INVALID_ARG);
    NS_ENSURE_ARG_POINTER(_retval);

    /* Get the channel restart flag. */
    rv = GetChannel(aChannelID, &pChannel);
    NS_ENSURE_SUCCESS(rv, rv);
    restart = pChannel->restart;

    /* Return results. */
    *_retval = restart;

    return (NS_OK);
}


/**
* \brief Set the restart flag for the channel.
*
* \param aChannelID Identifier for channel for which to set restart flag.
* \param aRestart Restart flag value.
*/

NS_IMETHODIMP sbTagLibChannelFileIOManager::SetChannelRestart(
    const nsACString            &aChannelID,
    PRBool                      aRestart)
{
    sbTagLibChannelFileIOManager::Channel   *pChannel;
    nsresult                                rv;

    /* Validate parameters. */
    NS_ENSURE_FALSE(aChannelID.IsEmpty(), NS_ERROR_INVALID_ARG);

    /* Set the channel restart flag. */
    rv = GetChannel(aChannelID, &pChannel);
    NS_ENSURE_SUCCESS(rv, rv);
    pChannel->restart = aRestart;

    return (NS_OK);
}


/*******************************************************************************
 *
 * Public TagLib sbISeekableChannel file I/O manager services.
 *
 ******************************************************************************/

/*
 * sbTagLibChannelFileIOManager
 *
 *   This function constructs a TagLib sbISeekable file I/O manager object.
 */

sbTagLibChannelFileIOManager::sbTagLibChannelFileIOManager()
{
}


/*
 * ~TagLibChannelFileIO
 *
 *   This function is the destructor for TagLib sbISeekable file I/O manager
 * objects.
 */

sbTagLibChannelFileIOManager::~sbTagLibChannelFileIOManager()
{
}


/*
 * FactoryInit
 *
 *   This function is called by the component factory to initialize the
 * component.
 */

nsresult sbTagLibChannelFileIOManager::FactoryInit()
{
    PRBool                          success;

    /* Initialize the channel map. */
    success = mChannelMap.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    return (NS_OK);
}


/*******************************************************************************
 *
 * Private TagLib sbISeekableChannel file I/O manager services.
 *
 ******************************************************************************/

/*
 * Channel
 *
 *   This function constructs a TagLib sbISeekable file I/O manager channel
 * object.
 */

sbTagLibChannelFileIOManager::Channel::Channel(
    nsCOMPtr<sbISeekableChannel>    apSeekableChannel)
:
    pSeekableChannel(apSeekableChannel),
    restart(PR_FALSE)
{
    MOZ_COUNT_CTOR(sbTagLibChannelFileIOManager::Channel);
    TRACE(("sbTagLibChannelFileIOManager::Channel[0x%.8x] - ctor", this));
}


/*
 * ~Channel
 *
 *   This function is the destructor for TagLib sbISeekable file I/O manager
 * channel objects.
 */

sbTagLibChannelFileIOManager::Channel::~Channel()
{
    MOZ_COUNT_DTOR(sbTagLibChannelFileIOManager::Channel);
    TRACE(("sbTagLibChannelFileIOManager::Channel[0x%.8x] - dtor", this));
}


/*
 * GetChannel
 *
 *   --> aChannelID             Channel identifier.
 *   <-- appChannel             TagLib channel.
 *
 *   <-- NS_ERROR_NOT_AVAILABLE No channel with the specified identifier is
 *                              available.
 *
 *   This function returns in appChannel a TagLib channel corresponding to the
 * channel identifier specified by aChannelID.  If no channels can be found,
 * this function returns NS_ERROR_NOT_AVAILABLE.
 */

nsresult sbTagLibChannelFileIOManager::GetChannel(
    const nsACString            &aChannelID,
    sbTagLibChannelFileIOManager::Channel
                                **appChannel)
{
    sbTagLibChannelFileIOManager::Channel   *pChannel;
    PRBool                                  success;

    /* Validate parameters. */
    NS_ASSERTION(!aChannelID.IsEmpty(), "aChannelID is empty");
    NS_ASSERTION(appChannel, "appChannel is null");

    /* Get the channel from the channel map. */
    success = mChannelMap.Get(aChannelID, &pChannel);
    if (!success)
        return (NS_ERROR_NOT_AVAILABLE);

    /* Return results. */
    *appChannel = pChannel;

    return (NS_OK);
}
