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

        TCHAR cDrive[_MAX_DRIVE] = { 0 };
        TCHAR cDir[_MAX_DIR] = { 0 };
        TCHAR cName[_MAX_FNAME] = { 0 };
        TCHAR cExt[_MAX_EXT] = { 0 };
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
    inline CString GetPath() const { return m_strDrive + m_strDirectory; }
    inline CString GetNameAndExtension() const { return m_strName + m_strExtension; }

    /**************************************************************************************************
    Advanced queries
    **************************************************************************************************/
    CString BuildFilename(LPCTSTR lpszDrive = APE_NULL, LPCTSTR lpszDirectory = APE_NULL, LPCTSTR lpszName = APE_NULL, LPCTSTR lpszExtension = APE_NULL)
    {
        if (lpszDrive == APE_NULL) lpszDrive = m_strDrive;
        if (lpszDirectory == APE_NULL) lpszDirectory = m_strDirectory;
        if (lpszName == APE_NULL) lpszName = m_strName;
        if (lpszExtension == APE_NULL) lpszExtension = m_strExtension;

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
