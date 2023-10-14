#include "stdafx.h"
#include "MAC.h"
#include "MACDlg.h"
#include "MACListCtrl.h"
#include "FormatArray.h"
#include <afxole.h>
#include "MemDC.h"

using namespace APE;

struct MAC_ERROR_EXPLANATION
{
    int nErrorCode;
    LPCTSTR pDescription;
};
const MAC_ERROR_EXPLANATION g_MACErrorExplanations[] = {
    { ERROR_IO_READ                               , _T("I/O read error") },                         \
    { ERROR_IO_WRITE                              , _T("I/O write error") },                        \
    { ERROR_INVALID_INPUT_FILE                    , _T("invalid input file") },                     \
    { ERROR_INVALID_OUTPUT_FILE                   , _T("invalid output file") },                    \
    { ERROR_INPUT_FILE_TOO_LARGE                  , _T("input file file too large") },              \
    { ERROR_INPUT_FILE_UNSUPPORTED_BIT_DEPTH      , _T("input file unsupported bit depth") },       \
    { ERROR_INPUT_FILE_UNSUPPORTED_SAMPLE_RATE    , _T("input file unsupported sample rate") },     \
    { ERROR_INPUT_FILE_UNSUPPORTED_CHANNEL_COUNT  , _T("input file unsupported channel count") },   \
    { ERROR_INPUT_FILE_TOO_SMALL                  , _T("input file too small") },                   \
    { ERROR_INVALID_CHECKSUM                      , _T("invalid checksum") },                       \
    { ERROR_DECOMPRESSING_FRAME                   , _T("decompressing frame") },                    \
    { ERROR_INITIALIZING_UNMAC                    , _T("initializing unmac") },                     \
    { ERROR_INVALID_FUNCTION_PARAMETER            , _T("invalid function parameter") },             \
    { ERROR_UNSUPPORTED_FILE_TYPE                 , _T("unsupported file type") },                  \
    { ERROR_INSUFFICIENT_MEMORY                   , _T("insufficient memory") },                    \
    { ERROR_LOADING_APE_DLL                       , _T("loading MAC.dll") },                        \
    { ERROR_LOADING_APE_INFO_DLL                  , _T("loading MACinfo.dll") },                    \
    { ERROR_LOADING_UNMAC_DLL                     , _T("loading UnMAC.dll") },                      \
    { ERROR_USER_STOPPED_PROCESSING               , _T("user stopped") },                           \
    { ERROR_SKIPPED                               , _T("skipped") },                                \
    { ERROR_BAD_PARAMETER                         , _T("bad parameter") },                          \
    { ERROR_APE_COMPRESS_TOO_MUCH_DATA            , _T("APE compress too much data") },             \
    { ERROR_UNSUPPORTED_FILE_VERSION              , _T("unsupported file version") },               \
    { ERROR_OPENING_FILE_IN_USE                   , _T("file in use") },                            \
    { ERROR_UNDEFINED                             , _T("undefined") }
};

#define IDC_AUTO_SIZE_ALL 1000

CMACListCtrl::CMACListCtrl()
{
    m_pParent = NULL;
    m_paryFiles = NULL;
}

CMACListCtrl::~CMACListCtrl()
{
}

BEGIN_MESSAGE_MAP(CMACListCtrl, CListCtrl)
    ON_WM_DESTROY()
    ON_NOTIFY_REFLECT(LVN_GETDISPINFO, &CMACListCtrl::OnGetdispinfo)
    ON_WM_DROPFILES()
    ON_NOTIFY_REFLECT(LVN_BEGINDRAG, &CMACListCtrl::OnBegindrag)
    ON_NOTIFY_REFLECT(NM_RCLICK, &CMACListCtrl::OnRclick)
    ON_NOTIFY_REFLECT(LVN_COLUMNCLICK, &CMACListCtrl::OnLvnColumnclickList)
    ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

