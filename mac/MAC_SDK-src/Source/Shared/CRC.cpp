#include "All.h"
#include "CRC.h"

namespace APE
{
#ifdef ENABLE_ARM_CRC32_OPT

// 0x04C11DB7 CRC-32 CRC32B, CRC32H, CRC32W, CRC32X
#define CRC32D(crc, value)  crc = __builtin_arm_crc32d(crc, value)
#define CRC32W(crc, value)  crc = __builtin_arm_crc32w(crc, value)
#define CRC32H(crc, value)  crc = __builtin_arm_crc32h(crc, value)
#define CRC32B(crc, value)  crc = __builtin_arm_crc32b(crc, value)

uint32 CRC_update(uint32 crc, const unsigned char * pData, int nBytes)
{
    // aligning loop
    while (((((intptr_t)pData) & (sizeof(uint64)-1)) != 0) && (nBytes != 0))
    {
        CRC32B(crc, *pData++);
        --nBytes;
    }

    // 64-bit aligned loop
    while (nBytes >= sizeof(uint64))
    {
        CRC32D(crc, *(const uint64 *)pData);

        nBytes -= sizeof(uint64);
        pData += sizeof(uint64);
    }

    // leftover
    if (nBytes & sizeof(uint32))
    {
        CRC32W(crc, *(const uint32 *)pData);
        pData += sizeof(uint32);
    }
    if (nBytes & sizeof(uint16))
    {
        CRC32H(crc, *(const uint16 *)pData);
        pData += sizeof(uint16);
    }
    if (nBytes & sizeof(uint8))
        CRC32B(crc, *pData);

    return crc;
}

#else

static uint32 CRC32_TABLE[8][256];

static uint32 CRC_reflect(uint32 ref, char ch)
{
    uint32 value = 0;

    for (int i = 1; i < (ch + 1); ++i)
    {
        if (ref & 1) value |= 1 << (ch - i);
        ref >>= 1;
    }

    return value;
}

static bool CRC_init()
{
    uint32 polynomial = 0x4c11db7;

    for (int i = 0; i <= 0xFF; i++)
    {
        uint32 crc = CRC_reflect(i, 8) << 24;

        for (int j = 0; j < 8; j++)
            crc = (crc << 1) ^ ((crc & ((unsigned int) 1 << 31)) ? polynomial : 0);

        CRC32_TABLE[0][i] = CRC_reflect(crc, 32);
    }

    for (int i = 0; i <= 0xFF; i++)
        for (int j = 1; j < 8; j++)
            CRC32_TABLE[j][i] = CRC32_TABLE[0][CRC32_TABLE[j - 1][i] & 0xFF] ^ (CRC32_TABLE[j - 1][i] >> 8);

    return true;
}

static bool CRC_initialized = CRC_init();

uint32 CRC_update(uint32 crc, const unsigned char * pData, int nBytes)
{
    while (nBytes >= 8)
    {
        crc  ^= pData[3] << 24 | pData[2] << 16 | pData[1] << 8 | pData[0];

        crc   =    CRC32_TABLE[7][ crc       & 0xFF] ^ CRC32_TABLE[6][(crc >>  8) & 0xFF] ^
            CRC32_TABLE[5][(crc >> 16) & 0xFF] ^ CRC32_TABLE[4][ crc >> 24          ] ^
            CRC32_TABLE[3][pData[4]         ] ^ CRC32_TABLE[2][pData[5]          ] ^
            CRC32_TABLE[1][pData[6]         ] ^ CRC32_TABLE[0][pData[7]          ];

        pData += 8;
        nBytes -= 8;
    }

    while (nBytes--) crc = (crc >> 8) ^ CRC32_TABLE[0][(crc & 0xFF) ^ *pData++];

    return crc;
}

#endif
}
