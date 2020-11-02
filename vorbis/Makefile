FRAMEWORK_NAME=vorbis
XCODEPROJ=vorbis.xcodeproj

xcframework: copy

include ../xcframework.mk

SRC_DIR=vorbis-src

$(SRC_DIR)/config.h: config.h
	cp -f $< $@

copy: $(SRC_DIR)/config.h
.PHONY: copy
