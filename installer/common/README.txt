SONGBIRD 0.4RC2 README
---------------------------------------------------------------------------------
NOTICE TO LEGACY (0.2.5 and older) USERS
Songbird 0.3 and Later requires a new library format and profiles.  As such, you 
won't be able to migrate your old 0.2.5 libraries and profiles to your new Songbird 
0.3 or Later installation.  It is possible to install both Songbird 0.2.5 and 0.3 
or Later side-by-side to allow you to continue to access your old libraries and 
profiles while you setup your new Songbird 0.3 or Later installation.Please see the 
following document for more information on how to do this, and for links to download 
prior Songbird 0.2.5 binaries for your system: 
http://www.songbirdnest.com/migration-from-0.2.5-to-0.3 

---------------------------------------------------------------------------------
NOTICE TO iPOD USERS
* iPOD Device Support version 2.1.2 or Later is needed for Songbird 0.4RC1.  Previous 
versions of this add-on are not compatible with this new release.
* Linux Fedora users will need to upgrade to libdbus version 3 to get iPOD 
to sync OR wait for version 2.1.5.

---------------------------------------------------------------------------------

DOCUMENTATION
The following development documentation is available:

* Display Panes Documentation (NEW!)
http://www.songbirdnest.com/add-on-api/articles/display-panes/

* Webpage API Documentation
Starter Guides and Sample Code - http://developer.songbirdnest.com/webpage-api/
Full Webpage API Documentation - http://developer.songbirdnest.com/webpage-api/docs/0.3/

* Add-on API Documentation</l
Starter Guides and Sample Code - http://developer.songbirdnest.com/add-on-api/
Full Add-on API Documentation - http://developer.songbirdnest.com/add-on-api/docs/0.3/

---------------------------------------------------------------------------------
NEW FEATURES AND IMPROVEMENTS
The following are new features and improvements for the 0.4RC1 release:

* Display panes that make it friendly for multiple add-ons to be installed in Songbird 
at once by allowing the user to define which add-on gets displayed by context. Read the 
details on these integration points at: http://www.songbirdnest.com/add-on-api/articles/display-panes/
* The latest generation iPods (the iPod touch/iPhone an exception) are now supported and we 
have improved the user experience for the iPod including a new iPod Summary Page.  Requires 
latest iPod add-on - http://developer.songbirdnest.com/nightly/addons/
* Library support to return to the web location where a media item originated.
* Bug free preferences/options.
* List views now retain users sorting preferences and column reordering.
* The popular "JumpTo" feature is back!
* Faster, smoother window resize.
* Many memory leaks fixed.

---------------------------------------------------------------------------------
KNOWN BUGS/ISSUES
The following high profile bugs are known to still be open for this release:

All Platforms
* 'Edit' menu items not hooked up (2116)
* Index column width can be shrunk to cut off index values (4571)
* Audioscrobbler add-on not updated to support this release (5193)
* Songs with special characters are not sorted correctly (5373)
* Need to change the HTTP accept-language tag to match locale (5437)
* [iTunes Importer] Scanning for media duplicates track entries (5465)
   Fixed with version 3.0.10 of the importer.
* [iPod] Need to support the iPod Touch/iPhone (6389)

Linux Specific

* No way to have system-wide install (can only install to a single user) (3679)
* Service pane context menu requires right click + hold (4584)
* [Linux Fedora 32 and 64 bit] Cannot mount iPOD (6200)
   Fixed with version 2.1.5 of the iPod Device Support add-on.

MacOSX Specific

* Clicking Pause/Play on the main pane can sometimes take several seconds to respond (4137)
* Service pane context menu requires right click + hold (4584)
* "Cheezy Video Window" cannot be hidden or resized (4706)
* AppleKey + Q Doesn't always Quit (4845)
* Most keyboard shortcuts unavailable on OS X (5409)
---------------------------------------------------------------------------------

For more details, visit Bugzilla at: http://bugzilla.songbirdnest.com
          