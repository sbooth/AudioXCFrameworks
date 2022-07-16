#include "stdafx.h"
#include "resource.h"
#include "MAC.h"
#include "MACDlg.h"
#include "AboutDlg.h"

#include "FolderDialog.h"
#include "OptionsDlg.h"
#include "FormatArray.h"
#include "APEInfoDlg.h"
#include "APEButtons.h"

#define IDT_UPDATE_PROGRESS        1
#define IDT_UPDATE_PROGRESS_MS    100

#define WM_DPICHANGED                   0x02E0

#define TBI(ID) (m_ctrlToolbar.GetToolBarCtrl().CommandToIndex(ID))

CMACDlg::CMACDlg(CWnd * pParent) : 
    CDialog(CMACDlg::IDD, pParent),
    m_ctrlStatusBar(this)
{
    m_bLastLoadMenuAndToolbarProcessing = FALSE;
    m_bInitialized = FALSE;
    m_hAcceleratorTable = NULL;
    
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMACDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST, m_ctrlList);
}

BEGIN_MESSAGE_MAP(CMACDlg, CDialog)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_COMMAND(ID_FILE_PROCESS_FILES, OnFileProcessFiles)
    ON_COMMAND(ID_FILE_ADD_FILES, OnFileAddFiles)
    ON_COMMAND(ID_FILE_ADD_FOLDER, OnFileAddFolder)
    ON_COMMAND(ID_FILE_CLEAR_FILES, OnFileClearFiles)
    ON_COMMAND(ID_FILE_SELECTALL, OnFileSelectAll)
    ON_COMMAND(ID_FILE_REMOVE_FILES, OnFileRemoveFiles)
    ON_COMMAND(ID_FILE_FILE_INFO, OnFileFileInfo)
    ON_COMMAND(ID_FILE_EXIT, OnFileExit)
    ON_WM_SIZE()
    ON_WM_TIMER()
    ON_NOTIFY(TBN_DROPDOWN, AFX_IDW_TOOLBAR, OnToolbarDropDown)
    ON_COMMAND(ID_MODE_COMPRESS, OnModeCompress)
    ON_COMMAND(ID_MODE_DECOMPRESS, OnModeDecompress)
    ON_COMMAND(ID_MODE_VERIFY, OnModeVerify)
    ON_COMMAND(ID_MODE_CONVERT, OnModeConvert)
    ON_COMMAND(ID_MODE_MAKE_APL, OnModeMakeAPL)
    ON_COMMAND(ID_STOP, OnStop)
    ON_COMMAND(ID_COMPRESSION, OnCompression)
    ON_COMMAND(ID_COMPRESSION_APE_EXTRA_HIGH, OnCompressionAPEExtraHigh)
    ON_COMMAND(ID_COMPRESSION_APE_FAST, OnCompressionAPEFast)
    ON_COMMAND(ID_COMPRESSION_APE_HIGH, OnCompressionAPEHigh)
    ON_COMMAND(ID_COMPRESSION_APE_NORMAL, OnCompressionAPENormal)
    ON_COMMAND(ID_COMPRESSION_APE_INSANE, OnCompressionAPEInsane)
    ON_COMMAND_RANGE(ID_SET_COMPRESSION_FIRST, ID_SET_COMPRESSION_LAST, OnSetCompressionMenu)
    ON_WM_DESTROY()
    ON_COMMAND(ID_PAUSE, OnPause)
    ON_COMMAND(ID_STOP_AFTER_CURRENT_FILE, OnStopAfterCurrentFile)
    ON_COMMAND(ID_STOP_IMMEDIATELY, OnStopImmediately)
    ON_COMMAND(ID_HELP_HELP, OnHelpHelp)
    ON_COMMAND(ID_HELP_ABOUT, OnHelpAbout)
    ON_COMMAND(ID_HELP_WEBSITE_MONKEYS_AUDIO, OnHelpWebsiteMonkeysAudio)
    ON_COMMAND(ID_HELP_WEBSITE_MEDIA_JUKEBOX, OnHelpWebsiteJRiver)
    ON_COMMAND(ID_HELP_WEBSITE_WINAMP, OnHelpWebsiteWinamp)
    ON_COMMAND(ID_HELP_WEBSITE_EAC, OnHelpWebsiteEac)
    ON_COMMAND(ID_TOOLS_OPTIONS, OnToolsOptions)
    ON_WM_INITMENU()
    ON_WM_INITMENUPOPUP()
    ON_WM_GETMINMAXINFO()
    ON_WM_ENDSESSION()
    ON_WM_QUERYENDSESSION()
    ON_MESSAGE(WM_DPICHANGED, OnDPIChange)
END_MESSAGE_MAP()

