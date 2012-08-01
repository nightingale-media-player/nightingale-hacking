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

#ifndef __TAGLIB_CHANNEL_FILE_IO_MANAGER_H__
#define __TAGLIB_CHANNEL_FILE_IO_MANAGER_H__

/*******************************************************************************
 *******************************************************************************
 *
 * TagLib sbISeekableChannel file IO manager.
 *
 *******************************************************************************
 ******************************************************************************/

/**
* \file  TagLibChannelFileIOManager.h
* \brief Songbird TagLib sbISeekableChannel file IO manager component.
*/

/*******************************************************************************
 *
 * TagLib sbISeekableChannel file IO manager configuration.
 *
 ******************************************************************************/

/*
 * TagLib sbISeekableChannel file IO manager XPCOM component definitions.
 */

#define SONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CONTRACTID                         \
                    "@songbirdnest.com/Songbird/sbTagLibChannelFileIOManager;1"
#define SONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CLASSNAME                          \
                "Songbird TagLib sbISeekableChannel file IO Manager Component"
#define SONGBIRD_TAGLIBCHANNELFILEIOMANAGER_CID                                \
{                                                                              \
    0xED17756D,                                                                \
    0xDEAF,                                                                    \
    0x437E,                                                                    \
    { 0x8E, 0x51, 0xAD, 0x6D, 0x1C, 0x63, 0xB7, 0x14 }                         \
}


/*******************************************************************************
 *
 * TagLib sbISeekableChannel file IO manager imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include <TaglibChannelFileIO.h>
#include <sbITagLibChannelFileIOManager.h>

/* Songbird imports. */
#include <nsClassHashtable.h>
#include <sbISeekableChannel.h>

/* Mozilla imports. */
#include <nsCOMPtr.h>
#include <nsStringGlue.h>

/* TagLib imports. */
#include <tfile.h>

using namespace TagLib;


/*******************************************************************************
 *
 * TagLib sbISeekableChannel file IO manager classes.
 *
 ******************************************************************************/


/*
 * sbTagLibChannelFileIOManager class
 */

class sbTagLibChannelFileIOManager : public sbITagLibChannelFileIOManager
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
    NS_DECL_SBITAGLIBCHANNELFILEIOMANAGER


    /*
     * Public services.
     */

    sbTagLibChannelFileIOManager();

    virtual ~sbTagLibChannelFileIOManager();

    nsresult FactoryInit();


    /***************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * Private TagLib sbISeekableChannel file I/O internal classes.
     */

    class Channel
    {
        /*
         * pSeekableChannel         Seekable channel component.
         * restart                  Flag indicating that the channel needs to be
         *                          restarted.
         */

        public:

        Channel(
            nsCOMPtr<sbISeekableChannel>    apSeekableChannel);

        virtual ~Channel();

        nsCOMPtr<sbISeekableChannel>
                                    pSeekableChannel;
        PRBool                      restart;
    };


    /*
     * Private TagLib sbISeekableChannel file I/O class services.
     */

    nsresult GetChannel(
        const nsACString            &aChannelID,
        sbTagLibChannelFileIOManager::Channel
                                    **appChannel);


    /*
     * Private TagLib sbISeekableChannel file I/O class services.
     *
     *   mChannelMap            TagLib channel map.
     *   mpResolver             sbISeekableChannel file I/O type resolver.
     */

private:
    nsClassHashtableMT<nsCStringHashKey,
                       sbTagLibChannelFileIOManager::Channel> mChannelMap;
};


#endif /* __TAGLIB_CHANNEL_FILE_IO_MANAGER_H__ */
