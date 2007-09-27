----------------------------------
XPCOM Hello World Sample Extension
----------------------------------

Description
-----------

This code demonstrates using the Songbird build system
to bundle C++ XPCOM components in an extension.


How to Build
------------

Copy everything to trunk/extras/xpcom_helloworld in your
Songbird tree.  See http://publicsvn.songbirdnest.com for
information on getting and building the Songbird source code.

First build Songbird:
  make -f songbird.mk clobber
  make -f songbird.mk debug

Then build the extension:
  make -C compiled/extras/xpcom_helloworld

In debug mode the extension and a simple unit test will
automatically be installed. In release mode the extension
will be packaged as 
  compiled/extras/xpcom_helloworld/xpcom_helloworld.xpi


Additional Resources
--------------------
 * http://developer.mozilla.org/en/docs/XPCOM
 * http://developer.mozilla.org/en/docs/Creating_XPCOM_Components
 * http://www.ibm.com/developerworks/webservices/library/co-xpcom.html


TODO
----
 * Demonstrate an XPCOM Service (Singleton)
 * Demonstrate a Javascript component
 * Demonstrate loading an external library
 * Demonstrate interacting with Songbird components
