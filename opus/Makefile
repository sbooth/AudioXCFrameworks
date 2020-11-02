FRAMEWORK_NAME=opus
XCODEPROJ=opus.xcodeproj

xcframework: patch

include ../xcframework.mk

OPUS_SRC_DIR=opus-src
OPUSFILE_SRC_DIR=opusfile-src
PATCH_FILES=quoted_include.patch

$(OPUS_SRC_DIR)/config.h: config.h
	cp -f $< $@

$(OPUSFILE_SRC_DIR)/config.h: opusfile-config.h
	cp -f $< $@

copy: $(OPUS_SRC_DIR)/config.h $(OPUSFILE_SRC_DIR)/config.h
.PHONY: copy

patch: copy $(PATCH_FILES:.patch=.patched)
.PHONY: patch

%.patched: %.patch
	if [ -e $@ ]; then patch -p0 -R < $@; fi
	patch -p0 < $<
	cp -f $< $@

realclean:: clean
	rm -f $(PATCH_FILES:.patch=.patched)
