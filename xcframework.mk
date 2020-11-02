xcframework: $(FRAMEWORK_NAME).xcframework
.PHONY: xcframework

clean:
	rm -Rf macOS.xcarchive
	rm -Rf macOS-Catalyst.xcarchive
	rm -Rf iOS.xcarchive
	rm -Rf iOS-Simulator.xcarchive
	rm -Rf $(FRAMEWORK_NAME).xcframework
.PHONY: clean

macOS.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project $(XCODEPROJ) -scheme "macOS" -destination "generic/platform=macOS" -archivePath "macOS"

macOS-Catalyst.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project $(XCODEPROJ) -scheme "iOS" -destination "platform=macOS,variant=Mac Catalyst" -archivePath "macOS-Catalyst"

iOS.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project $(XCODEPROJ) -scheme "iOS" -destination "generic/platform=iOS" -archivePath "iOS"

iOS-Simulator.xcarchive: $(XCODEPROJ)
	xcodebuild archive -project $(XCODEPROJ) -scheme "iOS" -destination "generic/platform=iOS Simulator" -archivePath "iOS-Simulator"

$(FRAMEWORK_NAME).xcframework: macOS.xcarchive macOS-Catalyst.xcarchive iOS.xcarchive iOS-Simulator.xcarchive
	xcodebuild -create-xcframework $(foreach xcarchive,$^,-framework $(xcarchive)/Products/Library/Frameworks/$(FRAMEWORK_NAME).framework ) -output $@
