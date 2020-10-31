#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

DUMB_TAG=2.0.3
DUMB_ARCHIVE=dumb-$DUMB_TAG.tar.gz
DUMB_DOWNLOAD_URL=https://github.com/kode54/dumb/archive/$DUMB_TAG.tar.gz
DUMB_DIR=$(basename "$DUMB_ARCHIVE" .tar.gz)

if ! [ -f "./$DUMB_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $DUMB_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $DUMB_DOWNLOAD_URL
		exit 1
	fi

	curl -L "$DUMB_DOWNLOAD_URL" --output $DUMB_ARCHIVE
fi

/usr/bin/tar -xf "./$DUMB_ARCHIVE"

if [ -h "./dumb-src" ]; then
	/bin/rm "./dumb-src"
fi

/bin/ln -s "./$DUMB_DIR" "./dumb-src"
