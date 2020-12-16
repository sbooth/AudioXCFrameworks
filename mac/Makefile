FRAMEWORK_NAME := MAC
XCODEPROJ := mac.xcodeproj

xcframework: patch

include ../xcframework.mk

SRC_ARCHIVE := MAC_SDK_569.zip
SRC_DOWNLOAD_URL := https://monkeysaudio.com/files/$(SRC_ARCHIVE)
SRC_DIR := MAC_SDK-src
PATCH_FILES := angle_bracket_include.patch

$(SRC_ARCHIVE):
	curl $(SRC_DOWNLOAD_URL) -o $@

$(SRC_DIR):
	unzip -u $(SRC_ARCHIVE) -d $(SRC_DIR)

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
