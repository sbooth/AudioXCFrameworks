#pragma once

class CAPLHelper  
{
public:
    CAPLHelper();
    virtual ~CAPLHelper();

    BOOL GenerateLinkFiles(const CString & strImage, const CString & strNamingTemplate);

protected:
    int64 GetAPESampleRate(const CString & strFilename);
};
