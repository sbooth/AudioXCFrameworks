cmake_minimum_required(VERSION 3.6)
project("Monkey's Audio")

set(CMAKE_CXX_STANDARD 98)

option(BUILD_SHARED "Build libMAC as a shared library" ON)
option(BUILD_UTIL "Build the mac command line tool" ON)

#
# Determine interface version
#
file(READ Shared/Version.h VERSION_INFO)
string(REGEX MATCH "APE_INTERFACE_VERSION[ \t]+([0-9]+)" _ ${VERSION_INFO})
set(MAC_SOVERSION ${CMAKE_MATCH_1})

#
# Architecture detection code
#
set(archdetect_c_code "
#if defined __i386__ || defined _M_IX86
    #error cmake_ARCH x86
#elif defined __x86_64__ || defined _M_AMD64
    #error cmake_ARCH x86_64
#elif defined __arm__ || defined _M_ARM
    #error cmake_ARCH arm
#elif defined __aarch64__ || defined _M_ARM64
    #error cmake_ARCH aarch64
#elif defined __mips__
    #error cmake_ARCH mips
#elif defined __ppc__
    #error cmake_ARCH ppc
#elif defined __ppc64__
    #error cmake_ARCH ppc64
#elif defined __riscv
    #error cmake_ARCH riscv
#else
    #error cmake_ARCH unknown
#endif
")

function(find_target_architecture output_var)
    file(WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/arch.c" "${archdetect_c_code}")

    enable_language(C)

    try_run(
        run_result_unused
        compile_result_unused
        "${CMAKE_BINARY_DIR}"
        "${CMAKE_BINARY_DIR}/CMakeFiles/arch.c"
        COMPILE_OUTPUT_VARIABLE ARCH
        CMAKE_FLAGS
    )

    # Parse the architecture name from the compiler output
    string(REGEX MATCH "cmake_ARCH ([a-zA-Z0-9_]+)" ARCH "${ARCH}")

    # Get rid of the value marker leaving just the architecture name
    string(REPLACE "cmake_ARCH " "" ARCH "${ARCH}")

    set(${output_var} "${ARCH}" PARENT_SCOPE)
endfunction()

find_target_architecture(ARCHITECTURE)

#
# Define Monkey's Audio source files
#
set(MAC_PUBLIC_HEADERS
        Shared/All.h
        Shared/APETag.h
        Shared/CharacterHelper.h
        Shared/IO.h
        Shared/MACDll.h
        Shared/MACLib.h
        Shared/NoWindows.h
        Shared/SmartPtr.h
        Shared/Version.h
        Shared/Warnings.h
        Shared/WindowsEnvironment.h)

set(MAC_SHARED_HEADERS
        Source/Shared/All.h
        Source/Shared/BufferIO.h
        Source/Shared/CharacterHelper.h
        Source/Shared/CircleBuffer.h
        Source/Shared/CPUFeatures.h
        Source/Shared/CRC.h
        Source/Shared/GlobalFunctions.h
        Source/Shared/ID3Genres.h
        Source/Shared/IO.h
        Source/Shared/MACDll.h
        Source/Shared/MemoryIO.h
        Source/Shared/MFCGlobals.h
        Source/Shared/MFCWarnings.h
        Source/Shared/NoWindows.h
        Source/Shared/RollBuffer.h
        Source/Shared/Semaphore.h
        Source/Shared/SmartPtr.h
        Source/Shared/StdLibFileIO.h
        Source/Shared/Thread.h
        Source/Shared/Version.h
        Source/Shared/Warnings.h
        Source/Shared/WholeFileIO.h
        Source/Shared/WindowsEnvironment.h
        Source/Shared/WinFileIO.h)

set(MAC_LIBRARY_HEADERS
        Source/MACLib/APECompress.h
        Source/MACLib/APECompressCore.h
        Source/MACLib/APECompressCreate.h
        Source/MACLib/APEDecompress.h
        Source/MACLib/APEDecompressCore.h
        Source/MACLib/APEHeader.h
        Source/MACLib/APEInfo.h
        Source/MACLib/APELink.h
        Source/MACLib/APETag.h
        Source/MACLib/BitArray.h
        Source/MACLib/FloatTransform.h
        Source/MACLib/MACLib.h
        Source/MACLib/MACProgressHelper.h
        Source/MACLib/MD5.h
        Source/MACLib/NewPredictor.h
        Source/MACLib/NNFilter.h
        Source/MACLib/NNFilterCommon.h
        Source/MACLib/Predictor.h
        Source/MACLib/Prepare.h
        Source/MACLib/ScaledFirstOrderFilter.h
        Source/MACLib/UnBitArray.h
        Source/MACLib/UnBitArrayBase.h
        Source/MACLib/WAVInputSource.h
        Source/MACLib/Old/Anti-Predictor.h
        Source/MACLib/Old/APEDecompressCoreOld.h
        Source/MACLib/Old/APEDecompressOld.h
        Source/MACLib/Old/UnBitArrayOld.h
        Source/MACLib/Old/UnMAC.h)

if(WIN32)
   set(MAC_OS_SRC
        Source/Shared/WinFileIO.cpp)
else()
   set(MAC_OS_SRC
        Source/Shared/StdLibFileIO.cpp)
endif()

set(MAC_SHARED_SOURCE_FILES
        Source/Shared/BufferIO.cpp
        Source/Shared/CharacterHelper.cpp
        Source/Shared/CircleBuffer.cpp
        Source/Shared/CPUFeatures.cpp
        Source/Shared/CRC.cpp
        Source/Shared/GlobalFunctions.cpp
        Source/Shared/MemoryIO.cpp
        Source/Shared/Semaphore.cpp
        Source/Shared/Thread.cpp
        Source/Shared/WholeFileIO.cpp
        ${MAC_OS_SRC})

set(MAC_LIBRARY_SOURCE_FILES
        Source/MACDll/MACDll.cpp
        Source/MACLib/APECompress.cpp
        Source/MACLib/APECompressCore.cpp
        Source/MACLib/APECompressCreate.cpp
        Source/MACLib/APEDecompress.cpp
        Source/MACLib/APEDecompressCore.cpp
        Source/MACLib/APEHeader.cpp
        Source/MACLib/APEInfo.cpp
        Source/MACLib/APELink.cpp
        Source/MACLib/APETag.cpp
        Source/MACLib/BitArray.cpp
        Source/MACLib/FloatTransform.cpp
        Source/MACLib/MACLib.cpp
        Source/MACLib/MACProgressHelper.cpp
        Source/MACLib/MD5.cpp
        Source/MACLib/NewPredictor.cpp
        Source/MACLib/NNFilter.cpp
        Source/MACLib/NNFilterGeneric.cpp
        Source/MACLib/Prepare.cpp
        Source/MACLib/UnBitArray.cpp
        Source/MACLib/UnBitArrayBase.cpp
        Source/MACLib/WAVInputSource.cpp
        Source/MACLib/Old/Anti-Predictor.cpp
        Source/MACLib/Old/AntiPredictorExtraHigh.cpp
        Source/MACLib/Old/AntiPredictorFast.cpp
        Source/MACLib/Old/AntiPredictorHigh.cpp
        Source/MACLib/Old/AntiPredictorNormal.cpp
        Source/MACLib/Old/APEDecompressCoreOld.cpp
        Source/MACLib/Old/APEDecompressOld.cpp
        Source/MACLib/Old/UnBitArrayOld.cpp
        Source/MACLib/Old/UnMAC.cpp)

if(${ARCHITECTURE} STREQUAL x86 OR ${ARCHITECTURE} STREQUAL x86_64)
    set(MAC_LIBRARY_SOURCE_FILES
        ${MAC_LIBRARY_SOURCE_FILES}
        Source/MACLib/NNFilterAVX2.cpp
        Source/MACLib/NNFilterAVX512.cpp
        Source/MACLib/NNFilterSSE2.cpp
        Source/MACLib/NNFilterSSE4.1.cpp)
endif()

if(${ARCHITECTURE} STREQUAL arm OR ${ARCHITECTURE} STREQUAL aarch64)
    set(MAC_LIBRARY_SOURCE_FILES
        ${MAC_LIBRARY_SOURCE_FILES}
        Source/MACLib/NNFilterNeon.cpp)
endif()

#
# Define library targets
#
add_library(shared OBJECT ${MAC_SHARED_HEADERS} ${MAC_SHARED_SOURCE_FILES})
add_library(objects OBJECT ${MAC_LIBRARY_HEADERS} ${MAC_LIBRARY_SOURCE_FILES})

if(BUILD_SHARED)
    add_library(MAC SHARED $<TARGET_OBJECTS:shared> $<TARGET_OBJECTS:objects>)
    set_target_properties(MAC PROPERTIES VERSION ${MAC_SOVERSION} SOVERSION ${MAC_SOVERSION})

    if(WIN32)
        if(${ARCHITECTURE} STREQUAL x86)
            target_link_options(MAC PRIVATE "-Wl,--add-stdcall-alias")
        endif()
        set_target_properties(MAC PROPERTIES OUTPUT_NAME MACDll)
        set(CMAKE_SHARED_LIBRARY_PREFIX "")
        set(CMAKE_IMPORT_LIBRARY_PREFIX "")
    else()
        target_compile_options(shared PRIVATE "-fPIC" "-fvisibility=hidden")
        target_compile_options(objects PRIVATE "-fPIC" "-fvisibility=hidden")
    endif()
else()
    add_library(MAC STATIC $<TARGET_OBJECTS:shared> $<TARGET_OBJECTS:objects>)
endif()

if(WIN32)
    target_compile_definitions(shared PUBLIC PLATFORM_WINDOWS)
    target_compile_definitions(objects PUBLIC PLATFORM_WINDOWS)
elseif(APPLE)
    target_compile_definitions(shared PUBLIC PLATFORM_APPLE)
    target_compile_definitions(objects PUBLIC PLATFORM_APPLE)
else()
    target_compile_definitions(shared PUBLIC PLATFORM_LINUX)
    target_compile_definitions(objects PUBLIC PLATFORM_LINUX)

    if(ANDROID)
        target_compile_definitions(shared PUBLIC PLATFORM_ANDROID)
        target_compile_definitions(objects PUBLIC PLATFORM_ANDROID)
    endif()
endif()

if(${ARCHITECTURE} STREQUAL x86 OR ${ARCHITECTURE} STREQUAL x86_64)
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 6.1))
        set_source_files_properties(Source/MACLib/NNFilterAVX512.cpp PROPERTIES COMPILE_FLAGS "-mavx512dq -mavx512bw")
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.7))
        set_source_files_properties(Source/MACLib/NNFilterAVX2.cpp PROPERTIES COMPILE_FLAGS "-mavx2")
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.3))
        set_source_files_properties(Source/MACLib/NNFilterSSE4.1.cpp PROPERTIES COMPILE_FLAGS "-msse4.1")
    endif()
    if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        set_source_files_properties(Source/MACLib/NNFilterSSE2.cpp PROPERTIES COMPILE_FLAGS "-msse2")
    endif()
