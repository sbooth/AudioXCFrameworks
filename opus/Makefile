FRAMEWORK_NAME := opus
XCODEPROJ := opus.xcodeproj

xcframework: patch

include ../xcframework.mk

OPUS_SRC_DIR := opus-src
OPUSFILE_SRC_DIR := opusfile-src
LIBOPUSENC_SRC_DIR := libopusenc-src
PATCH_FILES := quoted_include.patch quoted_include_2.patch

$(OPUS_SRC_DIR)/config.h: config.h
	cp -f $< $@

$(OPUSFILE_SRC_DIR)/config.h: opusfile-config.h
	cp -f $< $@

$(LIBOPUSENC_SRC_DIR)/config.h: libopusenc-config.h
	cp -f $< $@

copy: $(OPUS_SRC_DIR)/config.h $(OPUSFILE_SRC_DIR)/config.h $(LIBOPUSENC_SRC_DIR)/config.h
.PHONY: copy

patch: copy $(PATCH_FILES:.patch=.patched)
.PHONY: patch

%.patched: %.patch
	if [ -e $@ ]; then patch -p0 -R < $@; fi
	patch -p0 < $<
	cp -f $< $@

realclean:: clean
	rm -f $(PATCH_FILES:.patch=.patched)
