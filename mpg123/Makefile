FRAMEWORK_NAME := mpg123
XCODEPROJ := mpg123.xcodeproj

xcframework: extract

include ../xcframework.mk

SRC_ARCHIVE := mpg123-1.26.3.tar.bz2
SRC_DOWNLOAD_URL := https://master.dl.sourceforge.net/project/mpg123/mpg123/1.26.3/$(SRC_ARCHIVE)
SRC_DIR := mpg123-src

$(SRC_ARCHIVE):
	curl $(SRC_DOWNLOAD_URL) -o $@

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
