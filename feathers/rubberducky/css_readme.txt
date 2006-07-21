= Songbird CSS Architecture =
Songbird's CSS is made to make feathering easier.  At the top level, we make a distinction between styles for the Main Player, the Mini Player, and the Dialog Boxes. From there we split up styles into 3 cascading layers: Size & Position, Color and Imagery. I've also excerpted out the "-moz-appearance: none" style into a base style sheet so our potential featherers wouldn't have to worry about the default -moz styles affecting their work as it did ours.  

== Size & Position ==
Move it around, test it out, move it around. These style sheets include margins, padding, -moz-styles, size, alignment, etc. If it's not a color or an image or "-moz-appearance: none", it belongs in the Size & Position style sheet. For the Main Window, this is mainwin.css, for the mini this is mini.css, and for everything else, dialog.css.

== Color ==
Everything that can be colored in Songbird is colored in one stylesheet. If you want to customize your Songbird with a new paint scheme: see mainwin_colors.css for the Main window, see mini_colors.css for the Mini Player, and dialog_colors.css for everything else.

== Images ==
All images used in rendering Songbird can be found in one stylesheet. For the Main window, this is mainwin_images.css. Dialogs have only 2 images for check boxes so we've left those in dialog_color.css and saved making another css file for dialogs.

== Songbird's 8 stylesheets ==
1. Songbird.css - This style sheet @imports the other stylesheets.

2. base_styles.css

3. mainwin.css

4. mainwin_colors.css

5. mainwin_images.css

6. dialog.css

7. dialog_colors.css

8. mini.css


