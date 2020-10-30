#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

MPG123_ARCHIVE=mpg123-1.26.3.tar.bz2
MPG123_DOWNLOAD_URL=https://master.dl.sourceforge.net/project/mpg123/mpg123/1.26.3/$MPG123_ARCHIVE
MPG123_DIR=$(basename "$MPG123_ARCHIVE" .tar.bz2)

if ! [ -f "./$MPG123_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $MPG123_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $MPG123_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$MPG123_DOWNLOAD_URL"
fi

/usr/bin/tar -xf "./$MPG123_ARCHIVE"
/bin/ln -s "./$MPG123_DIR" "./mpg123-src"
/bin/cp -f "./config.h" "./mpg123-src/libmpg123/"
/bin/cp -f "./mpg123.h" "./mpg123-src/src/libmpg123/"
