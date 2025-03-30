// ----------------------------------------------------------------------------

#include <streams.h>
#include <initguid.h>
#include <mmreg.h>
#include <tchar.h>
#include "RegistryUtils.h"

// ----------------------------------------------------------------------------

HRESULT GUID2String(TCHAR *DstString, const GUID SrcGuid)
{
    OLECHAR CLSIDOLEString[CHARS_IN_GUID];
    HRESULT hr = StringFromGUID2(SrcGuid, CLSIDOLEString, CHARS_IN_GUID);
    if (FAILED(hr))
        return hr;
    wsprintf(DstString, TEXT("%ls"), CLSIDOLEString);
    return S_OK;
}

// ----------------------------------------------------------------------------

void RegisterSourceFilterExtension(const TCHAR* Extension,
    const GUID SourceFilterGUID,
    const GUID MediaType,
    const GUID Subtype)
{
    // Identification by extension
    // HKEY_CLASSES_ROOT\Media Type\Extensions\.ext

    HKEY Key;
    DWORD Disp;
    TCHAR RegistryKeyName[256];
    TCHAR CLSIDString[CHARS_IN_GUID];

    wsprintf(RegistryKeyName, _T("Media Type\\Extensions\\%s"), Extension);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT,
        RegistryKeyName, 0, _T("REG_SZ"), REG_OPTION_NON_VOLATILE, KEY_WRITE,
        NULL, &Key, &Disp))
    {
        GUID2String(CLSIDString, SourceFilterGUID);
        RegSetValueEx(Key, _T("Source Filter"), 0, REG_SZ,
            reinterpret_cast<CONST BYTE *>(CLSIDString), static_cast<DWORD>(_tcslen(CLSIDString)));

        if (!IsEqualGUID(MediaType, CLSID_NULL))
        {
            GUID2String(CLSIDString, MediaType);
            RegSetValueEx(Key, _T("Media Type"), 0, REG_SZ,
                reinterpret_cast<CONST BYTE*>(CLSIDString), static_cast<DWORD>(_tcslen(CLSIDString)));
        }

        if (!IsEqualGUID(MediaType, CLSID_NULL))
        {
            GUID2String(CLSIDString, Subtype);
            RegSetValueEx(Key, _T("Subtype"), 0, REG_SZ,
                reinterpret_cast<CONST BYTE *>(CLSIDString), static_cast<DWORD>(_tcslen(CLSIDString)));
        }
        RegCloseKey(Key);
    }
}

// ----------------------------------------------------------------------------

void UnRegisterSourceFilterExtension(const TCHAR* Extension)
{
    TCHAR RegistryKeyName[256];
    wsprintf(RegistryKeyName, _T("Media Type\\Extensions\\%s"), Extension);
    RegDeleteKey(HKEY_CLASSES_ROOT, RegistryKeyName);
}

// ----------------------------------------------------------------------------

void RegisterSourceFilterPattern(const char* Pattern,
    const GUID SourceFilterGUID,
    const GUID MajorType,
    const GUID Subtype)
{
    // Identification by content :
    // HKEY_CLASSES_ROOT\MediaType\{major type}\{subtype}

    HKEY Key;
    DWORD Disp;
    TCHAR RegistryKeyName[256];
    TCHAR CLSIDString[CHARS_IN_GUID], CLSIDString2[CHARS_IN_GUID];

    GUID2String(CLSIDString, MajorType);
    GUID2String(CLSIDString2, Subtype);
    wsprintf(RegistryKeyName, _T("Media Type\\%s\\%s"), CLSIDString, CLSIDString2);
    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_CLASSES_ROOT,
        RegistryKeyName, 0, _T("REG_SZ"), REG_OPTION_NON_VOLATILE, KEY_WRITE,
        NULL, &Key, &Disp))
    {
        GUID2String(CLSIDString, SourceFilterGUID);
        RegSetValueEx(Key, _T("Source Filter"), 0, REG_SZ,
            reinterpret_cast<CONST BYTE *>(CLSIDString), static_cast<DWORD>(_tcslen(CLSIDString)));

        // The pattern use the following format : offset,cb,mask,val
        RegSetValueEx(Key, _T("0"), 0, REG_SZ, reinterpret_cast<CONST BYTE *>(Pattern), static_cast<DWORD>(strlen(Pattern)));
        RegCloseKey(Key);
    }
}

