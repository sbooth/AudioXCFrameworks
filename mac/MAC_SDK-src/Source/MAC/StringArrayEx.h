#pragma once

class CStringArrayEx : public CStringArray
{
public:
    CStringArrayEx();
    virtual ~CStringArrayEx();

    const CStringArrayEx & operator=(const CStringArrayEx & arySource);

    int Find(const CString & strFind, bool bMatchCase = false, int nStartElement = 0);
    void Remove(const CString & strRemove, bool bMatchCase = false, int nStartElement = 0);

    void InitFromList(const CString & strList, const CString & strDelimiter = _T(","));
    CString GetList(const CString & strDelimiter = _T(","));

    int GetTotalCharacterLength(bool bAccountForNullTerminators);

    void RemoveDuplicates(bool bMatchCase = false);
    void Append(CStringArrayEx & aryAppend, bool bRemoveDuplicates = false, bool bMatchCase = false);

    void SortAscending();
    void SortDescending();

protected:
    static int SortAscendingCallback(const void * pStringA, const void * pStringB);
    static int SortDescendingCallback(const void * pStringA, const void * pStringB);
};
