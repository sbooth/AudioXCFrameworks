FRAMEWORK_NAME := sndfile
XCODEPROJ := sndfile.xcodeproj

xcframework: patch

include ../xcframework.mk

SRC_DIR := libsndfile-src
PATCH_FILES := quoted_include.patch

$(SRC_DIR)/src/config.h: config.h
	cp -f $< $@

$(SRC_DIR)/include/sndfile.h: sndfile.h
	cp -f $< $@

copy: $(SRC_DIR)/src/config.h $(SRC_DIR)/include/sndfile.h
.PHONY: copy

patch: copy $(PATCH_FILES:.patch=.patched)
.PHONY: patch

%.patched: %.patch
	if [ -e $@ ]; then patch -p0 -R < $@; fi
	patch -p0 < $<
	cp -f $< $@

realclean:: clean
	rm -f $(PATCH_FILES:.patch=.patched)