// ----------------------------------------------------------------------------

void UnRegisterSourceFilterPattern(const GUID MajorType,
    const GUID Subtype)
{
    TCHAR RegistryKeyName[256];
    TCHAR CLSIDString[CHARS_IN_GUID], CLSIDString2[CHARS_IN_GUID];

    GUID2String(CLSIDString, MajorType);
    GUID2String(CLSIDString2, Subtype);
    wsprintf(RegistryKeyName, _T("Media Type\\%s\\%s"), CLSIDString, CLSIDString2);
    RegDeleteKey(HKEY_CLASSES_ROOT, RegistryKeyName);
}

// ----------------------------------------------------------------------------
TCHAR* GetToken(TCHAR* src, const TCHAR* sep, int& position) {
    TCHAR* res = src + position;
    TCHAR* nextRes = wcsstr(res, sep);
    if (nextRes) {
        position += static_cast<int>(nextRes - res + 1);
        *nextRes = 0;
    }
    else {
        position = -1;
    }
    return res;
}

int ContainsExt(const TCHAR* Src, const TCHAR* Extension) {
    if (Src == NULL) {
        return 0;
    }
    TCHAR* SrcDup = _wcsdup(Src);
    int position = 0;
    while (position != -1) {
        TCHAR* token = GetToken(SrcDup, _T(";"), position);
        if (wcscmp(token, Extension) == 0) {
            free(SrcDup);
            return 1;
        }
    }
    free(SrcDup);
    return 0;
}

void RegisterWMPExtension(const TCHAR* Extension, const TCHAR* Description,
    const TCHAR* MUIDescription, const TCHAR* PerceivedType)
{
    HKEY Key;
    DWORD Disp;
    const TCHAR* ExtensionWithoutStar = Extension + 1;

    // WMP 6.4
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\Types"), 0,
        KEY_WRITE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &Key))
    {
        DWORD Index = 0, CurrentIndex = 0, MaxIndex = 0;
        TCHAR KeyName[256] = { 0 };
        DWORD KeyNameMaxLen = 256;
        TCHAR KeyValue[256] = { 0 };
        DWORD KeyValueMaxLen = 256;
        bool AlreadyRegistered = false;

        // Check if our extension is not already here, and get the new index if it's not
        while (ERROR_SUCCESS == RegEnumValue(Key, Index++, KeyName, &KeyNameMaxLen,
            NULL, NULL, reinterpret_cast<BYTE*>(KeyValue), &KeyValueMaxLen))
        {
            _wcslwr_s(KeyValue, 255);
            if (ContainsExt(KeyValue, Extension))
            {
                AlreadyRegistered = true;
                break;
            }
            CurrentIndex = _ttoi(KeyName);
            if (CurrentIndex > MaxIndex)
            {
                MaxIndex = CurrentIndex;
            }

            KeyNameMaxLen = 256;
            KeyValueMaxLen = 256;
        }

        if (!AlreadyRegistered)
        {
            wsprintf(KeyName, _T("%d"), MaxIndex + 1);
            // Add Extension
            RegSetValueEx(Key, KeyName, 0, REG_SZ, reinterpret_cast<CONST BYTE*>(Extension), static_cast<DWORD>(_tcslen(Extension)) * sizeof(TCHAR));
            RegCloseKey(Key);

            // Add Description
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                _T("SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\Descriptions"), 0, KEY_WRITE, &Key))
            {
                RegSetValueEx(Key, KeyName, 0, REG_SZ, reinterpret_cast<CONST BYTE*>(Description), static_cast<DWORD>(_tcslen(Description)) * sizeof(TCHAR));
                RegCloseKey(Key);
            }

            // Add MUIDescription
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                _T("SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\MUIDescriptions"), 0, KEY_WRITE, &Key))
            {
                RegSetValueEx(Key, KeyName, 0, REG_SZ, reinterpret_cast<CONST BYTE*>(MUIDescription), static_cast<DWORD>(_tcslen(MUIDescription)) * sizeof(TCHAR));
                RegCloseKey(Key);
            }
        }
        else {
            RegCloseKey(Key);
        }
    }

    // WMP9
    // From "File Name Extension Registry Settings"
    // http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wmplay10/mmp_sdk/filenameextensionregistrysettings.asp

    TCHAR RegistryKeyName[256];
    DWORD dwRuntimeFlag = 0x7;
    DWORD dwPermissionsFlag = 0xf;
    wsprintf(RegistryKeyName,
        _T("SOFTWARE\\Microsoft\\Multimedia\\WMPlayer\\Extensions\\%s"), ExtensionWithoutStar);

    if (ERROR_SUCCESS == RegCreateKeyEx(HKEY_LOCAL_MACHINE, RegistryKeyName,
        0, _T("REG_SZ"), REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &Key, &Disp))
    {
        RegSetValueEx(Key, _T("Runtime"), 0, REG_DWORD, reinterpret_cast<BYTE*>(&dwRuntimeFlag),
            sizeof(DWORD));
        RegSetValueEx(Key, _T("Permissions"), 0, REG_DWORD, reinterpret_cast<BYTE*>(&dwPermissionsFlag),
            sizeof(DWORD));
        if (PerceivedType)
        {
            RegSetValueEx(Key, _T("PerceivedType"), 0, REG_SZ, reinterpret_cast<const BYTE*>(PerceivedType),
                static_cast<DWORD>(_tcslen(PerceivedType)) * sizeof(TCHAR));
        }
        RegCloseKey(Key);
    }
}

