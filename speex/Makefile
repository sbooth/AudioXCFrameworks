FRAMEWORK_NAME := speex
XCODEPROJ := speex.xcodeproj

xcframework: copy

include ../xcframework.mk

SPEEX_SRC_DIR := speex-src
SPEEXDSP_SRC_DIR := speexdsp-src

$(SPEEX_SRC_DIR)/config.h: config.h
	cp -f $< $@

$(SPEEX_SRC_DIR)/include/speex/speex_config_types.h: speex_config_types.h
	cp -f $< $@

$(SPEEXDSP_SRC_DIR)/config.h: speexdsp-config.h
	cp -f $< $@

$(SPEEXDSP_SRC_DIR)/include/speex/speexdsp_config_types.h: speexdsp_config_types.h
	cp -f $< $@

copy: $(SPEEX_SRC_DIR)/config.h $(SPEEX_SRC_DIR)/include/speex/speex_config_types.h $(SPEEXDSP_SRC_DIR)/config.h $(SPEEXDSP_SRC_DIR)/include/speex/speexdsp_config_types.h
.PHONY: copy
