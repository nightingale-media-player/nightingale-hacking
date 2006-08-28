#!/bin/bash

if [ $# = 0 ]; then
  echo usage: apply-patches patchdir targetdir [-R]
  exit 1
fi

patchdir="$1"
targetdir="$2"
reverse="$3"

list_files() {
  find . -maxdepth 1 -type f \
  -name "bug*" \
  ! -name "CVS" \
  | sed 's/\.\/\(.*\)/"\1"/'
}

notice() {
  echo $* 1>&2
}

list=$(cd "$patchdir" && list_files)
eval "patches=($list)"
num_patches=${#patches[*]}

D=`dirname "$patchdir"`
B=`basename "$patchdir"`
abspath="`cd \"$D\" 2>/dev/null && pwd || echo \"$D\"`/$B"
for ((i=0; $i<$num_patches; i=$i+1)); do
  p=${patches[$i]}
  notice  "# applying $p #"
  patch $reverse --backup-if-mismatch -p0 \
    -d "$targetdir" -i "$abspath/$p"
  echo ""
done