// ----------------------------------------------------------------------------

void UnRegisterWMPExtension(const TCHAR* Extension)
{
    HKEY Key;
    TCHAR RegistryKeyName[256];
    const TCHAR* ExtensionWithoutStar = Extension + 1;

    // WMP 6.4
    if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
        _T("SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\Types"), 0,
        KEY_WRITE | KEY_ENUMERATE_SUB_KEYS | KEY_QUERY_VALUE, &Key))
    {
        DWORD Index = 0;
        TCHAR KeyName[256] = { 0 };
        DWORD KeyNameMaxLen = 256;
        TCHAR KeyValue[256] = { 0 };
        DWORD KeyValueMaxLen = 256;
        bool AlreadyRegistered = false;

        // Check if our extension is already here
        while (ERROR_SUCCESS == RegEnumValue(Key, Index++, KeyName, &KeyNameMaxLen,
            NULL, NULL, reinterpret_cast<BYTE*>(KeyValue), &KeyValueMaxLen))
        {
            _wcslwr_s(KeyValue, 255);
            if (ContainsExt(KeyValue, Extension))
            {
                AlreadyRegistered = true;
                break;
            }
            KeyNameMaxLen = 256;
            KeyValueMaxLen = 256;
        }
        if (AlreadyRegistered)
        {
            // Remove Extension
            RegDeleteValue(Key, KeyName);
            RegCloseKey(Key);

            // Remove Description
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                _T("SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\Descriptions"), 0, KEY_WRITE, &Key))
            {
                RegDeleteValue(Key, KeyName);
                RegCloseKey(Key);
            }

            // Remove MUIDescription
            if (ERROR_SUCCESS == RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                _T("SOFTWARE\\Microsoft\\MediaPlayer\\Player\\Extensions\\MUIDescriptions"), 0, KEY_WRITE, &Key))
            {
                RegDeleteValue(Key, KeyName);
                RegCloseKey(Key);
            }
        }
    }

    // WMP9
    wsprintf(RegistryKeyName,
        _T("SOFTWARE\\Microsoft\\Multimedia\\WMPlayer\\Extensions\\%s"), ExtensionWithoutStar);
    RegDeleteKey(HKEY_LOCAL_MACHINE, RegistryKeyName);
}
