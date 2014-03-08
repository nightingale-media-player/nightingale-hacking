/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#include "lnNotifs.h"
#include <gtk/gtk.h>
#include <libnotify/notify.h>


static GtkWindow *playerGtkWindow = NULL;
static gchar *playerIcon = NULL;


/* Used to get a handle on the window so notifications don't
 * fire while the window is already on the top-level
 */
void checkWindowTitle(gpointer data, gpointer user_data)
{
    const gchar *title = gtk_window_get_title((GtkWindow*) data);
    if (playerGtkWindow || g_strcmp0(title, (char*) user_data)) return;
    playerGtkWindow = (GtkWindow*) data;
}


NS_IMPL_ISUPPORTS1(lnNotifs, ILNNotifs)


lnNotifs::lnNotifs()
{
    /* member initializers and constructor code */
    g_message("lnNotifs: constructor");
    notifsEnabled = true;
    notification = NULL;
    playerGtkWindow = NULL;
}


lnNotifs::~lnNotifs()
{
    /* destructor code */
    notify_uninit();
}


/* void InitNotifs(); */
NS_IMETHODIMP lnNotifs::InitNotifs(const char *windowTitle)
{
    g_message("lnNotifs: Initializing");
    // Get handle to window
    GList* wlist = gtk_window_list_toplevels();
    g_list_foreach(wlist, checkWindowTitle, (gpointer) windowTitle);
    g_list_free(wlist);
    if (!playerGtkWindow) return NS_ERROR_INVALID_ARG;

    if (!notify_init("nightingale"))
        return NS_ERROR_NOT_INITIALIZED;
    
    GKeyFile *keyFile = g_key_file_new ();

    gchar *desktopFilePath = g_build_filename ("/usr/share/applications", "nightingale.desktop", NULL);

    if (g_key_file_load_from_file (keyFile, desktopFilePath, G_KEY_FILE_NONE, NULL))
    {
        playerIcon = g_key_file_get_string (keyFile, "Desktop Entry", "Icon", NULL);
    }

    if(!playerIcon) return NS_ERROR_UNEXPECTED;

    notification = notify_notification_new("", "", playerIcon);
    if (!notification) return NS_ERROR_NOT_INITIALIZED;

    return NS_OK;
}


/* void TrackChangeNotify(in string title,
 *                        in string artist,
 *                        in string album,
 *                        in string coverFilePath,
 *                        in PRInt32 timeout);
 */
NS_IMETHODIMP lnNotifs::TrackChangeNotify(const char *title,
                                          const char *artist,
                                          const char *album,
                                          const char *coverFilePath,
                                          PRInt32 timeout)
{
    notify_notification_set_timeout(notification, timeout);

    // don't show notifications if the window is active
    if (notifsEnabled && !gtk_window_is_active(playerGtkWindow)) {
        gchar *summary = g_strdup_printf("%s", title);
        gchar *body = g_strdup_printf("%s - %s", artist, album);
        const char *image = (!coverFilePath) ? playerIcon : coverFilePath;

        notify_notification_update(notification, summary, body, image);
        notify_notification_show(notification, NULL);

        g_free(summary);
        g_free(body);
    }

    return NS_OK;
}


/* void EnableNotifications(in PRBool enable); */
NS_IMETHODIMP lnNotifs::EnableNotifications(PRBool enable)
{
    notifsEnabled = !!enable;

    return NS_OK;
}
