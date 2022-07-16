#pragma once

#define ID_SET_COMPRESSION_FIRST    40000
#define ID_SET_COMPRESSION_LAST     50000

class MAC_FILE;

class IFormat
{
public:

    virtual ~IFormat() { }
    
    virtual CString GetName() = 0;

    virtual int Process(MAC_FILE * pInfo) = 0;

    virtual BOOL BuildMenu(CMenu * pMenu, int nBaseID) = 0;
    virtual BOOL ProcessMenuCommand(int nCommand) = 0;

    virtual CString GetInputExtensions(MAC_MODES Mode) = 0;
    virtual CString GetOutputExtension(MAC_MODES Mode, const CString & strInputFilename, int nLevel) = 0;
};