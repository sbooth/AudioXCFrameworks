#!/bin/sh

WAVPACK_ARCHIVE=wavpack-5.3.0.tar.xz
WAVPACK_DOWNLOAD_URL=http://wavpack.com/$WAVPACK_ARCHIVE
WAVPACK_DIR=$(basename "$WAVPACK_ARCHIVE" .tar.xz)

if ! [ -f "./$WAVPACK_ARCHIVE" ]; then
	if ! [ -x "$(command -v curl)" ]; then
		echo "Error: $WAVPACK_ARCHIVE not found and curl not present"
		echo "Please download manually from"
		echo WAVPACK_DOWNLOAD_URL
		exit 1
	fi

	curl -O "$WAVPACK_DOWNLOAD_URL"
fi

/usr/bin/tar -xf "./$WAVPACK_ARCHIVE"
/bin/ln -s "./$WAVPACK_DIR" "./wavpack-src"
