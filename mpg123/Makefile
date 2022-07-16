FRAMEWORK_NAME := mpg123
XCODEPROJ := mpg123.xcodeproj

xcframework: extract

include ../xcframework.mk

SRC_ARCHIVE := mpg123-1.30.1.tar.bz2
SRC_DOWNLOAD_URL := https://sourceforge.net/projects/mpg123/files/mpg123/1.30.1/mpg123-1.30.1.tar.bz2/download
SRC_DIR := mpg123-src

$(SRC_ARCHIVE):
	curl -L $(SRC_DOWNLOAD_URL) -o $@

$(SRC_DIR):
	mkdir $(SRC_DIR) && tar -xf $(SRC_ARCHIVE) --directory $(SRC_DIR) --strip-components 1
	cp -f config.h mpg123-src/src/
	cp -f mpg123.h mpg123-src/src/libmpg123/

download: $(SRC_ARCHIVE)
.PHONY: download

extract: download $(SRC_DIR)
.PHONY: extract

realclean:: clean
	rm -f $(SRC_ARCHIVE)
	rm -Rf $(SRC_DIR)
