#pragma once

class CStringArrayEx : public CStringArray  
{
public:

    CStringArrayEx();
    virtual ~CStringArrayEx();

    const CStringArrayEx & operator=(const CStringArrayEx & arySource);

    int Find(const CString & strFind, BOOL bMatchCase = FALSE, int nStartElement = 0);
    void Remove(const CString & strRemove, BOOL bMatchCase = FALSE, int nStartElement = 0);
    
    void InitFromList(const CString & strList, const CString strDelimiter = _T(","));
    CString GetList(const CString strDelimiter = _T(","));

    int GetTotalCharacterLength(BOOL bAccountForNullTerminators);

    void RemoveDuplicates(BOOL bMatchCase = FALSE);
    void Append(CStringArrayEx & aryAppend, BOOL bRemoveDuplicates = FALSE, BOOL bMatchCase = FALSE);

    void SortAscending();
    void SortDescending();

protected:
    
    static int SortAscendingCallback(const void * pStringA, const void * pStringB);
    static int SortDescendingCallback(const void * pStringA, const void * pStringB);
};