endif()

#
# Set include folders
#
target_include_directories(shared PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/Source/Shared)

target_include_directories(objects PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/Source/MACLib
   ${CMAKE_CURRENT_SOURCE_DIR}/Source/Shared)

#
# Define utility target
#
if(BUILD_UTIL)
    set(MAC_UTIL_HEADER_FILES
       Source/Console/Console.h)

    set(MAC_UTIL_SOURCE_FILES
       Source/Console/Console.cpp)

    add_executable(macutil ${MAC_UTIL_HEADER_FILES} ${MAC_UTIL_SOURCE_FILES} $<TARGET_OBJECTS:shared>)
    target_link_libraries(macutil MAC)

    target_include_directories(macutil PUBLIC
       ${CMAKE_CURRENT_SOURCE_DIR}/Source/MACLib
       ${CMAKE_CURRENT_SOURCE_DIR}/Source/Shared)

    if(WIN32)
        target_compile_definitions(macutil PRIVATE PLATFORM_WINDOWS)
    elseif(APPLE)
        target_compile_definitions(macutil PRIVATE PLATFORM_APPLE)
    else()
        target_compile_definitions(macutil PRIVATE PLATFORM_LINUX)

        if(ANDROID)
    	    target_compile_definitions(macutil PRIVATE PLATFORM_ANDROID)
        endif()
    endif()

    if(WIN32)
        target_link_options(macutil PRIVATE "-municode" "-mconsole" "-Wl,--enable-stdcall-fixup")
        set_target_properties(macutil PROPERTIES OUTPUT_NAME MAC)
    else()
        set_target_properties(macutil PROPERTIES OUTPUT_NAME mac)
    endif()
endif()

#
# Define install targets
#
include(GNUInstallDirs)

install(FILES ${MAC_PUBLIC_HEADERS} DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/MAC")

if(BUILD_SHARED)
    install(TARGETS MAC LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}")
else()
    install(TARGETS MAC ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")
endif()

if(BUILD_UTIL)
    install(TARGETS macutil RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()

