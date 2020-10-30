#!/bin/sh

# Copyright (c) 2020 Stephen F. Booth <me@sbooth.org>
# See https://github.com/sbooth/AudioXCFrameworks/blob/master/LICENSE.txt for license information

if [ "$1" == "" ]; then
	echo "Usage: $0 framework-name"
	exit 1
fi

FRAMEWORK_NAME=$1

xcodebuild clean archive \
	-scheme "macOS" \
	-destination "generic/platform=macOS" \
	-archivePath "macOS"

xcodebuild clean archive \
	-scheme "iOS" \
	-destination "generic/platform=iOS" \
	-archivePath "iOS"

xcodebuild clean archive \
	-scheme "iOS" \
	-destination "generic/platform=iOS Simulator" \
	-archivePath "iOS-Simulator"

xcodebuild clean archive \
	-scheme "iOS" \
	-destination "platform=macOS,variant=Mac Catalyst" \
	-archivePath "catalyst"

/bin/rm -rf "./$FRAMEWORK_NAME.xcframework"

xcodebuild -create-xcframework \
	-framework macOS.xcarchive/Products/Library/Frameworks/$FRAMEWORK_NAME.framework \
	-framework iOS.xcarchive/Products/Library/Frameworks/$FRAMEWORK_NAME.framework \
	-framework iOS-Simulator.xcarchive/Products/Library/Frameworks/$FRAMEWORK_NAME.framework \
	-framework catalyst.xcarchive/Products/Library/Frameworks/$FRAMEWORK_NAME.framework \
	-output $FRAMEWORK_NAME.xcframework

## Debug symbols aren't working for me, despite https://developer.apple.com/forums/thread/655768

#	-debug-symbols macOS.xcarchive/dSYMs/$FRAMEWORK_NAME.framework.dSYM \
#	-debug-symbols iOS.xcarchive/dSYMs/$FRAMEWORK_NAME.framework.dSYM \
#	-debug-symbols iOS-Simulator.xcarchive/dSYMs/$FRAMEWORK_NAME.framework.dSYM \
#	-debug-symbols catalyst.xcarchive/dSYMs/$FRAMEWORK_NAME.framework.dSYM \
