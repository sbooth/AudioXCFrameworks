FRAMEWORK_NAME := sndfile
XCODEPROJ := sndfile.xcodeproj

xcframework: patch

include ../xcframework.mk

SRC_ARCHIVE := libsndfile-1.0.28.tar.gz
SRC_DOWNLOAD_URL := http://www.mega-nerd.com/libsndfile/files/$(SRC_ARCHIVE)
SRC_DIR := libsndfile-src
PATCH_FILES := quoted_include.patch

$(SRC_ARCHIVE):
	curl $(SRC_DOWNLOAD_URL) -o $@

$(SRC_DIR):
	mkdir $(SRC_DIR) && tar -xf $(SRC_ARCHIVE) --directory $(SRC_DIR) --strip-components 1
	cp -f config.h libsndfile-src/src/
	cp -f sndfile.h libsndfile-src/src/

download: $(SRC_ARCHIVE)
.PHONY: download

extract: download $(SRC_DIR)
.PHONY: extract

patch: extract $(PATCH_FILES:.patch=.patched)
.PHONY: patch

%.patched: %.patch
	if [ -e $@ ]; then patch -p0 -R < $@; fi
	patch -p0 < $<
	cp -f $< $@

realclean:: clean
	rm -f $(PATCH_FILES:.patch=.patched)
	rm -f $(SRC_ARCHIVE)
	rm -Rf $(SRC_DIR)
