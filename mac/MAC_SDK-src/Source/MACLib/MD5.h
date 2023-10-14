/*
 *  Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All rights reserved.
 *
 *  License to copy and use this software is granted provided that it is identified
 *  as the "RSA Data Security, Inc. MD5 Message-Digest Algorithm" in all material
 *  mentioning or referencing this software or this function.
 *
 *  License is also granted to make and use derivative works provided that such
 *  works are identified as "derived from the RSA Data Security, Inc. MD5 Message-
 *  Digest Algorithm" in all material mentioning or referencing the derived work.
 *
 *  RSA Data Security, Inc. makes no representations concerning either the
 *  merchantability of this software or the suitability of this software for any
 *  particular purpose. It is provided "as is" without express or implied warranty
 *  of any kind. These notices must be retained in any copies of any part of this
 *  documentation and/or software.
 */

#pragma once

namespace APE
{

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

/*
 *  Define the MD5 context structure
 *  Please DO NOT change the order or contents of the structure as various assembler files depend on it !!
 */

typedef struct {
   uint32_t  state  [ 4];     /* state (ABCD) */
   uint32_t  count  [ 2];     /* number of bits, modulo 2^64 (least sig word first) */
   uint8_t   buffer [64];     /* input buffer for incomplete buffer data */
} MD5_CTX;

void   MD5Init   ( MD5_CTX* ctx );
void   MD5Update ( MD5_CTX* ctx, const uint8_t* buf, int64 len );
void   MD5Final  ( uint8_t digest [16], MD5_CTX* ctx );

#pragma pack(push, 1)

class CMD5Helper
{
public:
    CMD5Helper();

#ifdef APE_ENABLE_MD5_ADD_DATA
    __forceinline void AddData(const void * pData, int64 nBytes)
    {
        MD5Update(&m_MD5Context, static_cast<const unsigned char *>(pData), nBytes);
        m_nTotalBytes += nBytes;
    }
#endif
    bool GetResult(unsigned char cResult[16]);

protected:
    MD5_CTX m_MD5Context;
    int64 m_nTotalBytes;
};

#pragma pack(pop)

}
