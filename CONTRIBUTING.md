Contribute to Nightingale
=========================
You've decided that you want to help out Nightingale by contributing code? This is
exactly the document you should be reading right now. Please read it until the
end before doing anything else. If you're looking to contribute in a different
manner, have a look at http://wiki.getnightingale.com/doku.php?id=contribute.

You are certainly not contributing to one of the
simplest open source projects, however we'll try to make it as easy for you as
possible.

This document will try and guide you step by step through your (hopefully not
too scary) adventure of improving Nightingale.

This is by no means complete, so if it's missing something important, please
tell us (or add it in a pull request)!

Claim the Issue
---------------
Tell everybody that you're working on an issue with a comment. If there's no
issue for the bug you want to fix (reporting bugs is contributing too!) or the
feature you'd like to implement, open an issue for it. You'll need a GitHub
account to do this, and that's the only account you need to contribute to
Nightingale.

XULRunner and the other Technologies
------------------------------------
Nightingale is based on a pretty rare combination of toolchains (these days),
not to mention that some of them are outdated.

Fist off, Nightingale uses a combination of C++ and JS as its main languages. In
order to contribute code you'll have to use git. In case you don't know git yet,
we recommend http://try.github.com. If you encounter any language you're not
familiar with, or new APIs, it's worth checking out
http://wiki.getnightingale.com/doku.php?id=coding_tutorials.

We use a GNU make based buildsystem. There is no real documentation on it,
other than it being similar to Mozilla's old buildsystem. If you don't know make,
just look at the Makefiles in Nightingale's source to get an idea of how it
works. If you run into problems, first try to figure them out using other
Makefiles or build/rules.mk, which has inline documentation of the available
variables.

If you're already familiar with XPCOM, you'll want to skip ahead to the next
paragraph. The first thing you'll want to do, if you've not done it already, is
read up on XPCOM, we recommend [the guide on MDN](https://developer.mozilla.org/en-US/docs/Mozilla/Tech/XPCOM/Guide).
You might also have to use or understand XUL or XBL, both are explained pretty
well on MDN.

Nightingale has a complete documentation for its XPCOM APIs on
http://developer.getnightingale.com.

Nightingale further uses TagLib, GStreamer and SQLite.

Pick a Branch
-------------
Before you can do any coding or testing, you'll have to pick a branch you
want to base your patch on. Most of the times that'll be sb-trunk-oldxul, which
is our "master" at the moment. If you want to work with GStreamer 1.0 you'll
have to pick the gstreamer-1.0 branch until that's merged. For any development
towards upgrading XULRunner, check out the master-xul-9.0.1 branch.

Coding Guidelines
-----------------
Before we get to the exciting part - changing Nightingale, some more theory.
This time coding guidelines. We do have very few code guidelines at the moment,
but there are guidelines: http://wiki.getnightingale.com/doku.php?id=developer_center:code_guidelines

Further you should try to respect [Mozilla's code guidelines](https://developer.mozilla.org/en-US/docs/Mozilla/Developer_guide/Coding_Style),
even though we don't insist on following these one-on-one.

Writing the Code
----------------
To the exciting part: writing code. Finally. If you haven't already,
[fork this repository](https://github.com/nightingale-media-player/nightingale-hacking/fork),
or update the branch you're working on to match ours. Before you change or add
code, if you're working on a bug within a component or certain parts of the UI
(mainly bindings), we would like you to create a unit test that reproduces the bug.
This allows us to make sure we don't reintroduce a bug.

Now it's time to do your changes to our code. You should be well prepared for
this, so there's nothing more on that.

If you've added new features in a component or a binding, please write unit
tests. Thank you.

Building
-------
While writing your patch you'll want to test it too, you'll have to build
Nightingale. Luckily we have pretty detailed [building guides](http://wiki.getnightingale.com/doku.php?id=build)
for all platforms. They explain all the prerequisite and steps needed for
building.

Submit a Pull Request
---------------------
Exciting times! You've finished the work on your issue or a stand-alone part of
it? Go ahead and post a pull request. We'll probably nit-pick on some of the
things you've done, ask you what the code does or why you've written it. But thankfully
you can add commits to pull requests after you've submitted them. If you want to
redo what you've done so far you can also close the pull and resubmit a new one.

If everything's alright we'll probably merge your pull after about a week.

Contact
-------
During all of this you'll probably encounter some issues, have no idea where to
look for things or you're just frustrated. We can most likely help you, as we probably
know the code base (as a whole) better than you. Just get in touch with us,
either directly on your issue or on irc.mozilla.org #nightingale.
