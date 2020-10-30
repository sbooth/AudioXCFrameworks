#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

LIBOGG_ARCHIVE=libogg-1.3.4.tar.xz
LIBOGG_DOWNLOAD_URL=https://downloads.xiph.org/releases/ogg/$LIBOGG_ARCHIVE
LIBOGG_DIR=$(basename "$LIBOGG_ARCHIVE" .tar.xz)

if ! [ -f "./$LIBOGG_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $LIBOGG_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $LIBOGG_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$LIBOGG_DOWNLOAD_URL"
fi

/usr/bin/tar -xf "./$LIBOGG_ARCHIVE"
/bin/ln -s "./$LIBOGG_DIR" "./libogg-src"