BOOL CMACDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    // layout the window (before getting the scale so the window is on the right monitor but hide the window)
    WINDOWPLACEMENT WindowPlacement; memset(&WindowPlacement, 0, sizeof(WindowPlacement));
    if (theApp.GetSettings()->LoadSetting(_T("Main Window Placement"), &WindowPlacement, sizeof(WindowPlacement)))
    {
        WindowPlacement.showCmd = SW_HIDE;
        SetWindowPlacement(&WindowPlacement);
    }
    else
    {
        SetWindowPos(NULL, 0, 0, theApp.GetSize(780, 0).cx, theApp.GetSize(0, 420).cy, SWP_NOZORDER | SWP_HIDEWINDOW);
        CenterWindow();
    }

    // accelerator
    m_hAcceleratorTable = LoadAccelerators(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME));

    // icons
    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    // file list
    m_ctrlList.Initialize(this, &m_aryFiles);
    INITIALIZE_COMMON_CONTROL(m_ctrlList.GetSafeHwnd());

    // load scale
    LoadScale();

    // load the formats
    theApp.GetFormatArray()->Load();

    // window properties
    SetWindowText(MAC_NAME);

    // set the mode to the list
    m_ctrlList.SetMode(theApp.GetSettings()->GetMode());

    // load file list
    m_ctrlList.LoadFileList(GetUserDataPath() + _T("File Lists\\Current.m3u"));

    // layout the window (again)
    memset(&WindowPlacement, 0, sizeof(WindowPlacement));
    if (theApp.GetSettings()->LoadSetting(_T("Main Window Placement"), &WindowPlacement, sizeof(WindowPlacement)))
    {
        SetWindowPlacement(&WindowPlacement);
    }
    else
    {
        SetWindowPos(NULL, 0, 0, theApp.GetSize(780, 0).cx, theApp.GetSize(0, 420).cy, SWP_NOZORDER);
        CenterWindow();
    }

    m_bInitialized = TRUE;
    LayoutWindow();

    return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CMACDlg::LoadMenuAndToolbar(BOOL bProcessing)
{
    if (m_bInitialized && (bProcessing == m_bLastLoadMenuAndToolbarProcessing))
        return TRUE;
    m_bLastLoadMenuAndToolbarProcessing = bProcessing;
    // BTNS_WHOLEDROPDOWN -- requires IE 5.0 //

    m_menuMain.DestroyMenu();
    m_menuMain.LoadMenu(bProcessing ? IDR_STOP_MENU : IDR_MAIN_MENU);
    SetMenu(&m_menuMain);

    while (m_ctrlToolbar.GetToolBarCtrl().DeleteButton(0)) { }

    if (bProcessing)
    {
        AddToolbarButton(ID_STOP, TBB_STOP, _T("Stop"), TBSTYLE_DROPDOWN);
        AddToolbarButton(ID_PAUSE, TBB_PAUSE, _T("Pause"), TBSTYLE_CHECK);
    }
    else
    {
        AddToolbarButton(ID_FILE_PROCESS_FILES, TBB_COMPRESS, _T("Process"), TBSTYLE_DROPDOWN);
        AddToolbarButton(ID_COMPRESSION, TBB_COMPRESSION_EXTERNAL, _T("Mode"), TBSTYLE_DROPDOWN);
        AddToolbarButton(ID_SEPARATOR, TBB_NONE, _T(""), TBSTYLE_SEP);
        AddToolbarButton(ID_FILE_ADD_FILES, TBB_ADD_FILES, _T("Add Files"), TBSTYLE_DROPDOWN);
        AddToolbarButton(ID_FILE_ADD_FOLDER, TBB_ADD_FOLDER, _T("Add Folder"), TBSTYLE_DROPDOWN);
        AddToolbarButton(ID_SEPARATOR, TBB_NONE, _T(""), TBSTYLE_SEP);
        AddToolbarButton(ID_FILE_REMOVE_FILES, TBB_REMOVE_FILES, _T("Remove Files"), TBSTYLE_BUTTON);
        AddToolbarButton(ID_FILE_CLEAR_FILES, TBB_CLEAR_FILES, _T("Clear Files"), TBSTYLE_BUTTON);
        AddToolbarButton(ID_SEPARATOR, TBB_NONE, _T(""), TBSTYLE_SEP);
        AddToolbarButton(ID_TOOLS_OPTIONS, TBB_OPTIONS, _T("Options"), TBSTYLE_BUTTON);
    }

    return TRUE;
}

BOOL CMACDlg::AddToolbarButton(int nID, int nBitmap, const CString & strText, int nStyle)
{
    TBBUTTON Button; memset(&Button, 0, sizeof(Button));
    
    Button.idCommand = nID;
    Button.fsStyle = (BYTE) nStyle;
    Button.iBitmap = nBitmap;
    Button.iString = 0;
    Button.fsState = TBSTATE_ENABLED;
    
    int nIndex = m_ctrlToolbar.GetToolBarCtrl().GetButtonCount();
    
    m_ctrlToolbar.GetToolBarCtrl().InsertButton(nIndex, &Button);
    
    if (strText.IsEmpty() == FALSE)
        m_ctrlToolbar.SetButtonText(nIndex, strText);

    return TRUE;
}

void CMACDlg::OnPaint() 
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

        // center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialog::OnPaint();
    }
}

HCURSOR CMACDlg::OnQueryDragIcon()
{
    return (HCURSOR) m_hIcon;
}

