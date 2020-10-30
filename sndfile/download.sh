#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

LIBSNDFILE_ARCHIVE=libsndfile-1.0.28.tar.gz
LIBSNDFILE_DOWNLOAD_URL=http://www.mega-nerd.com/libsndfile/files/$LIBSNDFILE_ARCHIVE
LIBSNDFILE_DIR=$(basename "$LIBSNDFILE_ARCHIVE" .tar.gz)

if ! [ -f "./$LIBSNDFILE_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $LIBSNDFILE_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $LIBSNDFILE_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$LIBSNDFILE_DOWNLOAD_URL"
fi

/usr/bin/tar -xf "./$LIBSNDFILE_ARCHIVE"
/bin/ln -s "./$LIBSNDFILE_DIR" "./libsndfile-src"
/bin/cp -f "./config.h" "./libsndfile-src/src/"
/bin/cp -f "./sndfile.h" "./libsndfile-src/src/"
/usr/bin/patch -p0 < "./quoted_include.patch"
