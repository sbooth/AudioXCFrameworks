#pragma once

namespace APE
{

/**************************************************************************************************
CFLACTag
**************************************************************************************************/
class CFLACTag
{
public:
    CFLACTag(const TCHAR * pFilename);

    CMap<CString, LPCTSTR, CString, LPCTSTR> m_mapFields;
    CSmartPtr<char> m_spPicture;
    uint32 m_nPictureBytes;

protected:
    uint32 ReadInteger(CSmartPtr<char>& spBuffer, uint32 nByte, bool bSwap = true);
};

}