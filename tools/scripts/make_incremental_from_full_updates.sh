#!/bin/sh

. $(dirname "$0")/common.sh

# ----------------------------------

print_usage() {
  notice "Usage: $(basename $0) [OPTIONS] ARCHIVE FROMMAR TOMAR"
}

if [ $# = 0 ]; then
  print_usage
  exit 1
fi

if [ $1 = -h ]; then
  print_usage
  notice ""
  notice "The difference between FROMMAR and TOMAR wil be stored in ARCHIVE."
  notice "The current directory will be used to hold temporary files."
  notice ""
  notice "Options:"
  notice "  -h  show this help text"
  notice ""
  exit 1
fi

# -----------------------------------

archive="$1"
frommar="$2"
tomar="$3"
workdir=".work/"
fromdir="$workdir/from/"
todir="$workdir/to/"
topdir="../.."

mkdir -p "$fromdir"
mkdir -p "$todir"

notice "  unwrapping archive: $frommar"

cd "$fromdir"
$topdir/unwrap_full_update.sh "$topdir/$frommar"
rm -f "./update.manifest"

notice "  unwrapping archive: $tomar"

cd "$topdir/$todir"
$topdir/unwrap_full_update.sh "$topdir/$tomar"
rm -f "./update.manifest"

cd "$topdir/"

notice "  creating partial archive: $archive"

make_incremental_update.sh "$archive" "$fromdir" "$todir"

notice "  cleaning up"
rm -rf "./$workdir"

notice "  done"
