-------------------------------
XPCOM Hello World Sample Add-on
-------------------------------

Description
-----------

This code demonstrates using the Songbird build system
to bundle C++ XPCOM components in an add-on.


Instructions
------------

Follow the directions on http://publicsvn.songbirdnest.com
to get and build the Songbird source code.

In release builds (make -f songbird.mk) the add-on
is packaged as
  compiled/extensions/xpcom_helloworld/xpcom_helloworld.xpi 

In debug builds (make -f songbird.mk debug) the add-on is 
installed into compiled/dist/extensions, and a simple 
unit test is copied to compiled/dist/testharness

To run the unit test: 
    cd compiled/dist
    ./Songbird -test xpcom_helloworld

To recompile the add-on:
  make -C compiled/extensions/xpcom_helloworld


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
