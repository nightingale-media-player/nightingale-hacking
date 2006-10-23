#!/bin/sh
#
# BEGIN SONGBIRD GPL
#
# This file is part of the Songbird web player.
#
# Copyright© 2006 Pioneers of the Inevitable LLC
# http://www.songbirdnest.com
#
# This file may be licensed under the terms of of the
# GNU General Public License Version 2 (the <93>GPL<94>).
#
# Software distributed under the License is distributed
# on an <93>AS IS<94> basis, WITHOUT WARRANTY OF ANY KIND, either
# express or implied. See the GPL for the specific language
# governing rights and limitations.
#
# You should have received a copy of the GPL along with this
# program. If not, go to http://www.gnu.org/licenses/gpl.html
# or write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# END SONGBIRD GPL
#

# this file is a modified version of the mozilla file in test-harness

# MOZILLA_FIVE_HOME is exported by run-mozilla.sh, and is usually $(DIST)/bin
# we need to know that dir path in order to find xpcshell
#bin=${MOZILLA_FIVE_HOME:-`dirname $0`}

# this script is designed to run from the songbird dist directory, which has
# xulrunner as a sub dir, so we know where to find it already.
bin="."

exit_status=0

# accept a directory to test as an arg, with test-harness being the default
testdir="$1"
if [ "x$testdir" = "x" ]; then
    testdir="test-harness"
fi

# must have this head file to get some default functionality
headfiles="-f $bin/xulrunner/test-harness/xpcshell-simple/head.js"

# files matching the pattern head_*.js are treated like test setup files
# - they are run after head.js but before the test file
for h in $testdir/head_*.js
do
    if [ -f $h ]; then
	headfiles="$headfiles -f $h"
    fi
done

# default tail file for shut down functionality
tailfiles="-f $bin/xulrunner/test-harness/xpcshell-simple/tail.js"

# files matching the pattern tail_*.js are treated like teardown files
# - they are run after tail.js
for t in $testdir/tail_*.js
do
    if [ -f $t ]; then
	tailfiles="$tailfiles -f $t"
    fi
done

# for all files starting with 'test_' pass them to xpcshell and run them
for t in $testdir/test_*.js
do
    echo -n "$t: "
    $bin/xulrunner/run-mozilla.sh $bin/xulrunner/xpcshell $headfiles -f $t $tailfiles 2> $t.log 1>&2
    if [ `grep -c '\*\*\* PASS' $t.log` = 0 ]
    then
        echo "FAIL (see $t.log)"
        exit_status=1
    else
        echo "PASS"
    fi
done

exit $exit_status
