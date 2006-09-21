#!/bin/bash
targetdir="$1"

notice() {
  echo $* 1>&2
}

list_files() {
  find . -type f \
    ! -name "Root" \
    ! -name "Repository" \
    ! -name "Entries.Old" \
    ! -name "Entries.Extra.Old" \
    ! -name "Template" \
    ! -name "Entries" \
    ! -name "Entries.Extra" \
    | sed 's/\.\/\(.*\)/"\1"/'
}

list=$(cd "$targetdir" && list_files)
eval "files=($list)"
num_files=${#files[*]}

for ((i=0; $i<$num_files; i=$i+1)); do
  f=${files[$i]}
  notice "$f"
done
