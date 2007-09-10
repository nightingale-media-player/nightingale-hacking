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
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsIDialogParamBlock.h>
#include <nsIIOService.h>
#include <nsIObserverService.h>
#include <nsIPrefService.h>
#include <nsIStringBundle.h>
#include <prmon.h>

/* Songbird imports. */
#include <sbDownloadButtonPropertyInfo.h>
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

class sbDownloadDevice : public nsIObserver,
                         public sbIDownloadDevice,
                         public sbDeviceBase,
                         public sbIMediaListListener
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
    NS_DECL_NSIOBSERVER
    NS_DECL_SBIDEVICEBASE
    NS_DECL_SBIDOWNLOADDEVICE
    NS_DECL_SBIMEDIALISTLISTENER


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
     * mpDownloadMediaList      Download device medialist.
     * mpDeviceLibraryListener  Songbird device library listener.
     * mpMainLibrary            Main library.
     * mpWebLibrary             Web library.
     * mpPrefBranch             Preference branch.
     * mpIOService              I/O service.
     * mpStringBundle           Download device string bundle.
     * mpDownloadDirDR          Default download directory data remote.
     * mpTmpDownloadDir         Temporary download directory.
     * mpDownloadSession        Current download session.
     * mpDeviceLock             Lock for download device access.
     * mState                   Current device state.
     */

    nsCOMPtr<sbIMediaList>      mpDownloadMediaList;
    nsRefPtr<sbDeviceBaseLibraryListener>
                                mpDeviceLibraryListener;
    nsCOMPtr<sbILibrary>        mpMainLibrary;
    nsCOMPtr<sbILibrary>        mpWebLibrary;
    nsCOMPtr<nsIPrefBranch>     mpPrefBranch;
    nsCOMPtr<nsIIOService>      mpIOService;
    nsCOMPtr<nsIStringBundle>   mpStringBundle;
    nsCOMPtr<sbIDataRemote>     mpDownloadDirDR;
    nsCOMPtr<nsIFile>           mpTmpDownloadDir;
    nsRefPtr<sbDownloadSession> mpDownloadSession;
    PRMonitor                   *mpDeviceMonitor;
    PRUint32                    mState;


    /*
     * Private media list services.
     */

    nsresult InitializeDownloadMediaList();

    void FinalizeDownloadMediaList();

    nsresult CreateDownloadMediaList();

    void GetDownloadMediaList();


    /*
     * Private transfer services.
     */

    nsresult RunTransferQueue();

    PRBool GetNextTransferItem(
        sbIMediaItem                **appMediaItem);

    nsresult ResumeTransfers();

    nsresult ClearCompletedItems();

    nsresult SetTransferDestination(
        nsCOMPtr<sbIMediaItem>      pMediaItem);

    nsresult CancelSession();

    void SessionCompleted(
        sbDownloadSession           *pDownloadSession);


    /*
     * Private services.
     */

    nsresult GetTmpFile(
        nsIFile                     **ppTmpFile);

    static nsresult MakeFileUnique(
        nsIFile                     *apFile);

    nsresult QueryUserForDestination(
        PRBool                      *apCancelDownload,
        nsAString                   &aDstDir);

    nsresult OpenDialog(
        char                        *aChromeURL,
        nsIDialogParamBlock         *apDialogPB);

    static nsresult GetStatusTarget(
        sbIMediaItem                *apMediaItem,
        sbIMediaItem                **apStatusTarget);
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
#include <nsIHttpChannel.h>
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

class sbDownloadSession : public nsIWebProgressListener
{
    /* *************************************************************************
     *
     * Public interface.
     *
     **************************************************************************/

    public:

    /*
     * mpMediaItem              Media item being downloaded.
     */

    nsCOMPtr<sbIMediaItem>      mpMediaItem;


    /*
     * Inherited interfaces.
     */

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER

    /*
     * Public download session services.
     */

    sbDownloadSession(
        sbDownloadDevice            *pDownloadDevice,
        sbIMediaItem                *pMediaItem);

    virtual ~sbDownloadSession();

    nsresult Initiate();

    nsresult Suspend();

    nsresult Resume();

    void Shutdown();


    /* *************************************************************************
     *
     * Private interface.
     *
     **************************************************************************/

    private:

    /*
     * mpSessionLock            Lock for session.
     * mpDownloadDevice         Managing download device.
     * mpIOService              I/O service.
     * mpFileProtocolHandler    File protocol handler.
     * mpWebBrowser             Web browser used for download.
     * mpChannel                Download channel.
     * mpTmpFile                Temporary download file.
     * mpDstLibrary             Destination library.
     * mpDstFile                Destination download file.
     * mpDstURI                 Destination download URI.
     * mShutdown                True if session has been shut down.
     * mSuspended               True if session is suspended.
     */

    PRLock                      *mpSessionLock;
    sbDownloadDevice            *mpDownloadDevice;
    nsCOMPtr<nsIIOService>      mpIOService;
    nsCOMPtr<nsIWebBrowserPersist>
                                mpWebBrowser;
    nsCOMPtr<nsIRequest>        mpRequest;
    nsCOMPtr<nsIFile>           mpTmpFile;
    nsCOMPtr<sbILibrary>        mpDstLibrary;
    nsCOMPtr<nsIFile>           mpDstFile;
    nsCOMPtr<nsIURI>            mpDstURI;
    nsCOMPtr<sbIMediaItem>      mpStatusTarget;
    PRBool                      mShutdown;
    PRBool                      mSuspended;


    /*
     * Private download session services.
     */

    nsresult CompleteTransfer(nsIRequest* aRequest);

    nsresult UpdateDstLibraryMetadata();

    void UpdateProgress(
        PRUint64                    aProgress,
        PRUint64                    aProgressMax);

    /* *************************************************************************
     *
     * Library metadata updater class.
     *
     **************************************************************************/

    class LibraryMetadataUpdater : public sbIMediaListEnumerationListener
    {
        /*
         * Public interface.
         */

        public:

        NS_DECL_ISUPPORTS
        NS_DECL_SBIMEDIALISTENUMERATIONLISTENER


        /*
         * Private interface.
         */

        private:

        nsCOMPtr<nsIMutableArray>   mpMediaItemArray;
    };


    /* *************************************************************************
     *
     * Web library updater class.
     *
     **************************************************************************/

    class WebLibraryUpdater : public sbIMediaListEnumerationListener
    {
        /*
         * Public interface.
         */

        public:

        NS_DECL_ISUPPORTS
        NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

        WebLibraryUpdater(
            sbDownloadSession           *pDownloadSession)
        :
            mpDownloadSession(pDownloadSession)
        {
        }


        /*
         * Private interface.
         */

        private:

        sbDownloadSession           *mpDownloadSession;
    };
};

class sbAutoDownloadButtonPropertyValue
{
public:
  sbAutoDownloadButtonPropertyValue(sbIMediaItem* aMediaItem,
                                    sbIMediaItem* aStatusTarget = nsnull,
                                    PRBool aReadOnly = PR_FALSE);
  ~sbAutoDownloadButtonPropertyValue();

  nsAutoPtr<sbDownloadButtonPropertyValue> value;

private:
  nsCOMPtr<sbIMediaItem> mMediaItem;
  nsCOMPtr<sbIMediaItem> mStatusTarget;
  PRBool mReadOnly;
};

#endif // __DOWNLOAD_DEVICE_H__
