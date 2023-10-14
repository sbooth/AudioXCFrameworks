#include "stdafx.h"
#include "Filename.h"
#include "CharacterHelper.h"
using namespace APE;

void FixDirectory(CString & strDirectory)
{
    if (strDirectory.Right(1) != _T("\\"))
        strDirectory += _T("\\");
}

CString GetInstallPath()
{
    CRegKey reg;
    #if defined(_M_X64)
        reg.Open(HKEY_LOCAL_MACHINE, _T("Software\\Wow6432Node\\Monkey's Audio x64"), KEY_READ);
    #else
        reg.Open(HKEY_LOCAL_MACHINE, _T("Software\\Monkey's Audio"), KEY_READ);
    #endif
    CString strInstallDirectory;
    ULONG nChars = APE_MAX_PATH;
    reg.QueryStringValue(_T("Install_Dir"), strInstallDirectory.GetBuffer(static_cast<int>(APE_MAX_PATH)), &nChars);
    strInstallDirectory.ReleaseBuffer();
    FixDirectory(strInstallDirectory);
    return strInstallDirectory;
}

CString GetProgramPath(BOOL bAppendProgramName)
{
    CString strProgramPath;
    GetModuleFileName(NULL, strProgramPath.GetBuffer(_MAX_PATH), _MAX_PATH);
    strProgramPath.ReleaseBuffer();

    if (bAppendProgramName == FALSE)
    {
        CFilename fnProgram(strProgramPath);
        strProgramPath = fnProgram.GetPath();
    }

    return strProgramPath;
}

CString GetUserDataPath()
{
    size_t nRequired = 0;
    _tgetenv_s(&nRequired, NULL, 0, _T("APPDATA"));

    CSmartPtr<wchar_t> spAppData(new wchar_t[nRequired + 1], true);
    memset(spAppData, 0, nRequired + 1);
    _tgetenv_s(&nRequired, spAppData, nRequired, _T("APPDATA"));

    CString strPath;
    strPath = spAppData;
    FixDirectory(strPath);
    strPath += _T("Monkey's Audio\\");

    return strPath;
}

void CreateDirectoryEx(CString strDirectory)
{
    // quit if it's an empty string
    if (strDirectory.IsEmpty())
        return;

    // remove ending / if exists
    if (strDirectory.Right(1) == _T("\\"))
        strDirectory = strDirectory.Left(strDirectory.GetLength() - 1);

    // base case if directory exists
    if (GetFileAttributes(strDirectory) != INVALID_FILE_ATTRIBUTES)
        return;

    // recursive call, one less directory
    int nFound = strDirectory.ReverseFind('\\');
    CreateDirectoryEx(strDirectory.Left(nFound));

    // actual work
    CreateDirectory(strDirectory,NULL);
}

void ListFiles(CStringArray * pStringArray, CString strPath, BOOL bRecurse)
{
    FixDirectory(strPath);

    WIN32_FIND_DATA WFD;
    HANDLE hFind = FindFirstFile(strPath + _T("*.*"), &WFD);

    BOOL bFindSuccess = TRUE;

    while (bFindSuccess)
    {
        CString strFilename = WFD.cFileName;
        if (strFilename != "." && strFilename != _T(".."))
        {
            if (WFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                if (bRecurse)
                    ListFiles(pStringArray, strPath + strFilename + _T("\\"), TRUE);
            }
            else
            {
                pStringArray->Add(strPath + strFilename);
            }

        }

        bFindSuccess = FindNextFile(hFind, &WFD);
    }

    FindClose(hFind);
}

BOOL FileExists(const CString & strFilename)
{
    WIN32_FIND_DATA WFD; BOOL bExists = FALSE;
    HANDLE hFind = FindFirstFile(strFilename, &WFD);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        bExists = TRUE;
        FindClose(hFind);
    }

    return bExists;
}

CString GetUniqueFilename(CString strFilename)
{
    // if the file doesn't exist, just return
    if (FileExists(strFilename) == FALSE)
        return strFilename;

    // trim the number off the end
    int nNumber = 0;

    CFilename fnFilename(strFilename);

    CString strName = fnFilename.GetName();
    if (strName.Right(1) == _T(")"))
    {
        int nLeft = strName.ReverseFind('(');
        CString strNumber = strName.Mid(nLeft + 1, strName.GetLength() - nLeft - 2);

        BOOL bNumber = TRUE;
        for (int z = 0; z < strNumber.GetLength(); z++)
        {
            if (isdigit(strNumber[z]) == FALSE)
                bNumber = FALSE;
        }

        if (bNumber)
        {
            nNumber = _ttoi(strNumber);
            strName = strName.Left(nLeft);
            strName.TrimRight(' ');
        }
    }

    // keep adding numbers until it's a unique filename
    while (TRUE)
    {
        nNumber++;

        CString strTemp; strTemp.Format(_T("%s (%d)"), strName.GetString(), nNumber);
        CString strNewFilename = fnFilename.BuildFilename(NULL, NULL, strTemp, NULL);

        if (FileExists(strNewFilename) == FALSE)
            return strNewFilename;
    }

    return _T("");
}