void CMACDlg::OnToolbarDropDown(NMHDR * pnmtb, LRESULT *)
{
    NMTOOLBAR * pNMToolbar = (NMTOOLBAR *) pnmtb;
    CMenu menu;    CMenu * pMenu = NULL;

    // switch on button command id's.
    switch (pNMToolbar->iItem)
    {
        case ID_FILE_PROCESS_FILES:
        {
            menu.LoadMenu(IDR_MAIN_MENU);
            pMenu = menu.GetSubMenu(1);
            pMenu->RemoveMenu(pMenu->GetMenuItemCount() - 1, MF_BYPOSITION);
            pMenu->RemoveMenu(pMenu->GetMenuItemCount() - 1, MF_BYPOSITION);
            break;
        }
        case ID_COMPRESSION:
        {
            menu.LoadMenu(IDR_MAIN_MENU);
            pMenu = menu.GetSubMenu(1);
            if (pMenu)
                pMenu = pMenu->GetSubMenu(6);
            break;
        }
        case ID_STOP:
        {
            menu.LoadMenu(IDR_STOP_MENU);
            pMenu = menu.GetSubMenu(0);
            break;
        }
        case ID_FILE_ADD_FILES:
        {
            menu.CreatePopupMenu(); pMenu = &menu;
            if (theApp.GetSettings()->m_aryAddFilesMRU.GetSize() > 0)
            {
                for (int z = 0; z < theApp.GetSettings()->m_aryAddFilesMRU.GetSize(); z++)
                    menu.AppendMenu(MF_STRING, 1000 + z, theApp.GetSettings()->m_aryAddFilesMRU[z]);
            }
            else
            {
                menu.AppendMenu(MF_STRING | MF_GRAYED, 1000, _T("MRU List Empty"));
            }
            menu.AppendMenu(MF_SEPARATOR);
            menu.AppendMenu(MF_STRING, 1100, _T("Clear MRU List"));
            break;
        }
        case ID_FILE_ADD_FOLDER:
        {
            menu.CreatePopupMenu(); pMenu = &menu;
            if (theApp.GetSettings()->m_aryAddFolderMRU.GetSize() > 0)
            {
                for (int z = 0; z < theApp.GetSettings()->m_aryAddFolderMRU.GetSize(); z++)
                    menu.AppendMenu(MF_STRING, 2000 + z, theApp.GetSettings()->m_aryAddFolderMRU[z]);
            }
            else
            {
                menu.AppendMenu(MF_STRING | MF_GRAYED, 2000, _T("MRU List Empty"));
            }
            menu.AppendMenu(MF_SEPARATOR);
            menu.AppendMenu(MF_STRING, 2100, _T("Clear MRU List"));
            break;
        }
    }
    
    // load and display popup menu
    if (pMenu)
    {
        CRect rectItem;
        m_ctrlToolbar.SendMessage(TB_GETRECT, pNMToolbar->iItem, (LPARAM) &rectItem);
        m_ctrlToolbar.ClientToScreen(&rectItem);
        
        int nRetVal = pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL | TPM_RETURNCMD,
            rectItem.left, rectItem.bottom, this, &rectItem);

        if (pNMToolbar->iItem == ID_FILE_ADD_FILES)
        {
            if (nRetVal >= 1000 && nRetVal < 1100)
            {
                m_strAddFilesBasePath = theApp.GetSettings()->m_aryAddFilesMRU[nRetVal - 1000];
                OnFileAddFiles();
            }
            else if (nRetVal == 1100)
            {
                theApp.GetSettings()->m_aryAddFilesMRU.RemoveAll();
            }
        }
        else if (pNMToolbar->iItem == ID_FILE_ADD_FOLDER)
        {
            if (nRetVal >= 2000 && nRetVal < 2100)
            {
                m_ctrlList.AddFolder(theApp.GetSettings()->m_aryAddFolderMRU[nRetVal - 2000]);
            }
            else if (nRetVal == 2100)
            {
                theApp.GetSettings()->m_aryAddFolderMRU.RemoveAll();
            }
        }
        else
        {
            PostMessage(WM_COMMAND, nRetVal, 0);
        }
    }
}

void CMACDlg::OnFileProcessFiles() 
{
    if (GetProcessing()) 
        return;

    // clear all the files
    for (int z = 0; z < m_aryFiles.GetSize(); z++)
        m_aryFiles[z].bProcessing = FALSE;
    m_ctrlList.Invalidate();

    // process
    if (m_aryFiles.GetSize() <= 0)
    {
        AfxMessageBox(_T("There are no files to process."));
    }
    else
    {
        m_spProcessFiles.Assign(new CMACProcessFiles);
        UpdateWindow();
        m_spProcessFiles->Process(&m_aryFiles);
        m_ctrlStatusBar.StartProcessing();
        SetTimer(IDT_UPDATE_PROGRESS, IDT_UPDATE_PROGRESS_MS, NULL);
    }
}

void CMACDlg::OnFileAddFiles() 
{
    if (GetProcessing())
        return;

    // show the file dialog
    CFileDialog FileDialog(TRUE, NULL, NULL,
        OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY, theApp.GetFormatArray()->GetOpenFilesFilter());
    
    // extra initialization (hacky, but safe)
    CSmartPtr<TCHAR> spFilename(new TCHAR [(_MAX_PATH + 1) * 512], TRUE); spFilename[0] = 0;
    FileDialog.m_ofn.lpstrInitialDir = m_strAddFilesBasePath;
    FileDialog.m_ofn.nMaxFile = _MAX_PATH * 512;
    FileDialog.m_ofn.lpstrFile = spFilename.GetPtr();

    if (FileDialog.DoModal() == IDOK)
    {
        m_ctrlList.StartFileInsertion(FALSE);

        POSITION Pos = FileDialog.GetStartPosition();
        while (Pos != NULL)
        {
            CString strFilename = FileDialog.GetNextPathName(Pos);
            m_ctrlList.AddFileInternal(strFilename);

            m_strAddFilesBasePath = GetDirectory(strFilename);
        }

        m_ctrlList.FinishFileInsertion();

        theApp.GetSettings()->m_aryAddFilesMRU.Remove(m_strAddFilesBasePath, FALSE);
        while (theApp.GetSettings()->m_aryAddFilesMRU.GetSize() >= 4)
            theApp.GetSettings()->m_aryAddFilesMRU.RemoveAt(3);
        theApp.GetSettings()->m_aryAddFilesMRU.InsertAt(0, m_strAddFilesBasePath);
    }
}

void CMACDlg::OnFileAddFolder() 
{
    if (GetProcessing())
        return;

    CFolderDialog FolderDialog(NULL, BIF_RETURNONLYFSDIRS | BIF_EDITBOX);
    if (FolderDialog.DoModal() == IDOK)
    {
        m_ctrlList.AddFolder(FolderDialog.GetPathName());
    }    
}

