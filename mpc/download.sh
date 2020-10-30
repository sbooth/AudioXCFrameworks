#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

MUSEPACK_ARCHIVE=musepack_src_r475.tar.gz
MUSEPACK_DOWNLOAD_URL=https://files.musepack.net/source/$MUSEPACK_ARCHIVE
MUSEPACK_DIR=$(basename "$MUSEPACK_ARCHIVE" .tar.gz)

if ! [ -f "./$MUSEPACK_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $MUSEPACK_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $MUSEPACK_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$MUSEPACK_DOWNLOAD_URL"
fi

/usr/bin/tar -xf "./$MUSEPACK_ARCHIVE"
/bin/ln -s "./$MUSEPACK_DIR" "./musepack-src"
/usr/bin/patch -p0 < "./extern.patch"
