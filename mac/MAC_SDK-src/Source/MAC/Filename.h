#pragma once

/**************************************************************************************************
CFilename - a filename wrapper to provide easy path splitting and other tools
**************************************************************************************************/
class CFilename  
{
public:
    /**************************************************************************************************
    Construction / Destruction
    **************************************************************************************************/
    CFilename()
    {
        SetFilename(_T(""));
    }

    CFilename(LPCTSTR lpszFilename)
    {
        SetFilename(lpszFilename);
    }

    virtual ~CFilename()
    {
    }

    /**************************************************************************************************
    Operators
    **************************************************************************************************/
    const CFilename & operator=(LPCTSTR lpszFilename)
    {
        SetFilename(lpszFilename);
        return *this;
    }

    /**************************************************************************************************
    Set
    **************************************************************************************************/
    void SetFilename(LPCTSTR lpszFilename)
    {
        m_strFilename = lpszFilename;
        
        TCHAR cDrive[_MAX_DRIVE], cDir[_MAX_DIR], cName[_MAX_FNAME], cExt[_MAX_EXT];
        cDrive[0] = 0; cDir[0] = 0; cName[0] = 0; cExt[0] = 0;
        _tsplitpath_s(m_strFilename, cDrive, _MAX_DRIVE, cDir, _MAX_DIR, cName, _MAX_FNAME, cExt, _MAX_EXT);

        m_strDrive = cDrive;
        m_strDirectory = cDir;
        m_strName = cName;
        m_strExtension = cExt;
    }

    /**************************************************************************************************
    Simple queries
    **************************************************************************************************/
    inline const CString & GetFilename() { return m_strFilename; }
    inline const CString & GetDrive() { return m_strDrive; }
    inline const CString & GetDirectory() { return m_strDirectory; }
    inline const CString & GetName() { return m_strName; }
    inline const CString & GetExtension() { return m_strExtension; }
    inline CString GetPath() { return m_strDrive + m_strDirectory; }
    inline CString GetNameAndExtension() { return m_strName + m_strExtension; }
    
    /**************************************************************************************************
    Advanced queries
    **************************************************************************************************/
    CString BuildFilename(LPCTSTR lpszDrive = NULL, LPCTSTR lpszDirectory = NULL, LPCTSTR lpszName = NULL, LPCTSTR lpszExtension = NULL)
    {
        if (lpszDrive == NULL) lpszDrive = m_strDrive;
        if (lpszDirectory == NULL) lpszDirectory = m_strDirectory;
        if (lpszName == NULL) lpszName = m_strName;
        if (lpszExtension == NULL) lpszExtension = m_strExtension;

        return CString(lpszDrive) + lpszDirectory + lpszName + lpszExtension;
    }

protected:
    /**************************************************************************************************
    Data members
    **************************************************************************************************/
    CString m_strFilename;

    CString m_strDrive;
    CString m_strDirectory;
    CString m_strName;
    CString m_strExtension;
};
