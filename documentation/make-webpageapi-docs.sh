#!/bin/sh

notice() {
  echo $* 1>&2
}

if [ $# != 1 ]; then
  notice "usage: make-webpageapi-docs.sh [topsrcdir]"
  exit 1
fi

# Working directory
workingdir=`pwd`

# Trunk top level directory
topsrcdir="$workingdir/$1"

# Scripts directory
scriptsdir="$topsrcdir/tools/scripts"

# Remote API top level directory
remoteapidir="$topsrcdir/components/remoteapi"

# Library base top level directory
librarybasedir="$topsrcdir/components/library/base"

# Bindings base top level directory
bindingsdir="$topsrcdir/bindings/"

# Documentation temp directory
docstempdir="$topsrcdir/compiled/documentation/tmp"

# Documentation output directory
docsoutputdir="$topsrcdir/documentation/webpageapi"

# Natural Docs project directory
ndprojectdir="$topsrcdir/documentation/naturaldocsconfig"

# Remote API files needed to generate the documentation
remoteapifiles="$remoteapidir/public/sbIRemoteCommands.idl
                $remoteapidir/public/sbIRemoteLibrary.idl
                $remoteapidir/public/sbIRemoteMediaList.idl
                $remoteapidir/public/sbIRemotePlayer.idl
                $remoteapidir/public/sbIRemoteWebPlaylist.idl
                $remoteapidir/public/sbIWrappedMediaItem.h
"

# Library base files needed to generate the documentation
librarybasefiles="$librarybasedir/public/sbIMediaItem.idl
                  $librarybasedir/public/sbIMediaList.idl
                  $librarybasedir/public/sbILibraryResource.idl
"

# Library base files we need to clean up
librarybasefilescleanup="$docstempdir/sbIMediaItem.idl
                         $docstempdir/sbIMediaList.idl
                         $docstempdir/sbILibraryResource.idl
"
# Bindings files needed to generate the documentation
bindingsfiles="$bindingsdir/public/sbIPlaylistWidget.idl"

# Bindings files we need to clean up
bindingsfilescleanup="$docstempdir/public/sbIPlaylistWidget.idl"

# Shared lists
listsdir="$topsrcdir/documentation/lists"

# Cleanup old working temp dir
rm -rf $docstempdir

# Create working temp directory
mkdir $docstempdir

# Copy the necessary remoteapi idl files
cp -Lfp $remoteapifiles $docstempdir

# Copy the necessary library idl files
cp -Lfp $librarybasefiles $docstempdir

# Copy the necessary bindings idl files
cp -Lfp $bindingsfiles $docstempdir

# Merge RemoteMediaList with MediaList, MediaItem and LibraryResource
$scriptsdir/extract.pl sbILibraryResource < $docstempdir/sbILibraryResource.idl >> $docstempdir/sbIRemoteMediaList.idl
$scriptsdir/extract.pl sbIMediaItem < $docstempdir/sbIMediaItem.idl >> $docstempdir/sbIRemoteMediaList.idl
$scriptsdir/extract.pl sbIMediaList < $docstempdir/sbIMediaList.idl >> $docstempdir/sbIRemoteMediaList.idl

# Merge RemoteMediaItem with MediaItem, LibraryResource
$scriptsdir/extract.pl sbILibraryResource < $docstempdir/sbILibraryResource.idl >> $docstempdir/sbIWrappedMediaItem.h
$scriptsdir/extract.pl sbIMediaItem < $docstempdir/sbIMediaItem.idl >> $docstempdir/sbIWrappedMediaItem.h

# Merge SiteLibrary with LibraryResource
$scriptsdir/extract.pl sbILibraryResource < $docstempdir/sbILibraryResource.idl >> $docstempdir/sbIRemoteLibrary.idl

# Merge RemoteWebPlaylist with PlaylistWidget
$scriptsdir/extract.pl sbIPlaylistWidget < $docstempdir/sbIPlaylistWidget.idl >> $docstempdir/sbIRemoteWebPlaylist.idl

# Delete source files for merge so we don't get duplicate classes.
rm -f $librarybasefilescleanup
rm -f $bidningsfilecleanup

# Run NaturalDocs
$topsrcdir/tools/common/naturaldocs/NaturalDocs \
-i $docstempdir \
-i $listsdir \
-o HTML $docsoutputdir \
-p $ndprojectdir \
-s Default Docs -r
