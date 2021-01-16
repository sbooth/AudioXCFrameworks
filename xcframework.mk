ARCHIVE_DIR := archive

MACOS_ARCHIVE := $(ARCHIVE_DIR)/macOS.xcarchive
MACOS_CATALYST_ARCHIVE := $(ARCHIVE_DIR)/macOS-Catalyst.xcarchive
IOS_ARCHIVE := $(ARCHIVE_DIR)/iOS.xcarchive
IOS_SIMULATOR_ARCHIVE := $(ARCHIVE_DIR)/iOS-Simulator.xcarchive

ARCHIVES := $(MACOS_ARCHIVE) $(MACOS_CATALYST_ARCHIVE) $(IOS_ARCHIVE) $(IOS_SIMULATOR_ARCHIVE)

XCFRAMEWORK := $(FRAMEWORK_NAME).xcframework

xcframework: $(XCFRAMEWORK)
.PHONY: xcframework

clean:
	rm -Rf "$(MACOS_ARCHIVE)" "$(MACOS_CATALYST_ARCHIVE)" "$(IOS_ARCHIVE)" "$(IOS_SIMULATOR_ARCHIVE)" "$(XCFRAMEWORK)"
.PHONY: clean

ifneq (0,$(MAKELEVEL))
install: xcframework uninstall
	cp -R "$(XCFRAMEWORK)" "$(PREFIX)"
.PHONY: install

uninstall:
	rm -Rf "$(PREFIX)/$(XCFRAMEWORK)"
.PHONY: uninstall
endif

$(MACOS_ARCHIVE): $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme macOS -destination generic/platform=macOS -archivePath "$(basename $@)"

$(MACOS_CATALYST_ARCHIVE): $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme iOS -destination "platform=macOS,variant=Mac Catalyst" -archivePath "$(basename $@)"

$(IOS_ARCHIVE): $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme iOS -destination generic/platform=iOS -archivePath "$(basename $@)"

$(IOS_SIMULATOR_ARCHIVE): $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme iOS -destination "generic/platform=iOS Simulator" -archivePath "$(basename $@)"

$(XCFRAMEWORK): $(ARCHIVES)
	rm -Rf "$@"
	xcodebuild -create-xcframework $(foreach xcarchive,$^,-framework "$(xcarchive)/Products/Library/Frameworks/$(FRAMEWORK_NAME).framework" -debug-symbols "$(realpath $(xcarchive)/dSYMs/$(FRAMEWORK_NAME).framework.dSYM)" ) -output "$@"