double GetFileBytes(const CString & strFilename)
{
    double dFileBytes = 0;

    WIN32_FIND_DATA WFD;
    HANDLE hFind = FindFirstFile(strFilename, &WFD);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        dFileBytes = WFD.nFileSizeLow + (static_cast<double>(WFD.nFileSizeHigh) * static_cast<double>(4294967296.0));
        FindClose(hFind);
    }

    return dFileBytes;
}

CString GetExtension(LPCTSTR pFilename)
{
    const TCHAR * pDot = _tcsrchr(pFilename, '.');
    CString strExtension = pDot ? &pDot[0] : _T("");
    strExtension.MakeLower();
    return strExtension;
}

CString GetDirectory(LPCTSTR pFilename)
{
    const TCHAR * pDot = _tcsrchr(pFilename, '\\');
    return (pDot) ? CString(pFilename, static_cast<int>(pDot - pFilename + 1)) : _T("");
}

double GetDriveFreeMB(CString strDrive)
{
    double dFreeMB = -1;

    DWORD dwSectorsPerCluster = 0;
    DWORD dwBytesPerSector = 0;
    DWORD dwFreeClusters = 0;
    DWORD dwTotalClusters = 0;

    strDrive = strDrive.Left(1) + _T(":\\");
    if (GetDiskFreeSpace(strDrive, &dwSectorsPerCluster, &dwBytesPerSector, &dwFreeClusters, &dwTotalClusters))
    {
        double dFreeBytes = static_cast<double>(dwFreeClusters) * static_cast<double>(dwSectorsPerCluster) * static_cast<double>(dwBytesPerSector);
        dFreeMB = dFreeBytes / (static_cast<double>(1024) * static_cast<double>(1024));
    }

    return dFreeMB;
}

BOOL MoveFile(const CString & strExistingFilename, const CString & strNewFilename, BOOL bOverwrite)
{
    BOOL bRetVal = FALSE;

    if (FileExists(strNewFilename) && bOverwrite)
    {
        CString strTempFilename = GetUniqueFilename(strNewFilename);
        if (MoveFile(strNewFilename, strTempFilename))
        {
            if (MoveFile(strExistingFilename, strNewFilename))
            {
                bRetVal = TRUE;
                DeleteFileEx(strTempFilename);
            }
            else
            {
                MoveFile(strTempFilename, strNewFilename);
            }
        }
    }
    else
    {
        bRetVal = MoveFile(strExistingFilename, strNewFilename);
    }

    return bRetVal;
}

BOOL CopyFileTime(const CString & strSourceFilename, const CString & strDestinationFilename)
{
    BOOL bRetVal = FALSE;

    WIN32_FIND_DATA wfdInput;
    HANDLE hFind = FindFirstFile(strSourceFilename, &wfdInput);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        FindClose(hFind);
        HANDLE hOutput = CreateFile(strDestinationFilename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hOutput != INVALID_HANDLE_VALUE)
        {
            if (SetFileTime(hOutput, &wfdInput.ftCreationTime, &wfdInput.ftLastAccessTime, &wfdInput.ftLastWriteTime))
                bRetVal = TRUE;

            CloseHandle(hOutput);
        }
    }

    return bRetVal;
}

BOOL RecycleFile(const CString & strFilename, BOOL bConfirm)
{
    // setup the SHFILEOPSTRUCT
    SHFILEOPSTRUCT ShellFileOp; APE_CLEAR(ShellFileOp);
    ShellFileOp.wFunc = FO_DELETE;
    ShellFileOp.fFlags = FOF_ALLOWUNDO | (bConfirm ? 0 : FOF_NOCONFIRMATION);

    // we need  a double-null terminated string
    TCHAR cFrom[_MAX_PATH + 8] = {0}; _tcsncpy_s(cFrom, _MAX_PATH + 8, strFilename, _MAX_PATH);
    ShellFileOp.pFrom = cFrom;

    // run
    return (SHFileOperation(&ShellFileOp) == 0) ? TRUE : FALSE;
}

BOOL ReadWholeFile(const CString & strFilename, CString & strBuffer)
{
    BOOL bRetVal = FALSE;
    strBuffer.Empty();

    // open file
    HANDLE hFile = CreateFile(strFilename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        // build buffer for the data
        unsigned int nBytes = GetFileSize(hFile, NULL);
        CSmartPtr<char> spUTF8(new char [static_cast<size_t>(nBytes) + 1], TRUE);

        if (spUTF8 != NULL)
        {
            // read the file
            DWORD dwBytesRead = 0;
            if (ReadFile(hFile, spUTF8, nBytes, &dwBytesRead, NULL) && (dwBytesRead == nBytes))
            {
                // null-terminate
                spUTF8[dwBytesRead] = 0;

                // convert to UTF-16
                CSmartPtr<TCHAR> spUTF16(CAPECharacterHelper::GetUTF16FromUTF8(reinterpret_cast<unsigned char *>(spUTF8.GetPtr())), TRUE);
                if (spUTF16)
                {
                    strBuffer = spUTF16;
                    bRetVal = TRUE;
                }
            }
        }

        // close
        CloseHandle(hFile);
    }

    return bRetVal;
}

