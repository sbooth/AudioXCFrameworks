#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

MAC_ARCHIVE=MAC_SDK_558.zip
MAC_DOWNLOAD_URL=https://monkeysaudio.com/files/$MAC_ARCHIVE
MAC_DIR=$(basename "$MAC_ARCHIVE" .zip)

if ! [ -f "./$MAC_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $MAC_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo $MAC_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$MAC_DOWNLOAD_URL"
fi

/usr/bin/unzip "./$MAC_ARCHIVE" -d "./$MAC_DIR"
/bin/ln -s "./$MAC_DIR" "./MAC_SDK-src"
/usr/bin/patch -p0 < "./angle_bracket_include.patch"
/usr/bin/patch -p0 < "./objc_bool.patch"
