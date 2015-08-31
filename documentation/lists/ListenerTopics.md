Listener Topics
===============
This is a list of listener topics in the Remote API. See [sPublicMetadata](@ref sPublicMetadata) for the canonical list.

 - `metadata.artist`
   The name of the artist of the currently playing media.

 - `metadata.title`
   The title of the currently playing media.

 - `metadata.album`
   The album name of the currently playing media.

 - `metadata.genre`
   The genre of the currenly playing media.

 - `metadata.position`
   The current position in the media, as the number of milliseconds offset from
   the start of the media.

 - `metadata.length`
   The total length of the currently playing media.  May be zero for media of
   unknown length (e.g. streams).

 - `metadata.position.str`
   The current position in the current media, as expressed as a string
   (in minutes:seconds).

 - `metadata.length.str`
   The total length of the current media, as expressed as a string
   (in minutes:seconds).

 - `playlist.shuffle`
   A boolean value indicating whether shuffle mode is on.

 - `playlist.shuffle.disabled`
   A boolean value indicating if shuffle mode is disabled. If this is enabled,
  setting `playlist.shuffle` will have no effect.

 - `playlist.repeat`
   An integer value indicating which repeat mode is currently active.  This has a
   value of `0` for no repeat, `1` for repeat one, and `2` for repeat all.

 - `playlist.repeat.disabled`
   A boolean value indicating if repeat mode is disabled. If this is enabled,
   setting `playlist.repeat` will have no effect.

 - `playlist.previous.disabled`
   A boolean value indicating if the sequencer can go back in the playlist.

 - `playlist.next.disabled`
   A boolean value indicating if the sequencer can advance in the playlist.

 - `faceplate.volume`
   The current player volume as an integer between `0` and `255` inclusive.

 - `faceplate.mute`
   A boolean value indicating whether the player is currently muted.

 - `faceplate.playing`
   A boolean value indicating whether the player is currently playing.  Note that
   this will be true even if the player is paused.

 - `faceplate.paused`
   A boolean value indicating whether the player is currently paused.
