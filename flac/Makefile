FRAMEWORK_NAME := FLAC
XCODEPROJ := flac.xcodeproj

xcframework: copy

include ../xcframework.mk

SRC_DIR := flac-src

$(SRC_DIR)/config.h: config.h
	cp -f $< $@

copy: $(SRC_DIR)/config.h
.PHONY: copy
