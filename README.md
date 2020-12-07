# AudioXCFrameworks

XCFramework build process for various open-source audio libraries used by [SFBAudioEngine](https://github.com/sbooth/SFBAudioEngine).

The built XCFrameworks target macOS 10.15+ and iOS 14.0+ for all supported 64-bit architectures.

## Building

1. `git clone git@github.com:sbooth/AudioXCFrameworks.git --recurse-submodules`
2. `cd AudioXCFrameworks`
3. `make`

Each subfolder will contain the built XCFramework at its top level.

## Installation

`make install` may be used from the top level Makefile to copy the XCFrameworks into `PREFIX`:

1. `make install PREFIX=`*destination_folder*

This is useful from a `Run Script` build phase when `AudioXCFrameworks` is a submodule of your project:

`make -C "$SRCROOT/AudioXCFrameworks" install PREFIX="$SRCROOT/XCFrameworks"`

## Licensing

The Xcode project files and overall build system are distributed under the MIT license.

Each individual open source project is subject to its own licensing terms. See the README in each folder for that particular project's license.
