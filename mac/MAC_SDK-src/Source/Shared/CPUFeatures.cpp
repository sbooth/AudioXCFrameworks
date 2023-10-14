#include "All.h"
#include "CPUFeatures.h"

#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    #include <intrin.h>
    #define MSVC_CPUID
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
    #if __GNUC__ >= 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
        #include <cpuid.h>
        #define GNUC_CPUID
    #endif
#endif

#if !(defined(__AVX512DQ__) && defined(__AVX512BW__))
    #define CPUID_XGETBV_LEVEL    1
    #define CPUID_XGETBV_REGISTER 2
    #define CPUID_XGETBV_MASK     0x0C000000
#endif

#if !defined(__SSE2__) && !defined(_M_X64)
    #define CPUID_SSE2_LEVEL      1
    #define CPUID_SSE2_REGISTER   3
    #define CPUID_SSE2_MASK       0x04000000
#endif

#if !defined(__SSE4_1__)
    #define CPUID_SSE41_LEVEL     1
    #define CPUID_SSE41_REGISTER  2
    #define CPUID_SSE41_MASK      0x00080000
#endif

#if !defined(__AVX2__)
    #define XSTATE_FLAGS_AVX      0x06

    #define CPUID_AVX2_LEVEL      7
    #define CPUID_AVX2_REGISTER   1
    #define CPUID_AVX2_MASK       0x00000020
#endif

#if !(defined(__AVX512DQ__) && defined(__AVX512BW__))
    #define XSTATE_FLAGS_AVX512   0xE6

    #define CPUID_AVX512_LEVEL    7
    #define CPUID_AVX512_REGISTER 1
    #define CPUID_AVX512_MASK     0x40030000
#endif

namespace APE
{

static bool GetCPUInfo(uint32 * cpuInfo, int level)
{
#if defined(MSVC_CPUID)
    int levelInfo[4] = { 0, 0, 0, 0 };
    __cpuid(levelInfo, 0);
    if (levelInfo[0] < level)
        return false;

    __cpuid(reinterpret_cast<int *>(cpuInfo), level);
    return true;
#elif defined(GNUC_CPUID)
    if(__get_cpuid_count(level, 0, &cpuInfo[0], &cpuInfo[1], &cpuInfo[2], &cpuInfo[3]) == 0)
        return false;

    return true;
#else
    (void) cpuInfo; (void) level;
    return false;
#endif
}

static uint32 GetXStateFlags()
{
    uint32 xcr0 = 0;

#if defined(MSVC_CPUID)
    xcr0 = static_cast<uint32>(_xgetbv(_XCR_XFEATURE_ENABLED_MASK));
#elif defined(GNUC_CPUID)
    __asm__("xgetbv" : "=a" (xcr0) : "c" (0) : "%edx");
#endif

    return xcr0;
}

bool GetSSE2Supported()
{
#if defined(__SSE2__) || defined(_M_X64)
    return true;
#else
    unsigned int cpuInfo[4] = { 0, 0, 0, 0 };
    if (!GetCPUInfo(cpuInfo, CPUID_SSE2_LEVEL))
        return false;

    if ((cpuInfo[CPUID_SSE2_REGISTER] & CPUID_SSE2_MASK) == CPUID_SSE2_MASK)
        return true;

    return false;
#endif
}

bool GetSSE41Supported()
{
#if defined(__SSE4_1__)
    return true;
#else
    unsigned int cpuInfo[4] = { 0, 0, 0, 0 };
    if (!GetCPUInfo(cpuInfo, CPUID_SSE41_LEVEL))
        return false;

    if ((cpuInfo[CPUID_SSE41_REGISTER] & CPUID_SSE41_MASK) == CPUID_SSE41_MASK)
        return true;

    return false;
#endif
}

bool GetAVX2Supported()
{
#if defined(__AVX2__)
    return true;
#else
    unsigned int cpuInfo[4] = { 0, 0, 0, 0 };
    if (!GetCPUInfo(cpuInfo, CPUID_AVX2_LEVEL))
        return false;

    if ((cpuInfo[CPUID_AVX2_REGISTER] & CPUID_AVX2_MASK) == CPUID_AVX2_MASK)
    {
        if (!GetCPUInfo(cpuInfo, CPUID_XGETBV_LEVEL))
            return false;

        if ((cpuInfo[CPUID_XGETBV_REGISTER] & CPUID_XGETBV_MASK) == CPUID_XGETBV_MASK)
        {
            if ((GetXStateFlags() & XSTATE_FLAGS_AVX) == XSTATE_FLAGS_AVX)
                return true;
        }
    }

    return false;
#endif
}

bool GetAVX512Supported()
{
#if defined(__AVX512DQ__) && defined(__AVX512BW__)
    return true;
#else
    unsigned int cpuInfo[4] = { 0, 0, 0, 0 };
    if (!GetCPUInfo(cpuInfo, CPUID_AVX512_LEVEL))
        return false;

    if ((cpuInfo[CPUID_AVX512_REGISTER] & CPUID_AVX512_MASK) == CPUID_AVX512_MASK)
    {
        if (!GetCPUInfo(cpuInfo, CPUID_XGETBV_LEVEL))
            return false;

        if ((cpuInfo[CPUID_XGETBV_REGISTER] & CPUID_XGETBV_MASK) == CPUID_XGETBV_MASK)
        {
            if ((GetXStateFlags() & XSTATE_FLAGS_AVX512) == XSTATE_FLAGS_AVX512)
                return true;
        }
    }

    return false;
#endif
}

bool GetNeonSupported()
{
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64)
    return true;
#else
    return false;
#endif
}

}
