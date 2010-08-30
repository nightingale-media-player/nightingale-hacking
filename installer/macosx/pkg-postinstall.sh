#!/bin/bash

APPNAME="Songbird.app"
TARGET="${2}/${APPNAME}"
RESDIR="${TARGET}/Contents/Resources"
if [ -z "${DISTHELPER_DISTINI}" ] ; then
  DISTHELPER_DISTINI="${RESDIR}/songbird.ini"
fi

# Set the umask here explicitly to create files as group-writable, so any
# files disthelper creates are group-writable

umask 002

# We chown the entire .app bundle both before disthelper runs and after,
# just so we're sure we get any files disthelper itself created.
chown -R "${USER}" "${TARGET}" 1>&2

cd "${RESDIR}"
./disthelper install
DISTHELPER_RETURN_VAL=$?

chown -R "${USER}" "${TARGET}" 1>&2
exit $DISTHELPER_RETURN_VAL
