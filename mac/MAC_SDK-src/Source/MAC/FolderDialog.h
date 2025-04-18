// FolderDialog.h: interface for the CFolderDialog class.
//    $Copyright �  1998 Kenneth M. Reed, ALL RIGHTS RESERVED. $
//    $Header: FolderDialog.h  Revision:1.11  Mon Apr 06 12:04:50 1998  KenReed $

#ifndef _CFolderDialog_
#define _CFolderDialog_

#include <shlobj.h>

class CFolderDialog
{
public:
    static int CALLBACK BrowseDirectoryCallback(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData);

    CFolderDialog(    LPCTSTR lpszFolderName = APE_NULL,
                    DWORD dwFlags = 0/*BIF_RETURNONLYFSDIRS*/,
                    CWnd* pParentWnd = APE_NULL);
    virtual ~CFolderDialog();
    virtual int DoModal();
    CString GetPathName() const;

protected:
    virtual void OnInitDialog();
    virtual void OnSelChanged(ITEMIDLIST * pIdl);
    virtual void CallbackFunction(HWND hWnd, UINT uMsg, LPARAM lParam);

    void EnableOK(bool bEnable = true);
    void SetSelection(LPCTSTR pszSelection);
    void SetSelection(ITEMIDLIST * pIdl);
    void SetStatusText(LPCTSTR pszStatusText);
    CString ShortName(const CString & strName);

public:
    BROWSEINFO m_bi;

protected:
    CString m_strInitialFolderName;
    CString m_strFinalFolderName;

    TCHAR m_szDisplayName[APE_MAX_PATH];
    TCHAR m_szPath[APE_MAX_PATH];

    HWND m_hDialogBox;


};

#endif // _CFolderDialog_