BOOL CMACListCtrl::Initialize(CMACDlg * pParent, MAC_FILE_ARRAY * paryFiles)
{
    // store pointers
    m_pParent = pParent;
    m_paryFiles = paryFiles;

    // set the style
    SetExtendedStyle(GetExtendedStyle() | LVS_EX_HEADERDRAGDROP | LVS_EX_FULLROWSELECT);

    // accept drag-n-drop
    DragAcceptFiles(TRUE);

    // build the columns
    InsertColumn(COLUMN_FILENAME, _T("File Name"));
    InsertColumn(COLUMN_EXTENSION, _T("Extension"), LVCFMT_CENTER);
    InsertColumn(COLUMN_ORIGINAL_SIZE, _T("Original (MB)"), LVCFMT_CENTER);
    InsertColumn(COLUMN_COMPRESSED_SIZE, _T("Compressed (MB)"), LVCFMT_CENTER);
    InsertColumn(COLUMN_COMPRESSION_PERCENTAGE, _T("%"), LVCFMT_CENTER);
    InsertColumn(COLUMN_TIME_ELAPSED, _T("Time (s)"), LVCFMT_CENTER);
    InsertColumn(COLUMN_STATUS, _T("Status"), LVCFMT_CENTER);

    // set background color
    SetTextBkColor(CLR_NONE);
    SetTextColor(RGB(0, 0, 0));

    // get system UI font
    NONCLIENTMETRICS ncm;
    APE_CLEAR(ncm);
    ncm.cbSize = sizeof(ncm);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, ncm.cbSize, &ncm, 0);

    // create font
    m_Font.CreateFont(ncm.lfMessageFont.lfHeight, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, ncm.lfMessageFont.lfFaceName);
    SetFont(&m_Font);

    return TRUE;
}

BOOL CMACListCtrl::LoadFileList(const CString & strPath, CStringArrayEx * paryFiles)
{
    if ((paryFiles != NULL) && (paryFiles->GetSize() > 0))
    {
        for (int z = 0; z < paryFiles->GetSize(); z++)
        {
            CString strFilename = paryFiles->ElementAt(z);
            strFilename.Trim(_T("\""));
            if (z == 0)
            {
                // set the mode based on the first file
                if (strFilename.Right(3).CompareNoCase(_T("ape")) == 0)
                    m_pParent->SetMode(MODE_DECOMPRESS);
                else if (strFilename.Right(3).CompareNoCase(_T("wav")) == 0)
                    m_pParent->SetMode(MODE_COMPRESS);

                // start insertion after we set the mode
                StartFileInsertion();
            }
            AddFileInternal(strFilename);
        }
        FinishFileInsertion();
    }
    else
    {
        HANDLE hInputFile = CreateFile(strPath, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hInputFile != INVALID_HANDLE_VALUE)
        {
            DWORD dwTotalBytes = GetFileSize(hInputFile, NULL);
            unsigned long nBytesRead = 0;

            CSmartPtr<char> spBuffer(new char [static_cast<size_t>(dwTotalBytes) + 1], TRUE);
            spBuffer[dwTotalBytes] = 0;
            if (ReadFile(hInputFile, spBuffer, dwTotalBytes, &nBytesRead, NULL) == false)
                spBuffer[0] = 0;

            DWORD dwIndex = 0;
            for (dwIndex = 0; dwIndex < dwTotalBytes; dwIndex++)
            {
                if ((spBuffer[dwIndex] == '\r') || (spBuffer[dwIndex] == '\n'))
                    spBuffer[dwIndex] = 0;
            }

            StartFileInsertion();

            dwIndex = 0;
            while (dwIndex < dwTotalBytes)
            {
                if (spBuffer[dwIndex] == 0)
                {
                    dwIndex++;
                }
                else
                {
                    CSmartPtr<TCHAR> spUTF16(CAPECharacterHelper::GetUTF16FromUTF8(reinterpret_cast<unsigned char *>(&spBuffer[dwIndex])), TRUE);
                    AddFileInternal(spUTF16.GetPtr());
                    dwIndex += static_cast<DWORD>(strlen(&spBuffer[dwIndex]) + 1);
                }
            }

            FinishFileInsertion();

            CloseHandle(hInputFile);
        }
    }

    return TRUE;
}