void CMACDlg::OnFileSelectAll()
{
    m_ctrlList.SelectAll();
}

void CMACDlg::OnFileClearFiles() 
{
    if (GetProcessing())
        return;

    m_ctrlList.RemoveAllFiles();
}

void CMACDlg::OnFileRemoveFiles() 
{
    if (GetProcessing())
        return;

    m_ctrlList.RemoveSelectedFiles();
}

void CMACDlg::OnFileFileInfo()
{
    CStringArray aryFiles;
    m_ctrlList.GetFileList(aryFiles);

    if (aryFiles.GetSize() <= 0)
    {
        AfxMessageBox(_T("There are no files to show information for."));
    }
    else
    {
        CAPEInfoDlg dlgInfo(this, aryFiles);
        dlgInfo.DoModal();
    }
}

void CMACDlg::OnFileExit()
{
    PostMessage(WM_CLOSE);
}

void CMACDlg::OnSize(UINT nType, int cx, int cy) 
{
    CDialog::OnSize(nType, cx, cy);

    LayoutWindow();
}

void CMACDlg::LayoutWindow()
{
    if (m_bInitialized)
    {
        RepositionBars(AFX_IDW_CONTROLBAR_FIRST, AFX_IDW_CONTROLBAR_LAST, 0, reposDefault);

        CRect rectClient; GetClientRect(&rectClient);
        int cx = rectClient.Width(); int cy = rectClient.Height();

        const int nBorderWidth = theApp.GetSize(5, 0).cx;

        int nStatusBarHeight = theApp.GetSize(20, 0).cx;
        m_ctrlStatusBar.SetWindowPos(NULL, nBorderWidth, cy - nStatusBarHeight, cx - nBorderWidth, nStatusBarHeight, SWP_NOZORDER);
        CRect rectStatusBar; m_ctrlStatusBar.GetClientRect(&rectStatusBar);

        CRect rectToolbar; m_ctrlToolbar.GetWindowRect(&rectToolbar); ScreenToClient(&rectToolbar);

        m_ctrlList.SetWindowPos(NULL, nBorderWidth, nBorderWidth + rectToolbar.bottom, cx - (2 * nBorderWidth), cy - nStatusBarHeight - (2 * nBorderWidth) - rectToolbar.Height(), SWP_NOZORDER);

        UpdateWindow();
    }
}

void CMACDlg::UpdateWindow()
{
    if (GetProcessing())
    {
        LoadMenuAndToolbar(TRUE);
        m_ctrlToolbar.GetToolBarCtrl().CheckButton(ID_PAUSE, m_spProcessFiles->GetPaused());
    }
    else
    {
        LoadMenuAndToolbar(FALSE);
        m_ctrlStatusBar.UpdateProgress(0, 0);
        m_ctrlToolbar.SetButtonText(TBI(ID_FILE_PROCESS_FILES), theApp.GetSettings()->GetModeName());
        
        // compression level
        CString strCompressionName = theApp.GetSettings()->GetCompressionName();
        if (strCompressionName.IsEmpty())
        {
            m_ctrlToolbar.SetButtonText(TBI(ID_COMPRESSION), _T("n/a"));
            m_ctrlToolbar.GetToolBarCtrl().EnableButton(ID_COMPRESSION, FALSE);
        }
        else
        {
            m_ctrlToolbar.SetButtonText(TBI(ID_COMPRESSION), strCompressionName);
            m_ctrlToolbar.GetToolBarCtrl().EnableButton(ID_COMPRESSION, TRUE);
        }

        // toolbar icons
        int nModeBitmap = TBB_COMPRESS;
        switch (theApp.GetSettings()->GetMode())
        {
            case MODE_COMPRESS: nModeBitmap = TBB_COMPRESS; break;
            case MODE_DECOMPRESS: nModeBitmap = TBB_DECOMPRESS; break;
            case MODE_VERIFY: nModeBitmap = TBB_VERIFY; break;
            case MODE_CONVERT: nModeBitmap = TBB_CONVERT; break;
            case MODE_MAKE_APL: nModeBitmap = TBB_MAKE_APL; break;
        }
        SetToolbarButtonBitmap(ID_FILE_PROCESS_FILES, nModeBitmap);

        int nCompressionBitmap = TBB_COMPRESSION_EXTERNAL;
        if (theApp.GetSettings()->GetFormat() == COMPRESSION_APE)
        {
            switch (theApp.GetSettings()->GetLevel())
            {
                case MAC_COMPRESSION_LEVEL_FAST: nCompressionBitmap = TBB_COMPRESSION_APE_FAST; break;
                case MAC_COMPRESSION_LEVEL_NORMAL: nCompressionBitmap = TBB_COMPRESSION_APE_NORMAL; break;
                case MAC_COMPRESSION_LEVEL_HIGH: nCompressionBitmap = TBB_COMPRESSION_APE_HIGH; break;
                case MAC_COMPRESSION_LEVEL_EXTRA_HIGH: nCompressionBitmap = TBB_COMPRESSION_APE_EXTRA_HIGH; break;
                case MAC_COMPRESSION_LEVEL_INSANE: nCompressionBitmap = TBB_COMPRESSION_APE_INSANE; break;
            }
        }
        SetToolbarButtonBitmap(ID_COMPRESSION, nCompressionBitmap);
    }

    // redraw the list
    m_ctrlList.Invalidate();
}

