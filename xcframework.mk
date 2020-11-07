ARCHIVE_DIR := archive

xcframework: $(FRAMEWORK_NAME).xcframework
.PHONY: xcframework

clean:
	rm -Rf "$(ARCHIVE_DIR)/macOS.xcarchive"
	rm -Rf "$(ARCHIVE_DIR)/macOS-Catalyst.xcarchive"
	rm -Rf "$(ARCHIVE_DIR)/iOS.xcarchive"
	rm -Rf "$(ARCHIVE_DIR)/iOS-Simulator.xcarchive"
	rm -Rf "$(FRAMEWORK_NAME).xcframework"
.PHONY: clean

ifneq (0,$(MAKELEVEL))
install: xcframework uninstall
	cp -R "$(FRAMEWORK_NAME).xcframework" "$(PREFIX)"
.PHONY: install

uninstall:
	rm -Rf "$(PREFIX)/$(FRAMEWORK_NAME).xcframework"
.PHONY: uninstall
endif

$(ARCHIVE_DIR)/macOS.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme macOS -destination generic/platform=macOS -archivePath "$(ARCHIVE_DIR)/macOS"

$(ARCHIVE_DIR)/macOS-Catalyst.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme iOS -destination "platform=macOS,variant=Mac Catalyst" -archivePath "$(ARCHIVE_DIR)/macOS-Catalyst"

$(ARCHIVE_DIR)/iOS.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme iOS -destination generic/platform=iOS -archivePath "$(ARCHIVE_DIR)/iOS"

$(ARCHIVE_DIR)/iOS-Simulator.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project "$(XCODEPROJ)" -scheme iOS -destination "generic/platform=iOS Simulator" -archivePath "$(ARCHIVE_DIR)/iOS-Simulator"

$(FRAMEWORK_NAME).xcframework: $(ARCHIVE_DIR)/macOS.xcarchive $(ARCHIVE_DIR)/macOS-Catalyst.xcarchive $(ARCHIVE_DIR)/iOS.xcarchive $(ARCHIVE_DIR)/iOS-Simulator.xcarchive
	xcodebuild -create-xcframework $(foreach xcarchive,$^,-framework "$(xcarchive)/Products/Library/Frameworks/$(FRAMEWORK_NAME).framework" ) -output "$@"
