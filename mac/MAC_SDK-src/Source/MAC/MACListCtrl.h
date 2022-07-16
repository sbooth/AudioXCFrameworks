#pragma once

#define COLUMN_FILENAME                 0
#define COLUMN_EXTENSION                1
#define COLUMN_ORIGINAL_SIZE            2
#define COLUMN_COMPRESSED_SIZE          3
#define COLUMN_COMPRESSION_PERCENTAGE   4
#define COLUMN_TIME_ELAPSED             5
#define COLUMN_STATUS                   6
#define COLUMN_COUNT                    7

#include "MACFileArray.h"

#pragma warning(push)
#pragma warning(disable: 4458)
#include <gdiplus.h>
#pragma warning(pop)

class CMACDlg;

class CMACListCtrl : public CListCtrl
{
public:

    CMACListCtrl();
    virtual ~CMACListCtrl();

    BOOL Initialize(CMACDlg * pParent, MAC_FILE_ARRAY * paryFiles);

    BOOL GetFileList(CStringArray & aryFiles, BOOL bIgnoreSelection = FALSE);
    
    BOOL StartFileInsertion(BOOL bClearList = TRUE);
    BOOL FinishFileInsertion();
    BOOL AddFileInternal(const CString & strFilename);
    BOOL AddFolderInternal(CString strPath);

    BOOL AddFile(const CString & strFilename);
    BOOL AddFolder(CString strPath);

    BOOL RemoveAllFiles();
    BOOL RemoveSelectedFiles();

    BOOL Update();
    void LoadColumns();

    BOOL SelectNone();
    BOOL SelectAll();
    
    CString GetStatus(MAC_FILE & File);
    BOOL SetMode(MAC_MODES Mode);
    BOOL LoadFileList(const CString & strPath);

protected:

    afx_msg void OnDestroy();
    afx_msg void OnGetdispinfo(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnDropFiles(HDROP hDropInfo);
    afx_msg void OnBegindrag(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnRclick(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg void OnLvnColumnclickList(NMHDR * pNMHDR, LRESULT * pResult);
    afx_msg BOOL OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * pResult);
    afx_msg BOOL OnEraseBkgnd(CDC * pDC);
    DECLARE_MESSAGE_MAP()

    // helper functions
    BOOL SaveFileList(const CString & strPath);
    CString GetFilename(int nIndex);

    // sorting
    int m_nSortColumn;
    BOOL m_bSortAscending;
    BOOL m_bSortEnabled;

    // the actual files
    MAC_FILE_ARRAY * m_paryFiles;

    // supported extensions
    CStringArrayEx m_arySupportedExtensions;

    // image
    CSmartPtr<Gdiplus::Bitmap> m_spBitmap;

    // other data
    CMACDlg * m_pParent;
};