void CMACDlg::SetToolbarButtonBitmap(int nID, int nBitmap)
{
    TBBUTTONINFO ButtonInfo; 
    memset(&ButtonInfo, 0, sizeof(ButtonInfo));
    ButtonInfo.cbSize = sizeof(ButtonInfo);
    ButtonInfo.dwMask = TBIF_IMAGE;
    m_ctrlToolbar.GetToolBarCtrl().GetButtonInfo(nID, &ButtonInfo);
    ButtonInfo.iImage = nBitmap;
    m_ctrlToolbar.GetToolBarCtrl().SetButtonInfo(nID, &ButtonInfo);
}

void CMACDlg::OnTimer(UINT_PTR nIDEvent) 
{
    if (nIDEvent == IDT_UPDATE_PROGRESS)
    {
        KillTimer(IDT_UPDATE_PROGRESS);

        // take inventory of what's running
        double dProgress = 0; double dSecondsLeft = 0;
        if (m_aryFiles.GetProcessingProgress(dProgress, dSecondsLeft, m_spProcessFiles->GetPausedTotalMS()))
        {
            m_ctrlStatusBar.UpdateProgress(dProgress, dSecondsLeft);
        }

        // analyze
        int nRunning = 0; BOOL bAllDone = TRUE;
        m_aryFiles.GetProcessingInfo(m_spProcessFiles->GetStopped(), nRunning, bAllDone);

        // start new ones if necessary
        if ((m_spProcessFiles->GetStopped() == FALSE) && (m_spProcessFiles->GetPaused() == FALSE))
        {
            for (int z = 0; (z < m_aryFiles.GetSize()) && (nRunning < theApp.GetSettings()->m_nProcessingSimultaneousFiles); z++)
            {
                MAC_FILE * pInfo = &m_aryFiles[z];
                if (pInfo->GetNeverStarted())
                {
                    m_spProcessFiles->ProcessFile(z);
                    m_ctrlList.EnsureVisible(z, FALSE);
                    nRunning++;
                }
            }
        }
        
        // output our progress
        for (int z = 0; z < m_aryFiles.GetSize(); z++)
        {
            MAC_FILE * pInfo = &m_aryFiles[z];
            if (pInfo && (pInfo->bNeedsUpdate || pInfo->GetRunning()))
            {
                CRect rectItem;
                m_ctrlList.GetItemRect(z, &rectItem, LVIR_BOUNDS);
                m_ctrlList.InvalidateRect(&rectItem);
                pInfo->bNeedsUpdate = FALSE;
            }
        }

        // handle completion (or restart the timer if we're not done)
        if (bAllDone == FALSE)
        {
            SetTimer(IDT_UPDATE_PROGRESS, IDT_UPDATE_PROGRESS_MS, NULL);
        }
        else
        {
            // mark files as done
            for (int z = 0; z < m_aryFiles.GetSize(); z++)
            {
                if (m_aryFiles[z].bDone == FALSE)
                {
                    m_aryFiles[z].bDone = TRUE;
                    m_aryFiles[z].nRetVal = ERROR_USER_STOPPED_PROCESSING;
                }
            }

            // clean up
            m_spProcessFiles.Delete();
            UpdateWindow();
            m_ctrlStatusBar.SetLastProcessTotalMS(GetTickCount() - m_aryFiles.m_dwStartProcessingTickCount);
            m_ctrlStatusBar.UpdateFiles(&m_aryFiles);
            m_ctrlStatusBar.EndProcessing();

            // play sound
            if (theApp.GetSettings()->m_bProcessingPlayCompletionSound)
                PlayDefaultSound();
        }
    }

    CDialog::OnTimer(nIDEvent);
}

BOOL CMACDlg::SetMode(MAC_MODES Mode)
{
    BOOL bRetVal = FALSE;

    if (Mode != theApp.GetSettings()->GetMode())
    {
        // set the mode
        MAC_MODES OriginalMode = theApp.GetSettings()->GetMode();
        theApp.GetSettings()->SetMode(Mode);

        // get the supported extensions
        CStringArrayEx aryExtensions;
        theApp.GetFormatArray()->GetInputExtensions(aryExtensions);

        // remove unsupported files (if any)
        CIntArray aryUnsupportedFiles;
        for (int z = 0; z < m_aryFiles.GetSize(); z++)
        {
            if (aryExtensions.Find(GetExtension(m_aryFiles[z].strInputFilename)) == -1)
                aryUnsupportedFiles.Add(z);
        }
        aryUnsupportedFiles.SortDescending();

        if (aryUnsupportedFiles.GetSize() > 0)
        {
            if (MessageBox(_T("The new mode does not support all of the files in the list.  Do you want to remove these unsupported files and continue?"), _T("Confirm Change Modes"), MB_YESNO) == IDYES)
            {
                for (int z = 0; z < aryUnsupportedFiles.GetSize(); z++)
                {
                    m_aryFiles.RemoveAt(aryUnsupportedFiles[z]);
                }

                m_ctrlList.Update();
            }
            else
            {
                theApp.GetSettings()->SetMode(OriginalMode);
            }
        }

        // set the mode in the list
        m_ctrlList.SetMode(Mode);

        // update if successful
        if (theApp.GetSettings()->GetMode() != OriginalMode)
        {
            UpdateWindow();
            bRetVal = TRUE;
        }
    }

    return bRetVal;
}

BOOL CMACDlg::SetAPECompressionLevel(int nAPECompressionLevel)
{
    theApp.GetSettings()->SetCompression(COMPRESSION_APE, nAPECompressionLevel);
    UpdateWindow();
    return TRUE;
}


void CMACDlg::OnCompression()
{
    NMTOOLBAR ToolbarNotification; memset(&ToolbarNotification, 0, sizeof(ToolbarNotification));
    ToolbarNotification.iItem = ID_COMPRESSION;

    LRESULT nRetVal = 0;
    OnToolbarDropDown((NMHDR *) &ToolbarNotification, &nRetVal);
}