BOOL CMACListCtrl::SaveFileList(const CString & strPath)
{
    HANDLE hOutputFile = CreateFile(strPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

    if (hOutputFile != INVALID_HANDLE_VALUE)
    {
        unsigned long nBytesWritten = 0;
        for (int z = 0; z < GetItemCount(); z++)
        {
            CSmartPtr<char> spUTF8(reinterpret_cast<char *>(CAPECharacterHelper::GetUTF8FromUTF16(GetFilename(z))), TRUE);

            WriteFile(hOutputFile, spUTF8.GetPtr(), static_cast<DWORD>(strlen(spUTF8)), &nBytesWritten, NULL);
            WriteFile(hOutputFile, "\r\n", 2, &nBytesWritten, NULL);
        }

        CloseHandle(hOutputFile);
    }

    return TRUE;
}

BOOL CMACListCtrl::StartFileInsertion(BOOL bClearList)
{
    if (bClearList)
    {
        m_paryFiles->RemoveAll();
    }

    m_arySupportedExtensions.RemoveAll();
    theApp.GetFormatArray()->GetInputExtensions(m_arySupportedExtensions);

    return TRUE;
}

BOOL CMACListCtrl::FinishFileInsertion()
{
    Update();

    return TRUE;
}

BOOL CMACListCtrl::Update()
{
    SetItemCount(static_cast<int>(m_paryFiles->GetSize()));
    if (m_pParent->m_ctrlStatusBar.m_hWnd != NULL)
        m_pParent->m_ctrlStatusBar.UpdateFiles(m_paryFiles);

    return TRUE;
}

void CMACListCtrl::SaveColumns()
{
    for (int z = 0; z < COLUMN_COUNT; z++)
    {
        CString strValueName; strValueName.Format(_T("List Column %d Width"), z);
        int nWidth = GetColumnWidth(z);
        nWidth = theApp.GetSizeReverse(nWidth);
        theApp.GetSettings()->SaveSetting(strValueName, nWidth);
    }
}

void CMACListCtrl::LoadColumns()
{
    for (int z = 0; z < COLUMN_COUNT; z++)
    {
        CString strValueName; strValueName.Format(_T("List Column %d Width"), z);
        int nSize = theApp.GetSettings()->LoadSetting(strValueName, 100);
        if (nSize <= 20)
            nSize = 200;
        nSize = theApp.GetSize(nSize, 0).cx;
        SetColumnWidth(z, nSize);
    }

    CSmartPtr<int> spOrderArray(new int[COLUMN_COUNT], TRUE);
    if (theApp.GetSettings()->LoadSetting(_T("List Column Order"), spOrderArray, (sizeof(int) * COLUMN_COUNT)))
        SetColumnOrderArray(COLUMN_COUNT, spOrderArray);
}

BOOL CMACListCtrl::AddFile(const CString & strFilename)
{
    StartFileInsertion();
    AddFileInternal(strFilename);
    FinishFileInsertion();

    return TRUE;
}

BOOL CMACListCtrl::AddFolder(CString strPath)
{
    theApp.GetSettings()->m_aryAddFolderMRU.Remove(strPath, FALSE);
    while (theApp.GetSettings()->m_aryAddFolderMRU.GetSize() > 4)
        theApp.GetSettings()->m_aryAddFolderMRU.RemoveAt(4);
    theApp.GetSettings()->m_aryAddFolderMRU.InsertAt(0, strPath);

    StartFileInsertion(false);
    AddFolderInternal(strPath);
    FinishFileInsertion();

    return TRUE;
}

BOOL CMACListCtrl::AddFileInternal(CString strInputFilename)
{
    // make sure the file exists
    WIN32_FIND_DATA WFD;
    HANDLE hFind = FindFirstFile(strInputFilename, &WFD);
    if (hFind == INVALID_HANDLE_VALUE) { return ERROR_UNDEFINED; }

    BOOL bRetVal = FALSE;
    if (WFD.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    {
        // if it's a directory, use AddFolder(...)
        bRetVal = AddFolderInternal(strInputFilename);
    }
    else
    {
        // expand the filename
        TCHAR * pLongFilename = new TCHAR [APE_MAX_PATH];
        pLongFilename[0] = 0;
        if (GetLongPathName(strInputFilename, pLongFilename, APE_MAX_PATH) > 0)
            strInputFilename = pLongFilename;
        APE_SAFE_ARRAY_DELETE(pLongFilename)
        if ((strInputFilename.GetLength() >= MAX_PATH) &&
            (strInputFilename.Left(4) != _T("\\\\?\\")))
        {
            strInputFilename = _T("\\\\?\\") + strInputFilename;
        }

        // split the path
        CString strExtension = GetExtension(strInputFilename);

        // add the item
        if (m_arySupportedExtensions.Find(strExtension) != -1)
        {
            // build file
            MAC_FILE File;
            File.strInputFilename = strInputFilename;
            File.dInputFileBytes = static_cast<double>(WFD.nFileSizeLow) + (static_cast<double>(WFD.nFileSizeHigh) * static_cast<double>(4294967296.0));

            // add
            m_paryFiles->Add(File);

            // success
            bRetVal = TRUE;
        }
    }

    // cleanup
    FindClose(hFind);

    return bRetVal;
}

BOOL CMACListCtrl::AddFolderInternal(CString strPath)
{
    CStringArray aryFiles; ListFiles(&aryFiles, strPath, TRUE);
    for (int z = 0; z < aryFiles.GetSize(); z++)
        AddFileInternal(aryFiles.GetAt(z));

    return TRUE;
}

BOOL CMACListCtrl::RemoveSelectedFiles()
{
    SetRedraw(FALSE);

    // build a list of indexes
    POSITION Pos = GetFirstSelectedItemPosition();
    CUIntArray aryIndex;
    while (Pos)
        aryIndex.Add(static_cast<UINT>(GetNextSelectedItem(Pos)));

    // remove the files
    for (intn z = aryIndex.GetUpperBound(); z >= 0; z--)
        m_paryFiles->RemoveAt(static_cast<INT_PTR>(aryIndex[z]));

    // update
    SetItemCount(static_cast<int>(m_paryFiles->GetSize()));
    SelectNone();
    m_pParent->m_ctrlStatusBar.UpdateFiles(m_paryFiles);

    SetRedraw(TRUE);

    return TRUE;
}

BOOL CMACListCtrl::RemoveAllFiles()
{
    DeleteAllItems();
    m_paryFiles->RemoveAll();
    m_pParent->m_ctrlStatusBar.UpdateFiles(m_paryFiles);

    return TRUE;
}

void CMACListCtrl::OnDestroy()
{
    // save the settings
    SaveColumns();

    CSmartPtr<int> spOrderArray(new int [COLUMN_COUNT], TRUE);
    GetColumnOrderArray(spOrderArray, -1);
    theApp.GetSettings()->SaveSetting(_T("List Column Order"), spOrderArray, (sizeof(int) * COLUMN_COUNT));

    // save the file list
    CString strFileListsFolder = GetUserDataPath() + _T("File Lists\\");
    CreateDirectoryEx(strFileListsFolder);
    SaveFileList(strFileListsFolder + _T("Current.m3u"));

    // base class
    CListCtrl::OnDestroy();
}

CString CMACListCtrl::GetFilename(int nIndex)
{
    CString strRetVal;
    if (m_paryFiles && (nIndex >= 0) && (nIndex < m_paryFiles->GetSize()))
        strRetVal = m_paryFiles->ElementAt(nIndex).strInputFilename;

    return strRetVal;
}

CString CMACListCtrl::GetStatus(const MAC_FILE & File)
{
    CString strValue;
    if (File.bDone)
    {
        // lookup the error text
        if (File.nRetVal == ERROR_SUCCESS)
        {
            strValue = _T("OK");
        }
        else if (File.nRetVal == ERROR_SKIPPED)
        {
            strValue = _T("Skipped");
        }
        else
        {
            CString strError = _T("unknown");
            for (int z = 0; z < static_cast<int>(_countof(g_MACErrorExplanations)); z++)
            {
                if (File.nRetVal == g_MACErrorExplanations[z].nErrorCode)
                {
                    strError = g_MACErrorExplanations[z].pDescription;
                    break;
                }
            }
            strValue.Format(_T("Error (%s)"), strError.GetString());
        }
    }
    else if (File.bProcessing)
    {
        double dProgress = File.GetProgress();
        strValue.Format(_T("%.1f%% (%s)"), dProgress * 100, g_aryModeActionNames[File.Mode]);
    }
    else
    {
        strValue = _T("Queued");
    }

    return strValue;
}

BOOL CMACListCtrl::SetMode(APE_MODES Mode)
{
    LVCOLUMN lvCol;
    APE_CLEAR(lvCol);
    lvCol.mask = LVCF_TEXT;
    GetColumn(3, &lvCol);

    if (Mode == MODE_DECOMPRESS)
        lvCol.pszText = _T("Decompressed (MB)");
    else if (Mode == MODE_CONVERT)
        lvCol.pszText = _T("Processed (MB)");
    else
        lvCol.pszText = _T("Compressed (MB)");
    SetColumn(3, &lvCol);

    return TRUE;
}

BOOL CMACListCtrl::GetFileList(CStringArray & aryFiles, BOOL bIgnoreSelection)
{
    aryFiles.RemoveAll();

    POSITION Pos = GetFirstSelectedItemPosition();

    if (bIgnoreSelection || (Pos == NULL))
    {
        for (int z = 0; z < GetItemCount(); z++)
            aryFiles.Add(GetFilename(z));
    }
    else
    {
        while (Pos)
        {
            int nIndex = GetNextSelectedItem(Pos);
            aryFiles.Add(GetFilename(nIndex));
        }
    }

    return TRUE;
}

void CMACListCtrl::OnGetdispinfo(NMHDR* pNMHDR, LRESULT* pResult)
{
    LV_DISPINFO * pDispInfo = reinterpret_cast<LV_DISPINFO *>(pNMHDR);

    if (pDispInfo->item.mask & LVIF_TEXT)
    {
        const int nRow = pDispInfo->item.iItem;
        const int nColumn = pDispInfo->item.iSubItem;
        if ((nRow >= 0) && (nRow < m_paryFiles->GetSize()))
        {
            MAC_FILE & File = m_paryFiles->ElementAt(nRow);
            CString strValue;
            switch (nColumn)
            {
                case COLUMN_FILENAME:
                {
                    strValue = File.strInputFilename;
                    break;
                }
                case COLUMN_EXTENSION:
                {
                    strValue = GetExtension(File.strInputFilename);
                    break;
                }
                case COLUMN_ORIGINAL_SIZE:
                {
                    strValue.Format(_T("%.2f"), static_cast<double>(File.dInputFileBytes) / (static_cast<double>(1024) * static_cast<double>(1024)));
                    break;
                }
                case COLUMN_COMPRESSED_SIZE:
                {
                    if (File.dOutputFileBytes > 0)
                        strValue.Format(_T("%.2f"), static_cast<double>(File.dOutputFileBytes) / (static_cast<double>(1024) * static_cast<double>(1024)));

                    break;
                }
                case COLUMN_COMPRESSION_PERCENTAGE:
                {
                    if (File.dOutputFileBytes > 0)
                        strValue.Format(_T("%.2f%%"), (static_cast<double>(100.0) * static_cast<double>(File.dOutputFileBytes)) / static_cast<double>(File.dInputFileBytes));
                    break;
                }
                case COLUMN_TIME_ELAPSED:
                {
                    if (File.bStarted && File.bDone)
                    {
                        int64 nElapsedMS = static_cast<int64>(File.dwEndTickCount - File.dwStartTickCount);
                        strValue.Format(_T("%.2f"), static_cast<double>(nElapsedMS) / static_cast<double>(1000));
                    }
                    break;
                }
                case COLUMN_STATUS:
                {
                    strValue = GetStatus(File);
                    break;
                }
            }

            _tcsncpy_s(pDispInfo->item.pszText, pDispInfo->item.cchTextMax, strValue, ape_max(0, int(pDispInfo->item.cchTextMax - 1)));
            pDispInfo->item.pszText[pDispInfo->item.cchTextMax - 1] = 0;
        }
    }

    *pResult = 0;
}

void CMACListCtrl::OnDropFiles(HDROP hDropInfo)
{
    StartFileInsertion(FALSE);

    const int nFiles = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
    TCHAR * pFilename = new TCHAR [APE_MAX_PATH];
    for (int z = 0; z < nFiles; z++)
    {
        pFilename[0] = 0;
        DragQueryFile(hDropInfo, z, pFilename, APE_MAX_PATH);
        AddFileInternal(pFilename);
    }
    APE_SAFE_ARRAY_DELETE(pFilename)

    FinishFileInsertion();

    CListCtrl::OnDropFiles(hDropInfo);
}

void CMACListCtrl::OnBegindrag(NMHDR *, LRESULT *)
{
    // get the list of files
    CStringList slDraggedFiles; int nBufferSize = 0;
    POSITION Pos = GetFirstSelectedItemPosition();
    if (Pos != NULL)
    {
        while (Pos != NULL)
        {
            CString strFilename = GetFilename(GetNextSelectedItem(Pos));
            slDraggedFiles.AddTail(strFilename);
            nBufferSize += strFilename.GetLength() + 1;
        }
        nBufferSize = static_cast<int>(sizeof(DROPFILES)) + (static_cast<int>(sizeof(TCHAR)) * (nBufferSize + 1));

        // create the drop object
        HGLOBAL hgDrop = GlobalAlloc(GHND | GMEM_SHARE, static_cast<size_t>(nBufferSize));
        if (hgDrop != NULL)
        {
            DROPFILES * pDrop = static_cast<DROPFILES *>(GlobalLock(hgDrop));
            if (pDrop != NULL)
            {
                // setup the drop object
                pDrop->pFiles = sizeof(DROPFILES);
                pDrop->pt.x = 0;
                pDrop->pt.y = 0;
                pDrop->fNC = 0;
                #ifdef _UNICODE
                    pDrop->fWide = TRUE;
                #endif
                int nBufferSizeLeft = nBufferSize - int(sizeof(DROPFILES));

                // fill in the actual filenames
                POSITION InternalPos = slDraggedFiles.GetHeadPosition();
                TCHAR * pszBuff = reinterpret_cast<TCHAR *>((LPBYTE(pDrop) + sizeof(DROPFILES)));
                while (InternalPos != NULL)
                {
                    LPCTSTR pFilename = slDraggedFiles.GetNext(InternalPos).GetString();
                    _tcscpy_s(pszBuff, static_cast<size_t>(nBufferSizeLeft) / sizeof(TCHAR), pFilename);
                    pszBuff += 1 + _tcslen(pFilename);
                    nBufferSizeLeft -= static_cast<int>((sizeof(TCHAR) * (1 + _tcslen(pFilename))));
                }
                GlobalUnlock(hgDrop);

                // create the data source
                COleDataSource * pDataSource = new COleDataSource;
                FORMATETC FormatEtc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
                pDataSource->CacheGlobalData(CF_HDROP, hgDrop, &FormatEtc);

                // do the drag-n-drop
                pDataSource->DoDragDrop();

                // release
                GlobalUnlock(hgDrop);
                pDataSource->InternalRelease();
            }

            GlobalFree(hgDrop);
        }
    }
}

void CMACListCtrl::OnRclick(NMHDR *, LRESULT * pResult)
{
    CMenu menuPopup; menuPopup.CreatePopupMenu();

    menuPopup.AppendMenu(MF_STRING, 1000, _T("Add File(s)"));
    menuPopup.AppendMenu(MF_STRING, 1001, _T("Add Folder"));
    menuPopup.AppendMenu(MF_STRING, 1002, _T("File Info..."));

    CPoint ptMouse; GetCursorPos(&ptMouse);
    int nRetVal = menuPopup.TrackPopupMenu(TPM_LEFTBUTTON | TPM_RIGHTBUTTON | TPM_RETURNCMD,
        ptMouse.x, ptMouse.y, this);

    if (nRetVal == 1000)
    {
        m_pParent->PostMessage(WM_COMMAND, ID_FILE_ADD_FILES, 0);
    }
    else if (nRetVal == 1001)
    {
        m_pParent->PostMessage(WM_COMMAND, ID_FILE_ADD_FOLDER, 0);
    }
    else if (nRetVal == 1002)
    {
        m_pParent->PostMessage(WM_COMMAND, ID_FILE_FILE_INFO, 0);
    }

    *pResult = 0;
}


BOOL CMACListCtrl::SelectNone()
{
    for (int z = 0; z < GetItemCount(); z++)
        SetItemState(z, 0, LVIS_SELECTED | LVIS_FOCUSED);

    return TRUE;
}

BOOL CMACListCtrl::SelectAll()
{
    for (int z = 0; z < GetItemCount(); z++)
        SetItemState(z, LVIS_SELECTED, LVIS_SELECTED | LVIS_FOCUSED);

    return TRUE;
}

static CMACListCtrl * s_pThis = NULL;
static int s_nCompareColumn = -1;
static int s_nCompareOrder = 0;

int Compare(const void * pOne, const void * pTwo); // declare before implementing because it makes Clang happy
int Compare(const void * pOne, const void * pTwo)
{
    int nReturn = 0;
    const MAC_FILE * pFileOne = static_cast<const MAC_FILE *>(pOne);
    const MAC_FILE * pFileTwo = static_cast<const MAC_FILE *>(pTwo);
    if (s_nCompareColumn == COLUMN_FILENAME)
    {
        nReturn = pFileOne->strInputFilename.Compare(pFileTwo->strInputFilename);
    }
    else if (s_nCompareColumn == COLUMN_EXTENSION)
    {
        CString strExtensionOne = GetExtension(pFileOne->strInputFilename);
        CString strExtensionTwo = GetExtension(pFileTwo->strInputFilename);

        nReturn = strExtensionOne.Compare(strExtensionTwo);
    }
    else if (s_nCompareColumn == COLUMN_ORIGINAL_SIZE)
    {
        nReturn = static_cast<int>(pFileOne->dInputFileBytes - pFileTwo->dInputFileBytes);
    }
    else if (s_nCompareColumn == COLUMN_COMPRESSED_SIZE)
    {
        nReturn = static_cast<int>(pFileOne->dOutputFileBytes - pFileTwo->dOutputFileBytes);
    }
    else if (s_nCompareColumn == COLUMN_COMPRESSION_PERCENTAGE)
    {
        double dOne = static_cast<double>(pFileOne->dOutputFileBytes) / static_cast<double>(pFileOne->dInputFileBytes);
        double dTwo = static_cast<double>(pFileTwo->dOutputFileBytes) / static_cast<double>(pFileTwo->dInputFileBytes);
        if (dOne > dTwo)
            nReturn = 1;
        else if (dOne < dTwo)
            nReturn = -1;
    }
    else if (s_nCompareColumn == COLUMN_TIME_ELAPSED)
    {
        nReturn = static_cast<int>((pFileOne->dwEndTickCount - pFileOne->dwStartTickCount) - (pFileTwo->dwEndTickCount - pFileTwo->dwStartTickCount));
    }
    else if (s_nCompareColumn == COLUMN_STATUS)
    {
        nReturn = s_pThis->GetStatus(*pFileOne).Compare(s_pThis->GetStatus(*pFileTwo));
    }

    if (s_nCompareOrder == 1)
    {
        if (nReturn > 0)
            nReturn = -1;
        else if (nReturn < 0)
            nReturn = 1;
    }

    return nReturn;
}

void CMACListCtrl::OnLvnColumnclickList(NMHDR *pNMHDR, LRESULT *pResult)
{
    if (m_pParent->GetProcessing() != false)
    {
        AfxMessageBox(_T("Sorting is not possible while processing."));
        return;
    }

    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    s_pThis = this;
    if (pNMLV->iSubItem == s_nCompareColumn)
        s_nCompareOrder = (s_nCompareOrder == 0) ? 1 : 0;
    else
        s_nCompareOrder = 0;
    s_nCompareColumn = pNMLV->iSubItem;

    if (m_paryFiles->GetSize() > 0)
    {
        if (s_nCompareColumn == COLUMN_STATUS)
        {
            bool bAllOK = true;
            for (int z = 0; z < m_paryFiles->GetSize(); z++)
            {
                CString strStatus = GetStatus(m_paryFiles->GetAt(z));
                if (strStatus != _T("OK"))
                {
                    bAllOK = false;
                    break;
                }
            }

            if (bAllOK && (m_paryFiles->GetSize() > 0))
            {
                AfxMessageBox(_T("All files are OK."));
                return;
            }
        }

        qsort(&m_paryFiles->ElementAt(0), static_cast<size_t>(m_paryFiles->GetSize()), sizeof(MAC_FILE), Compare);
        DeleteAllItems();
        Update();
    }

    *pResult = 0;
}

BOOL CMACListCtrl::OnNotify(WPARAM wParam, LPARAM lParam, LRESULT * pResult)
{
    if (wParam == 0 && (reinterpret_cast<NMHDR *>(lParam))->code == NM_RCLICK)
    {
        // get mouse point
        POINT MousePoint;
        GetCursorPos(&MousePoint);

        POINT Point = MousePoint;
        ScreenToClient(&Point);

        // create hit test object
        HDHITTESTINFO HitTest;
        APE_CLEAR(HitTest);

        // offset of right scrolling
        HitTest.pt.x = Point.x + GetScrollPos(SB_HORZ); // offset of right scrolling
        HitTest.pt.y = Point.y;

        // send the hit test message
        GetHeaderCtrl()->SendMessage(HDM_HITTEST, 0, reinterpret_cast<LPARAM>(&HitTest));

        // create a menu
        CMenu menu;
        menu.CreatePopupMenu();
        menu.AppendMenuW(MF_STRING, IDC_AUTO_SIZE_ALL, L"&Auto-size All");

        // track the menu
        int nResult = menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_RETURNCMD, MousePoint.x, MousePoint.y, this);

        // process result
        if (nResult == IDC_AUTO_SIZE_ALL)
        {
            RECT rectList;
            GetClientRect(&rectList);
            int nListWidth = rectList.right - rectList.left;

            int nTotalWidth = 0;
            for (int i = 0; i < GetHeaderCtrl()->GetItemCount(); i++)
            {
                // auto-size
                SetColumnWidth(i, LVSCW_AUTOSIZE);

                // set a minimum width
                if (GetColumnWidth(i) < 50)
                    SetColumnWidth(i, 50);

                nTotalWidth += GetColumnWidth(i);
            }

            if ((nTotalWidth + 32) < nListWidth)
            {
                int nColumn0 = static_cast<int>(static_cast<double>(nListWidth) * 0.30);
                SetColumnWidth(0, nColumn0);
                int nColumn1 = static_cast<int>(static_cast<double>(nListWidth) * 0.10);
                SetColumnWidth(1, nColumn1);
                int nColumn2 = static_cast<int>(static_cast<double>(nListWidth) * 0.10);
                SetColumnWidth(2, nColumn2);
                int nColumn3 = static_cast<int>(static_cast<double>(nListWidth) * 0.14);
                SetColumnWidth(3, nColumn3);
                int nColumn4 = static_cast<int>(static_cast<double>(nListWidth) * 0.10);
                SetColumnWidth(4, nColumn4);
                int nColumn5 = static_cast<int>(static_cast<double>(nListWidth) * 0.10);
                SetColumnWidth(5, nColumn5);
                int nColumn6 = static_cast<int>(static_cast<double>(nListWidth) * 0.10);
                SetColumnWidth(6, nColumn6);
            }
        }
    }

    return CListCtrl::OnNotify(wParam, lParam, pResult);
}

BOOL CMACListCtrl::OnEraseBkgnd(CDC * pDC)
{
    if ((m_pParent == NULL) || (m_pParent->GetInitialized() == FALSE))
        return TRUE;

    // get rectangle
    CRect rectClient;
    GetClientRect(&rectClient);

    // create a memory buffer
    CMemoryDC Buffer(pDC, &rectClient);

    // flush
    Buffer.FillSolidRect(rectClient, RGB(255, 255, 255));

    // handle scrolling
    int nScroll = GetScrollPos(SB_VERT);
    if (nScroll > 0)
    {
        CRect rectItem;
        GetItemRect(0, &rectItem, 0);
        nScroll *= rectItem.Height();
        rectClient.OffsetRect(0, -nScroll);
    }

    // layout image
    CSize sizeImage = theApp.GetSize(150, 150);
    CPoint ptTopLeft(rectClient.CenterPoint().x - (sizeImage.cx / 2), rectClient.CenterPoint().y - (sizeImage.cy / 2));
    CRect rectLayout(ptTopLeft, sizeImage);

    // drag image
    Gdiplus::Graphics graphics(Buffer.GetSafeHdc());
    graphics.SetInterpolationMode(Gdiplus::InterpolationMode::InterpolationModeHighQualityBicubic);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.DrawImage(theApp.GetMonkeyImage(), rectLayout.left, rectLayout.top, rectLayout.Width(), rectLayout.Height());

    // painting from the memory buffer to the screen happens when the memory DC unwinds

    return TRUE;
}
