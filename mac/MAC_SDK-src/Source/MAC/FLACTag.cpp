#include "stdafx.h"
#include "MAC.h"
#include "All.h"
#include "FLACTag.h"
#include "IO.h"

namespace APE
{

#define Swap4Bytes(val) \
((((val) >> 24) & 0x000000FF) | (((val) >> 8) & 0x0000FF00) | \
(((val) << 8) & 0x00FF0000) | (((val) << 24) & 0xFF000000))

/**************************************************************************************************
CFLACTag
**************************************************************************************************/
CFLACTag::CFLACTag(const TCHAR * pFilename)
{
    // initiliaze
    m_nPictureBytes = 0;

    // open the file
    CSmartPtr<APE::CIO> m_spIO;
    m_spIO.Assign(CreateCIO());
    if (m_spIO->Open(pFilename) == ERROR_SUCCESS)
    {
        // read FLAC header
        unsigned int nBytesRead = 4;
        char cFLAC[5] = { 0 };
        m_spIO->Read(cFLAC, 4, &nBytesRead);
        if (strcmp(cFLAC, "fLaC") == 0)
        {
            // loop chunks
            bool bReadImage = false;
            bool bReadTag = false;
            while (!bReadImage || !bReadTag)
            {
                // read the buffer
                unsigned char cBuffer[4] = { 0 };
                if (m_spIO->Read(cBuffer, 4, &nBytesRead) != ERROR_SUCCESS)
                    break;

                // buffer size
                uint32 nBufferBytes = 0;
                {
                    // flip the buffer
                    for (int z = 0; z < 3; z++)
                        nBufferBytes = (nBufferBytes << 8) | static_cast<uint32>(cBuffer[1 + z]);

                    // get type and last
                    //int nLast = static_cast<int>(cBuffer[0] & 0x80);
                    int nType = static_cast<int>(cBuffer[0] & (0x80 - 1));

                    // keep start position of the reader
                    int64 nOriginalPosition = m_spIO->GetPosition();

                    // switch based on the type
                    if (nType == 4) // FLAC__METADATA_TYPE_VORBIS_COMMENT
                    {
                        // tag
                        CSmartPtr<char> spTag(new char [nBufferBytes + 1], true);
                        m_spIO->Read(spTag, nBufferBytes, &nBytesRead);

                        // skip vendor
                        uint32 nStartByte = 0;
                        {
                            uint32 nBytes = ReadInteger(spTag, nStartByte, false);

                            CSmartPtr<char> spVendor(new char [nBytes + 1], true);
                            memcpy(spVendor, &spTag[nStartByte + 4], nBytes);
                            spVendor[nBytes] = 0;

                            nStartByte += nBytes + 4;
                        }

                        // read fields
                        uint32 nFields = ReadInteger(spTag, nStartByte, false);

                        // increment start byte
                        nStartByte += 4;

                        for (uint32 nField = 0; nField < nFields; nField++)
                        {
                            uint32 nBytes = ReadInteger(spTag, nStartByte, false);

                            CSmartPtr<APE::str_utf8> spChar(new APE::str_utf8[nBytes + 1], true);
                            memcpy(spChar, &spTag[nStartByte + 4], nBytes);
                            spChar[nBytes] = 0;

                            CString strTag = CAPECharacterHelper::GetUTF16FromUTF8(spChar.GetPtr());
                            int nEqual = strTag.Find(_T("="), 0);
                            if (nEqual >= 0)
                            {
                                CString strKey = strTag.Left(nEqual);
                                CString strValue = strTag.Mid(nEqual + 1);

                                m_mapFields.SetAt(strKey, strValue);
                            }

                            nStartByte += nBytes + 4;
                        }

                        bReadTag = true;
                    }
                    else if (nType == 6) // FLAC__METADATA_TYPE_PICTURE
                    {
                        // picture
                        CSmartPtr<char> spPicture(new char [nBufferBytes], true);
                        m_spIO->Read(spPicture, nBufferBytes, &nBytesRead);

                        // picture type
                        //int nPictureType = ReadInteger(spPicture, 0);

                        // MIME
                        uint32 nMIMELength = ReadInteger(spPicture, 4);
                        CSmartPtr<char> spMimeType(new char [nMIMELength + 1], true);
                        memcpy(spMimeType, &spPicture[8], nMIMELength);
                        spMimeType[nMIMELength] = 0;

                        if (_stricmp(spMimeType, "image/jpeg") == 0)
                        {
                            // description
                            uint32 nDescriptionLength = ReadInteger(spPicture, 8 + nMIMELength);
                            // we don't read the description because it's not used
                            
                            // width
                            //uint32 nWidth = ReadInteger(spPicture, 12 + nMIMELength + nDescriptionLength);

                            // height
                            //uint32 nHeight = ReadInteger(spPicture, 16 + nMIMELength + nDescriptionLength);

                            // depth
                            //uint32 nDepth = ReadInteger(spPicture, 20 + nMIMELength + nDescriptionLength);

                            // index
                            //uint32 nIndex = ReadInteger(spPicture, 24 + nMIMELength + nDescriptionLength);

                            // length
                            m_nPictureBytes = ReadInteger(spPicture, 28 + nMIMELength + nDescriptionLength);

                            // read data
                            if ((32 + nMIMELength + nDescriptionLength + m_nPictureBytes) <= nBufferBytes)
                            {
                                m_spPicture.Assign(new char [m_nPictureBytes], true);
                                memcpy(m_spPicture, &spPicture[32 + nMIMELength + nDescriptionLength], m_nPictureBytes);
                            }
                        }

                        bReadImage = true;
                    }

                    // skip the buffer
                    m_spIO->Seek(nOriginalPosition + nBufferBytes, APE::SeekFileBegin);
                }
            }
        }
    }
}

uint32 CFLACTag::ReadInteger(CSmartPtr<char> & spBuffer, uint32 nByte, bool bSwap)
{
    uint32 nValue = *(reinterpret_cast<uint32 *>(&spBuffer[nByte]));
    if (bSwap)
        nValue = Swap4Bytes(nValue);
    return nValue;
}

}