void CMACDlg::OnModeCompress() 
{
    SetMode(MODE_COMPRESS);
}

void CMACDlg::OnModeDecompress() 
{
    SetMode(MODE_DECOMPRESS);
}

void CMACDlg::OnModeVerify() 
{
    SetMode(MODE_VERIFY);
}

void CMACDlg::OnModeConvert()
{
    SetMode(MODE_CONVERT);
}

void CMACDlg::OnModeMakeAPL() 
{
    SetMode(MODE_MAKE_APL);
}

void CMACDlg::OnStop()
{
    NMTOOLBAR ToolbarNotification; memset(&ToolbarNotification, 0, sizeof(ToolbarNotification));
    ToolbarNotification.iItem = ID_STOP;

    LRESULT nRetVal = 0;
    OnToolbarDropDown((NMHDR *) &ToolbarNotification, &nRetVal);
}

void CMACDlg::OnCompressionAPEFast() 
{
    SetAPECompressionLevel(MAC_COMPRESSION_LEVEL_FAST);
}

void CMACDlg::OnCompressionAPENormal() 
{
    SetAPECompressionLevel(MAC_COMPRESSION_LEVEL_NORMAL);
}

void CMACDlg::OnCompressionAPEHigh() 
{
    SetAPECompressionLevel(MAC_COMPRESSION_LEVEL_HIGH);
}

void CMACDlg::OnCompressionAPEExtraHigh() 
{
    SetAPECompressionLevel(MAC_COMPRESSION_LEVEL_EXTRA_HIGH);
}

void CMACDlg::OnCompressionAPEInsane() 
{
    SetAPECompressionLevel(MAC_COMPRESSION_LEVEL_INSANE);
}

void CMACDlg::OnSetCompressionMenu(UINT nID)
{
    theApp.GetFormatArray()->ProcessCompressionMenu(nID);
    UpdateWindow();
}

void CMACDlg::OnDestroy()
{
    // stop file processing
    if (m_spProcessFiles != NULL)
        m_spProcessFiles->Stop(true);
    m_spProcessFiles.Delete();

    // save window placement
    WINDOWPLACEMENT WindowPlacement; memset(&WindowPlacement, 0, sizeof(WindowPlacement));
    if (GetWindowPlacement(&WindowPlacement))
    {
        theApp.GetSettings()->SaveSetting("Main Window Placement", &WindowPlacement, sizeof(WindowPlacement));
    }

    // unload the formats
    theApp.GetFormatArray()->Unload();

    CDialog::OnDestroy();
}

void CMACDlg::OnPause() 
{
    if (GetProcessing())
    {
        m_spProcessFiles->Pause(m_spProcessFiles->GetPaused() ? FALSE : TRUE);
        UpdateWindow();
    }
}

void CMACDlg::OnStopAfterCurrentFile() 
{
    if (GetProcessing())
    {
        m_spProcessFiles->Stop(false);
        UpdateWindow();
    }
}

void CMACDlg::OnStopImmediately() 
{
    if (GetProcessing())
    {
        m_spProcessFiles->Stop(true);
        UpdateWindow();
    }
}

void CMACDlg::OnHelpHelp()
{
    ShellExecute(NULL, NULL, _T("https://www.monkeysaudio.com/help.html"), NULL, NULL, SW_MAXIMIZE);
}

void CMACDlg::OnHelpAbout() 
{
    CAboutDlg dlgAbout;
    dlgAbout.DoModal();
}

void CMACDlg::OnHelpWebsiteMonkeysAudio() 
{
    ShellExecute(NULL, NULL, _T("https://www.monkeysaudio.com"), NULL, NULL, SW_MAXIMIZE);
}

void CMACDlg::OnHelpWebsiteJRiver() 
{
    ShellExecute(NULL, NULL, _T("https://www.jriver.com"), NULL, NULL, SW_MAXIMIZE);
}

void CMACDlg::OnHelpWebsiteWinamp() 
{
    ShellExecute(NULL, NULL, _T("https://www.winamp.com"), NULL, NULL, SW_MAXIMIZE);
}

void CMACDlg::OnHelpWebsiteEac() 
{
    ShellExecute(NULL, NULL, _T("https://www.exactaudiocopy.de/eac.html"), NULL, NULL, SW_MAXIMIZE);
}

void CMACDlg::OnToolsOptions() 
{
    COptionsDlg dlgOptions(this);
    if (dlgOptions.DoModal() == IDOK)
    {
        // update settings...
    }
}

CSize CMACDlg::MeasureText(const CString & strText)
{
    CRect rectSize;

    CPaintDC dc(this);
    dc.SelectObject(m_Font);
    dc.DrawText(strText, &rectSize, DT_NOPREFIX | DT_CALCRECT);
    return rectSize.Size();
}

BOOL CMACDlg::PreTranslateMessage(MSG * pMsg) 
{
    if (TranslateAccelerator(m_hWnd, m_hAcceleratorTable, pMsg) != 0)
        return TRUE;
    
    return CDialog::PreTranslateMessage(pMsg);
}

void CMACDlg::OnInitMenu(CMenu * pMenu) 
{
    CDialog::OnInitMenu(pMenu);
}

