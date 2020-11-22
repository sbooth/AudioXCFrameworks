FRAMEWORK_NAME := lame
XCODEPROJ := lame.xcodeproj

xcframework: extract
include ../xcframework.mk

SRC_ARCHIVE := lame-3.100.tar.gz
SRC_DOWNLOAD_URL := https://sourceforge.net/projects/lame/files/lame/3.100/lame-3.100.tar.gz/download
SRC_DIR := lame-src

$(SRC_ARCHIVE):
	curl -L $(SRC_DOWNLOAD_URL) -o $@

$(SRC_DIR):
	mkdir $(SRC_DIR) && tar -xf $(SRC_ARCHIVE) --directory $(SRC_DIR) --strip-components 1
	cp -f config.h lame-src/

download: $(SRC_ARCHIVE)
.PHONY: download

extract: download $(SRC_DIR)
.PHONY: extract

realclean:: clean
	rm -f $(SRC_ARCHIVE)
	rm -Rf $(SRC_DIR)
