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

#ifndef __DOWNLOAD_DEVICE_H__
#define __DOWNLOAD_DEVICE_H__

/* *****************************************************************************
 *******************************************************************************
 *
 * Download device.
 *
 *******************************************************************************
 ******************************************************************************/

/** 
* \file  DownloadDevice.h
* \brief Songbird DownloadDevice Component Definition.
*/

/* *****************************************************************************
 *
 * Download device configuration.
 *
 ******************************************************************************/

/*
 * Download device XPCOM component definitions.
 */

#define SONGBIRD_DownloadDevice_CONTRACTID                                     \
                            "@songbirdnest.com/Songbird/Device/DownloadDevice;1"
#define SONGBIRD_DownloadDevice_CLASSNAME "Songbird Download Device"
#define SONGBIRD_DownloadDevice_CID                                            \
{                                                                              \
    0x961DA3F4,                                                                \
    0x5EF1,                                                                    \
    0x4AD0,                                                                    \
    { 0x81, 0x8d, 0x62, 0x2C, 0x7B, 0xD1, 0x74, 0x47}                          \
}


/* *****************************************************************************
 *
 * Download device imported services.
 *
 ******************************************************************************/

/* Local imports. */
#include <DeviceBase.h>
#include <sbIDownloadDevice.h>

/* Mozilla imports. */
#include <nsCOMPtr.h>
#include <nsIIOService.h>

/* Songbird imports. */
#include <sbIDataRemote.h>
#include <sbILibrary.h>
#include <sbIMetadataJobManager.h>


/* *****************************************************************************
 *
 * Download device classes.
 *
 ******************************************************************************/

/*
 * sbDownloadDevice class.
 */

class sbDownloadSession;

class sbDownloadDevice : public sbIDownloadDevice, public sbDeviceBase
{
    /* *************************************************************************
     *
     * Friends.
     *
     **************************************************************************/

    friend class sbDownloadSession;


    /* *************************************************************************
     *
     * Public interface.
     *
     **************************************************************************/

    public:

    /*
     * Inherited interfaces.
     */

    NS_DECL_ISUPPORTS
    NS_DECL_SBIDEVICEBASE
    NS_DECL_SBIDOWNLOADDEVICE


    /*
     * Public download device services.
     */

    sbDownloadDevice();

    virtual ~sbDownloadDevice();


    /* *************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * mpDownloadLibrary        Download device library.
     * mpIOService              I/O service.
     * mpMetadataJobManager     Metadata job manager.
     * mpDownloadDirDR          Default download directory data remote.
     * mpTmpDownloadDir         Temporary download directory.
     * mpDownloadSession        Current download session.
     * mBusy                    Non-zero if the transfer queue is busy.
     */

    nsCOMPtr<sbILibrary>        mpDownloadLibrary;
    nsCOMPtr<nsIIOService>      mpIOService;
    nsCOMPtr<sbIMetadataJobManager>
                                mpMetadataJobManager;
    nsCOMPtr<sbIDataRemote>     mpDownloadDirDR;
    nsCOMPtr<nsIFile>           mpTmpDownloadDir;
    sbDownloadSession           *mpDownloadSession;
    PRInt32                     mBusy;


    /*
     * Private transfer services.
     */

    nsresult RunTransferQueue();

    nsresult ResumeTransfers();

    nsresult ClearCompletedItems();

    nsresult SetTransferDestination(
        nsCOMPtr<sbIMediaItem>      pMediaItem);

    void SessionCompleted(
        sbDownloadSession           *pDownloadSession);


    /*
     * Private services.
     */

    nsresult GetTmpFile(
        nsIFile                     **ppTmpFile);
};


/* *****************************************************************************
 *******************************************************************************
 *
 * Download session.
 *
 *******************************************************************************
 ******************************************************************************/

/* *****************************************************************************
 *
 * Download session imported services.
 *
 ******************************************************************************/

/* Mozilla imports. */
#include <nsIFileProtocolHandler.h>
#include <nsIHttpEventSink.h>
#include <nsIInterfaceRequestor.h>
#include <nsIProgressEventSink.h>
#include <nsIWebBrowserPersist.h>
#include <nsIWebProgressListener.h>


/* *****************************************************************************
 *
 * Download session classes.
 *
 ******************************************************************************/

/*
 * sbDownloadSession class.
 */

class sbDownloadSession : public nsIWebProgressListener,
                          public nsIInterfaceRequestor,
                          public nsIProgressEventSink,
                          public nsIHttpEventSink
{
    /* *************************************************************************
     *
     * Public interface.
     *
     **************************************************************************/

    public:

    /*
     * Inherited interfaces.
     */

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER
    NS_DECL_NSIINTERFACEREQUESTOR
    NS_DECL_NSIPROGRESSEVENTSINK
    NS_DECL_NSIHTTPEVENTSINK


    /*
     * Public download session services.
     */

    sbDownloadSession(
        sbDownloadDevice            *pDownloadDevice,
        sbIMediaItem                *pMediaItem);

    virtual ~sbDownloadSession();

    nsresult Initiate();

    void Shutdown();


    /* *************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * mpDownloadDevice         Managing download device.
     * mpMediaItem              Media item being downloaded.
     * mpIOService              I/O service.
     * mpFileProtocolHandler    File protocol handler.
     * mpWebBrowser             Web browser used for download.
     * mpTmpFile                Temporary download file.
     * mpDstLibrary             Destination library.
     * mpDstFile                Destination download file.
     * mpDstURI                 Destination download URI.
     * mShutdown                True if session has been shut down.
     */

    sbDownloadDevice            *mpDownloadDevice;
    nsCOMPtr<sbIMediaItem>      mpMediaItem;
    nsCOMPtr<nsIIOService>      mpIOService;
    nsCOMPtr<nsIFileProtocolHandler>
                                mpFileProtocolHandler;
    nsCOMPtr<nsIWebBrowserPersist>
                                mpWebBrowser;
    nsCOMPtr<nsIFile>           mpTmpFile;
    nsCOMPtr<sbILibrary>        mpDstLibrary;
    nsCOMPtr<nsIFile>           mpDstFile;
    nsCOMPtr<nsIURI>            mpDstURI;
    PRBool                      mShutdown;


    /*
     * Private download session services.
     */

    nsresult CompleteTransfer();
};


#endif // __DOWNLOAD_DEVICE_H__
