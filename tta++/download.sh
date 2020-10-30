#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

LIBTTA_CPP_ARCHIVE=libtta-cpp-2.3.tar.gz
LIBTTA_CPP_DOWNLOAD_URL=https://master.dl.sourceforge.net/project/tta/tta/libtta%2B%2B/$LIBTTA_CPP_ARCHIVE
LIBTTA_CPP_DIR=$(basename "$LIBTTA_CPP_ARCHIVE" .tar.gz)

if ! [ -f "./$LIBTTA_CPP_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $LIBTTA_CPP_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $LIBTTA_CPP_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$LIBTTA_CPP_DOWNLOAD_URL"
fi

/usr/bin/tar -xf "./$LIBTTA_CPP_ARCHIVE"
/bin/ln -s "./$LIBTTA_CPP_DIR" "./libtta-cpp-src"
/bin/cp -f "./config.h" "./libtta-cpp-src/"
/usr/bin/patch "./libtta-cpp-src/libtta.cpp" < "./aligned_alloc.patch"
