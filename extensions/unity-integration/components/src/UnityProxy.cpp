#include "UnityProxy.h"
#include <nsCOMPtr.h>
#include <nsIObserverService.h>
#include <nsServiceManagerUtils.h>

static nsCOMPtr<nsIObserverService> observerService = NULL;
static UnityMusicPlayer *musicPlayer = NULL;
static UnityTrackMetadata *trackMetadata = NULL;
static GtkWindow *playerGtkWindow = NULL;
static gchar *playerName = NULL;
static gchar *playerIcon = NULL;
static gboolean lockedFromSoundMenu = false;

static gboolean enableNotify = true;

void onPlayPause (GtkWidget *window,
                   gpointer data)
{
    UnityPlaybackState playbackState = unity_music_player_get_playback_state (musicPlayer);
    if (playbackState == UNITY_PLAYBACK_STATE_PLAYING) {
        observerService->NotifyObservers (NULL, "sound-menu-pause", NULL);
    }
    else {
        observerService->NotifyObservers (NULL, "sound-menu-play", NULL);
    }
}

void onNext (GtkWidget *window,
              gpointer data)
{
    lockedFromSoundMenu = true;
    observerService->NotifyObservers (NULL, "sound-menu-next", NULL);
}

void onPrevious (GtkWidget *window,
                  gpointer data)
{
    lockedFromSoundMenu = true;
    observerService->NotifyObservers (NULL, "sound-menu-previous", NULL);
}

void onRaise (GtkWidget *window,
               gpointer data)
{   
    gdk_window_show( ((GtkWidget*) playerGtkWindow) -> window );
}

void checkWindowTitle (gpointer data, 
                       gpointer user_data)
{
    const gchar *title = gtk_window_get_title ((GtkWindow*) data);

    if (playerGtkWindow || g_strcmp0 (title, (char*) user_data)) return;

    playerGtkWindow = (GtkWindow*) data;
}

NS_IMPL_ISUPPORTS1 (UnityProxy, IUnityProxy)

UnityProxy::UnityProxy ()
{
    g_message("Unity Integration: loading");

    observerService = do_GetService ("@mozilla.org/observer-service;1");
}

UnityProxy::~UnityProxy ()
{
    unity_music_player_unexport (musicPlayer);
}

NS_IMETHODIMP UnityProxy::InitializeFor (const char* desktopFileName, const char* windowTitle)
{
    g_message("Unity Integration: checking for unity components");
    if (!isUnityRunning()) return NS_ERROR_NOT_INITIALIZED;

    g_message("Unity Integration: found them!");
    trackMetadata = unity_track_metadata_new ();

    if (!desktopFileName || !windowTitle) return NS_ERROR_INVALID_ARG;

    GList* wlist = gtk_window_list_toplevels ();
    g_list_foreach (wlist, checkWindowTitle, (gpointer) windowTitle);
    g_list_free (wlist);

    if (!playerGtkWindow) return NS_ERROR_INVALID_ARG;

    GKeyFile *keyFile = g_key_file_new ();
    gchar *desktopFilePath = g_build_filename ("/usr/share/applications", desktopFileName, NULL);
    if (g_key_file_load_from_file (keyFile, desktopFilePath, G_KEY_FILE_NONE, NULL))
    {
        playerName = g_key_file_get_string (keyFile, "Desktop Entry", "Name", NULL);
        playerIcon = g_key_file_get_string (keyFile, "Desktop Entry", "Icon", NULL);
    }
    g_key_file_free (keyFile);

    if (!playerName || !playerIcon) return NS_ERROR_UNEXPECTED;

    musicPlayer = unity_music_player_new (desktopFileName);
    if (!musicPlayer) return NS_ERROR_INVALID_ARG;

    unity_music_player_set_title (musicPlayer, playerName);
    unity_music_player_export (musicPlayer);

    unity_music_player_set_can_go_next (musicPlayer, false);
    unity_music_player_set_can_play (musicPlayer, true);

    g_signal_connect (G_OBJECT (musicPlayer), "play_pause", G_CALLBACK (onPlayPause), NULL);
    g_signal_connect (G_OBJECT (musicPlayer), "next", G_CALLBACK (onNext), NULL);
    g_signal_connect (G_OBJECT (musicPlayer), "previous", G_CALLBACK (onPrevious), NULL);
    g_signal_connect (G_OBJECT (musicPlayer), "raise", G_CALLBACK (onRaise), NULL);

    return NS_OK;
}

NS_IMETHODIMP UnityProxy::SoundMenuSetTrackInfo (const char *title, const char *artist, const char *album, const char *coverFilePath)
{
    if (!musicPlayer) return NS_ERROR_NOT_INITIALIZED;

    unity_track_metadata_set_artist (trackMetadata, artist);
    unity_track_metadata_set_album (trackMetadata, album);
    unity_track_metadata_set_title (trackMetadata, title);

    GFile *coverFile = g_file_new_for_path (coverFilePath);
    unity_track_metadata_set_art_location (trackMetadata, coverFile);

    unity_music_player_set_current_track (musicPlayer, trackMetadata);

    g_object_unref (coverFile);

    return NS_OK;
}

NS_IMETHODIMP UnityProxy::SoundMenuSetPlayingState (PRInt16 playing)
{
    if (!musicPlayer) return NS_ERROR_NOT_INITIALIZED;

    if (playing < 0) {
        unity_music_player_set_playback_state (musicPlayer, UNITY_PLAYBACK_STATE_PAUSED);
        unity_music_player_set_current_track (musicPlayer, NULL);
    }
    else if (!!playing) {
        unity_music_player_set_playback_state (musicPlayer, UNITY_PLAYBACK_STATE_PLAYING);
    }
    else {
        unity_music_player_set_playback_state (musicPlayer, UNITY_PLAYBACK_STATE_PAUSED);
    }

    return NS_OK;
}

NS_IMETHODIMP UnityProxy::HideWindow ()
{
    if (!musicPlayer) return NS_ERROR_NOT_INITIALIZED;

    gdk_window_hide ( ((GtkWidget*) playerGtkWindow) -> window );

    return NS_OK;
}

NS_IMETHODIMP UnityProxy::ShowWindow ()
{
    if (!musicPlayer) return NS_ERROR_NOT_INITIALIZED;

    gdk_window_show ( ((GtkWidget*) playerGtkWindow) -> window );

    return NS_OK;
}

NS_IMETHODIMP UnityProxy::EnableNotifications (PRBool needEnable)
{
    enableNotify = !!needEnable;

    return NS_OK;
}
