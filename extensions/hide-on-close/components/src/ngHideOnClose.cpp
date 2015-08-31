/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2014
 * http://www.getnightingale.com
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

#include "ngHideOnClose.h"

static GtkWindow *playerGtkWindow = NULL;

void onRaise(GtkWidget *window, gpointer data)
{   
    gdk_window_show(((GtkWidget*) playerGtkWindow)->window);
}

void checkWindowTitle(gpointer data, gpointer user_data)
{
    const gchar *title = gtk_window_get_title((GtkWindow*) data);

    if (playerGtkWindow || g_strcmp0(title, (char*) user_data)) return;

    playerGtkWindow = (GtkWindow*) data;
}

NS_IMPL_ISUPPORTS1 (ngHideOnClose, ngIHideOnClose)

ngHideOnClose::ngHideOnClose()
{
}

ngHideOnClose::~ngHideOnClose()
{

}

NS_IMETHODIMP ngHideOnClose::InitializeFor(const char* desktopFileName, const char* windowTitle)
{
    if (!desktopFileName || !windowTitle) return NS_ERROR_INVALID_ARG;

    GList* wlist = gtk_window_list_toplevels();
    g_list_foreach(wlist, checkWindowTitle, (gpointer) windowTitle);
    g_list_free(wlist);

    if (!playerGtkWindow) return NS_ERROR_INVALID_ARG;

    return NS_OK;
}

NS_IMETHODIMP ngHideOnClose::HideWindow()
{
    gdk_window_hide(((GtkWidget*) playerGtkWindow)->window);

    return NS_OK;
}

NS_IMETHODIMP ngHideOnClose::ShowWindow()
{
    gdk_window_show(((GtkWidget*) playerGtkWindow)->window);

    return NS_OK;
}
