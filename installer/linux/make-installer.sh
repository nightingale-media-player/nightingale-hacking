#!/bin/bash

CURRENT_DIR=`pwd`
CURRENT_DATE=`date +%Y%m%d`
SONGBIRD_DIR=Songbird_${CURRENT_DATE}

rm -rf ../../../_built_installer
mkdir ../../../_built_installer

rm -rf ../../../compiled/${SONGBIRD_DIR}
mkdir ../../../compiled/${SONGBIRD_DIR}

cp -RL ../../../compiled/dist/* ../../../compiled/${SONGBIRD_DIR}
cp ../../../installer/linux/GPL.txt ../../../compiled/${SONGBIRD_DIR}
cp ../../../installer/linux/LICENSE.txt ../../../compiled/${SONGBIRD_DIR}
cp ../../../installer/linux/TRADEMARK.txt ../../../compiled/${SONGBIRD_DIR}

rm -f ../../../compiled/${SONGBIRD_DIR}.tar
rm -f ../../../compiled/${SONGBIRD_DIR}.tar.gz

cd ../../../compiled
tar -c -f ${SONGBIRD_DIR}.tar ${SONGBIRD_DIR}
gzip ${SONGBIRD_DIR}.tar

cp ${SONGBIRD_DIR}.tar.gz ../_built_installer

