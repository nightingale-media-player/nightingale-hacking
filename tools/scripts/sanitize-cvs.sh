#!/bin/sh

if [ $# = 0 ]; then
  echo usage: sanitize-cvs targetdir
  exit 1
fi

targetdir="$1"

list_dirs() {
  find . -type d \
  -name "CVS" \
  | sed 's/\.\/\(.*\)/"\1"/'
}

notice() {
  echo $* 1>&2
}

list=$(cd "$targetdir" && list_dirs)
eval "dirs=($list)"
num_dirs=${#dirs[*]}

for ((i=0; $i<$num_dirs; i=$i+1)); do
  d=${dirs[$i]}
  notice  "deleting $targetdir$d"
  rm -rf "$targetdir$d"
done
