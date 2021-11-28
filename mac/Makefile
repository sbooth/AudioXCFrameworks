FRAMEWORK_NAME := MAC
XCODEPROJ := mac.xcodeproj

xcframework: patch

include ../xcframework.mk

PATCH_FILES := angle_bracket_include.patch

patch: $(PATCH_FILES:.patch=.patched)
.PHONY: patch

%.patched: %.patch
	if [ -e $@ ]; then patch -p0 -R < $@; fi
	patch -p0 < $<
	cp -f $< $@

realclean:: clean
	rm -f $(PATCH_FILES:.patch=.patched)
