if [ $# = 0 ]; then
  echo usage: flatten-dir sourcedir targetdir
  exit 1
fi

sourcedir="$1"
targetdir="$2"

find $sourcedir -type f \
  ! -name "Entries" \
  ! -name "Entries.Log" \
  ! -name "Entries.Old" \
  ! -name "Entries.Extra" \
  ! -name "Entries.Extra.Old" \
  ! -name "Repository" \
  ! -name "Root" \
  ! -name "Template" \
  | xargs -I % cp % $targetdir