BOOL ExecuteProgramBlocking(CString strApplication, CString strParameters, int * pnExitCode, BOOL bShowPopup, CString * pstrReturnOutput)
{
    BOOL bRetVal = FALSE;
    if (pnExitCode) *pnExitCode = -1;

    STARTUPINFO StartupInfo; APE_CLEAR(StartupInfo);
    StartupInfo.cb = sizeof(StartupInfo);
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.wShowWindow = bShowPopup ? SW_SHOW : SW_HIDE;

    PROCESS_INFORMATION ProcessInfo; APE_CLEAR(ProcessInfo);

    CString strCommand = strApplication + _T(" ") + strParameters;

    // create pipes if we want to capture the output
    HANDLE hPipeStdOut_Wd = INVALID_HANDLE_VALUE;
    HANDLE hPipeStdOut_Rd = INVALID_HANDLE_VALUE;
    if (pstrReturnOutput != NULL)
    {
        SECURITY_ATTRIBUTES saAttr;
        APE_CLEAR(saAttr);
        saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
        saAttr.bInheritHandle = TRUE;
        saAttr.lpSecurityDescriptor = NULL;

        if (!CreatePipe(&hPipeStdOut_Rd, &hPipeStdOut_Wd, &saAttr, 0))
            return FALSE;

        if (!SetHandleInformation(hPipeStdOut_Rd, HANDLE_FLAG_INHERIT, 0))
        {
            APE_SAFE_FILE_CLOSE(hPipeStdOut_Rd)
            APE_SAFE_FILE_CLOSE(hPipeStdOut_Wd)
            return FALSE;
        }

        StartupInfo.hStdOutput = hPipeStdOut_Wd;
        StartupInfo.hStdError = hPipeStdOut_Wd;
        StartupInfo.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
        StartupInfo.dwFlags |= STARTF_USESTDHANDLES;
    }

    // create the process
    if (CreateProcess(NULL, strCommand.LockBuffer(), NULL, NULL,
        (StartupInfo.dwFlags & STARTF_USESTDHANDLES) ? TRUE : FALSE, NORMAL_PRIORITY_CLASS, NULL, CFilename(strApplication).GetPath(), &StartupInfo, &ProcessInfo))
    {
        // wait for completion
        WaitForSingleObject(ProcessInfo.hProcess, INFINITE);

        // read the output
        if (pstrReturnOutput != NULL)
        {
            pstrReturnOutput->Empty();

            int nBufferBytes = 65536;
            CSmartPtr<UCHAR> spBuffer(new UCHAR[static_cast<size_t>(nBufferBytes)], true);
            DWORD dwRead = 0;

            // close the write pipe so reads finish
            APE_SAFE_FILE_CLOSE(hPipeStdOut_Wd)

            CString strBuffer;
            while (true)
            {
                bool bRead = ReadFile(hPipeStdOut_Rd, spBuffer, static_cast<DWORD>(nBufferBytes - 1), &dwRead, NULL);
                spBuffer[dwRead] = 0; // null terminate (after the amount of data read -- which will always be less than the buffer size)
                if (!bRead || dwRead == 0)
                    break;

                CString strLine;
                strLine = CAPECharacterHelper::GetUTF16FromUTF8(spBuffer);
                pstrReturnOutput->Append(strLine);
            }
        }

        // get the exit code
        if (pnExitCode)
        {
            DWORD dwExit = 0;
            if (GetExitCodeProcess(ProcessInfo.hProcess, &dwExit))
                *pnExitCode = static_cast<int>(dwExit);
        }

        // close process
        CloseHandle(ProcessInfo.hProcess);
        CloseHandle(ProcessInfo.hThread);

        bRetVal = TRUE;
    }

    // close handles
    APE_SAFE_FILE_CLOSE(hPipeStdOut_Rd)
    APE_SAFE_FILE_CLOSE(hPipeStdOut_Wd)

    strCommand.UnlockBuffer();

    return bRetVal;
}

BOOL IsProcessElevated()
{
    BOOL fIsElevated = FALSE;
    HANDLE hToken = NULL;
    DWORD dwSize = 0;

    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
    {
        TOKEN_ELEVATION elevation;
        APE_CLEAR(elevation);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, sizeof(elevation), &dwSize))
        {
            fIsElevated = static_cast<BOOL>(elevation.TokenIsElevated);
        }
    }

    if (hToken)
    {
        CloseHandle(hToken);
        hToken = NULL;
    }
    return fIsElevated;
}

BOOL DeleteFileEx(LPCTSTR pFilename)
{
    // we can't delete a file if the attributes are hidden, so we'll reset those before doing the delete
    SetFileAttributes(pFilename, FILE_ATTRIBUTE_NORMAL);
    return DeleteFile(pFilename);
}
