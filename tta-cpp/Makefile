FRAMEWORK_NAME := tta++
XCODEPROJ := tta++.xcodeproj

xcframework: patch

include ../xcframework.mk

SRC_ARCHIVE := libtta-cpp-2.3.tar.gz
SRC_DOWNLOAD_URL := https://sourceforge.net/projects/tta/files/tta/libtta%2B%2B/libtta-cpp-2.3.tar.gz/download
SRC_DIR := libtta-cpp-src
PATCH_FILES := aligned_alloc.patch

$(SRC_ARCHIVE):
	curl -L $(SRC_DOWNLOAD_URL) -o $@

$(SRC_DIR):
	mkdir $(SRC_DIR) && tar -xf $(SRC_ARCHIVE) --directory $(SRC_DIR) --strip-components 1
	cp -f config.h libtta-cpp-src/

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