void CMACDlg::OnInitMenuPopup(CMenu * pPopupMenu, UINT nIndex, BOOL bSysMenu) 
{
    CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

    // compression menu
    if (pPopupMenu->GetMenuItemID(0) == ID_COMPRESSION_NA)
    {
        pPopupMenu->RemoveMenu(0, MF_BYPOSITION);
        theApp.GetFormatArray()->FillCompressionMenu(pPopupMenu);
    }

    // check the mode
    int nModeID = -1;
    switch (theApp.GetSettings()->GetMode())
    {
        case MODE_COMPRESS: nModeID = ID_MODE_COMPRESS; break;
        case MODE_DECOMPRESS: nModeID = ID_MODE_DECOMPRESS; break;
        case MODE_VERIFY: nModeID = ID_MODE_VERIFY; break;
        case MODE_CONVERT: nModeID = ID_MODE_CONVERT; break;
        case MODE_MAKE_APL: nModeID = ID_MODE_MAKE_APL; break;
    }
    if (nModeID >= 0)
        pPopupMenu->CheckMenuItem(nModeID, MF_CHECKED | MF_BYCOMMAND);

    // check the compression
    int nCompressionID = -1;
    if (theApp.GetSettings()->GetFormat() == COMPRESSION_APE)
    {
        switch (theApp.GetSettings()->GetLevel())
        {
            case MAC_COMPRESSION_LEVEL_FAST: nCompressionID = ID_COMPRESSION_APE_FAST; break;
            case MAC_COMPRESSION_LEVEL_NORMAL: nCompressionID = ID_COMPRESSION_APE_NORMAL; break;
            case MAC_COMPRESSION_LEVEL_HIGH: nCompressionID = ID_COMPRESSION_APE_HIGH; break;
            case MAC_COMPRESSION_LEVEL_EXTRA_HIGH: nCompressionID = ID_COMPRESSION_APE_EXTRA_HIGH; break;
            case MAC_COMPRESSION_LEVEL_INSANE: nCompressionID = ID_COMPRESSION_APE_INSANE; break;
        }
    }
    if (nCompressionID >= 0)
        pPopupMenu->CheckMenuItem(nCompressionID, MF_CHECKED | MF_BYCOMMAND);

    // enable / disable the compression menu
    pPopupMenu->EnableMenuItem(ID_COMPRESSION_APE_FAST, ((theApp.GetSettings()->GetMode() == MODE_COMPRESS) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    pPopupMenu->EnableMenuItem(ID_COMPRESSION_APE_NORMAL, ((theApp.GetSettings()->GetMode() == MODE_COMPRESS) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    pPopupMenu->EnableMenuItem(ID_COMPRESSION_APE_HIGH, ((theApp.GetSettings()->GetMode() == MODE_COMPRESS) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    pPopupMenu->EnableMenuItem(ID_COMPRESSION_APE_EXTRA_HIGH, ((theApp.GetSettings()->GetMode() == MODE_COMPRESS) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
    pPopupMenu->EnableMenuItem(ID_COMPRESSION_APE_INSANE, ((theApp.GetSettings()->GetMode() == MODE_COMPRESS) ? MF_ENABLED : MF_GRAYED) | MF_BYCOMMAND);
}

void CMACDlg::OnGetMinMaxInfo(MINMAXINFO FAR * lpMMI) 
{
    lpMMI->ptMinTrackSize.x = 320;
    lpMMI->ptMinTrackSize.y = 240;
    
    CDialog::OnGetMinMaxInfo(lpMMI);
}

BOOL CMACDlg::OnQueryEndSession()
{
    return TRUE;
}

void CMACDlg::OnEndSession(BOOL)
{
    SendMessage(WM_CLOSE);
    exit(EXIT_SUCCESS);
}

LRESULT CMACDlg::OnDPIChange(WPARAM, LPARAM)
{
    if (m_bInitialized)
    {
        LoadScale();
        LayoutWindow();
    }
    return TRUE;
}

void CMACDlg::WinHelp(DWORD_PTR, UINT nCmd)
{
    if (nCmd == HELP_CONTEXT)
        OnHelpHelp();
}

void CMACDlg::LayoutControlTop(CWnd * pwndLayout, CRect & rectLayout, bool bOnlyControlWidth, bool bCombobox, CWnd * pwndRight)
{
    if (pwndLayout == NULL)
        return;

    CRect rectWindow;
    pwndLayout->GetWindowRect(&rectWindow);

    // make a standard height
    rectWindow.bottom = rectWindow.top + theApp.GetSize(22, 0).cx;

    // layout
    CRect rectRight;
    if (pwndRight != NULL)
    {
        pwndRight->GetWindowRect(&rectRight);
        pwndRight->SetWindowPos(NULL, rectLayout.right - rectRight.Width(), rectLayout.top, rectRight.Width(), theApp.GetSize(22, 0).cx, SWP_NOZORDER);
        rectLayout.right -= rectRight.Width() + theApp.GetSize(8, 0).cx;
    }
    pwndLayout->SetWindowPos(NULL, rectLayout.left, rectLayout.top, bOnlyControlWidth ? rectWindow.Width() : rectLayout.Width(), bCombobox ? theApp.GetSize(500, 0).cx : rectWindow.Height(), SWP_NOZORDER);
    rectLayout.top += rectWindow.Height(); // control height
    if (bOnlyControlWidth)
        rectLayout.left += rectWindow.Width() + theApp.GetSize(8, 0).cx; // window and border
    else
        rectLayout.top += theApp.GetSize(8, 0).cx; // border
    if (pwndRight != NULL)
    {
        rectLayout.right += rectRight.Width() + theApp.GetSize(8, 0).cx;
    }

    // select nothing
    if (bCombobox)
    {
        CComboBox * pCombo = (CComboBox *) pwndLayout;
        pCombo->SetEditSel(0, 0);
    }
}

void CMACDlg::LayoutControlTopWithDivider(CWnd * pwndLayout, CWnd * pwndDivider, CWnd * pwndImage, CRect & rectLayout)
{
    if (pwndLayout == NULL)
        return;

    CRect rectWindow;
    pwndLayout->GetWindowRect(&rectWindow);
    CRect rectDivider;
    pwndDivider->GetWindowRect(&rectDivider);
    CRect rectImage;
    pwndImage->GetWindowRect(&rectImage);

    // size for text
    CString strWindowText;
    pwndLayout->GetWindowText(strWindowText);
    CDC* pDC = pwndLayout->GetDC();
    pDC->SelectObject(GetFont());
    CSize sizeText = pDC->GetTextExtent(strWindowText);
    pwndLayout->ReleaseDC(pDC);
    rectWindow.right = rectWindow.left + sizeText.cx + theApp.GetSize(8, 0).cx;

    // make a standard height
    rectWindow.bottom = rectWindow.top + theApp.GetSize(22, 0).cx;

    // layout
    pwndLayout->SetWindowPos(NULL, rectLayout.left, rectLayout.top, rectWindow.Width(), rectWindow.Height(), SWP_NOZORDER);
    pwndDivider->SetWindowPos(NULL, rectLayout.left + rectWindow.Width(), rectLayout.top + (rectWindow.Height() / 2) - (rectDivider.Height() / 2), rectLayout.Width() - rectWindow.Width(), rectDivider.Height(), SWP_NOZORDER);
    pwndImage->SetWindowPos(NULL, rectLayout.left, rectLayout.top + rectWindow.Height() + theApp.GetSize(6, 0).cx, rectImage.Width(), rectImage.Height(), SWP_NOZORDER);
    rectLayout.top += rectWindow.Height(); // control height
    rectLayout.top += theApp.GetSize(8, 0).cx; // border

    rectLayout.left += rectImage.Width() + theApp.GetSize(16, 0).cx;
}

void CMACDlg::PlayDefaultSound()
{
    HGLOBAL hResource = LoadResource(AfxGetInstanceHandle(), 
        FindResource(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_COMPLETION_WAVE), _T("WAVE")));
    LPCTSTR pSound = (LPCTSTR) LockResource(hResource);
    sndPlaySound(pSound, SND_MEMORY | SND_ASYNC);
    UnlockResource(hResource);
    FreeResource(hResource);
}

void CMACDlg::LoadScale()
{
    // default to 1.0
    double dScale = 1.0;

    // check the scale
    UINT(STDAPICALLTYPE * pGetDpiForWindow) (IN HWND hwnd);
    HMODULE hUser32 = LoadLibrary(_T("user32.dll"));
    if (hUser32 != NULL)
    {
        (FARPROC &) pGetDpiForWindow = GetProcAddress(hUser32, "GetDpiForWindow");
        if (pGetDpiForWindow != NULL)
        {
            UINT nDPI = pGetDpiForWindow(m_hWnd);
            dScale = double(nDPI) / double(96.0);
        }
        FreeLibrary(hUser32);
    }

    // set the scale
    //dScale = 2.0; // test for high scales
    //dScale = 3.0;
    //dScale = 4.0;
    if (theApp.SetScale(dScale) == false)
        return; // do nothing if the scale didn't change

    // release store image lists
    theApp.DeleteImageLists();

    // toolbar
    if (m_ctrlToolbar.GetSafeHwnd() == NULL)
    {
        m_ctrlToolbar.Create(this, WS_CHILD | WS_VISIBLE | CBRS_TOP | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_BORDER_BOTTOM | TBSTYLE_FLAT);
        INITIALIZE_COMMON_CONTROL(m_ctrlToolbar.GetSafeHwnd());
        m_ctrlToolbar.ModifyStyle(0, TBSTYLE_FLAT);
    }

    // status bar
    if (m_ctrlStatusBar.GetSafeHwnd() == NULL)
    {
        m_ctrlStatusBar.Create(this);
        INITIALIZE_COMMON_CONTROL(m_ctrlStatusBar.GetSafeHwnd());
    }

    // set images
    m_ctrlToolbar.GetToolBarCtrl().SetExtendedStyle(TBSTYLE_EX_DRAWDDARROWS);
    m_ctrlToolbar.GetToolBarCtrl().SetImageList(theApp.GetImageList(CMACApp::Image_Toolbar));

    // set the fonts
    {
        // store the start font (stored since we're going to scale after this)
        if (m_fontStart.GetSafeHandle() == NULL)
        {
            CFont * pCurrentFont = m_ctrlList.GetFont();
            if (pCurrentFont != NULL)
            {
                LOGFONT lf; // used to create the CFont
                pCurrentFont->GetLogFont(&lf);
                m_fontStart.CreateFontIndirect(&lf);
            }
        }

        // build a scaled font
        if (m_fontStart.GetSafeHandle() != NULL)
        {
            LOGFONT lf; // used to create the CFont
            m_fontStart.GetLogFont(&lf);
            lf.lfHeight = theApp.GetSize(lf.lfHeight, 0).cx;
            m_Font.DeleteObject();
            m_Font.CreateFontIndirect(&lf); // create the font

            // set the font to all the controls
            m_ctrlList.SetFont(&m_Font);
            m_ctrlStatusBar.SetFont(&m_Font);
            m_ctrlToolbar.SetFont(&m_Font);
            SetFont(&m_Font);
        }
    }

    // load menu and toolbar
    LoadMenuAndToolbar(FALSE);

    // load columns
    m_ctrlList.LoadColumns();

    // size the toolbar (using the size that it lays out at plus the icon size)
    CRect rectItem;
    m_ctrlToolbar.GetItemRect(0, &rectItem);
    CSize sizeButtons = theApp.GetSize(32, 32);
    m_ctrlToolbar.SetSizes(CSize(rectItem.Width(), rectItem.Height()), sizeButtons);

}