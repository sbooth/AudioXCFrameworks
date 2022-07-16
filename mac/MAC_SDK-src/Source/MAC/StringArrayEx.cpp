#include "stdafx.h"
#include "MAC.h"
#include "StringArrayEx.h"

CStringArrayEx::CStringArrayEx()
{

}

CStringArrayEx::~CStringArrayEx()
{

}

const CStringArrayEx & CStringArrayEx::operator=(const CStringArrayEx & arySource)
{
    Copy(arySource);
    return *this;
}

int CStringArrayEx::Find(const CString & strFind, BOOL bMatchCase, int nStartElement)
{
    if (bMatchCase)
    {
        for (int z = nStartElement; z < GetSize(); z++)
        {
            if (ElementAt(z).Compare(strFind) == 0)
                return z;
        }
    }
    else
    {
        for (int z = nStartElement; z < GetSize(); z++)
        {
            if (ElementAt(z).CompareNoCase(strFind) == 0)
                return z;
        }
    }

    return -1;
}

void CStringArrayEx::Remove(const CString & strRemove, BOOL bMatchCase, int nStartElement)
{
    int nIndex = Find(strRemove, bMatchCase, nStartElement);
    while (nIndex != -1)
    {
        RemoveAt(nIndex);
        nIndex = Find(strRemove, bMatchCase, nStartElement);
    }
}

void CStringArrayEx::InitFromList(const CString & strList, const CString strDelimiter)
{
    RemoveAll();

    LPCTSTR pHead = (LPCTSTR) strList;
    LPCTSTR pTail = _tcsstr(pHead, strDelimiter);
    while (pTail != NULL)
    {
        Add(CString(pHead, int(pTail - pHead)));
        pHead = pTail + strDelimiter.GetLength();
        pTail = _tcsstr(pHead, strDelimiter);
    }

    if (*pHead != 0)
        Add(pHead);
}

CString CStringArrayEx::GetList(const CString strDelimiter)
{
    // allocate a memory block for the total length
    const int nTotalCharacters = GetTotalCharacterLength(FALSE);
    CString strRetVal;
    LPTSTR pBuffer = strRetVal.GetBuffer(nTotalCharacters + 1 + int(strDelimiter.GetLength() * GetSize()));

    // copy in the strings (with delimiters)
    LPTSTR pCurrent = pBuffer;
    for (int z = 0; z < GetSize(); z++)
    {
        // string
        int nCharacters = ElementAt(z).GetLength();
        memcpy(pCurrent, (LPCTSTR) ElementAt(z), nCharacters * sizeof(TCHAR));
        pCurrent += nCharacters;

        // delimiter
        memcpy(pCurrent, strDelimiter, strDelimiter.GetLength() * sizeof(TCHAR));
        pCurrent += strDelimiter.GetLength();
    }

    // remove the last delimiter
    pCurrent -= strDelimiter.GetLength(); *pCurrent = 0;

    // return the string
    strRetVal.ReleaseBuffer();
    return strRetVal;
}

int CStringArrayEx::GetTotalCharacterLength(BOOL bAccountForNullTerminators)
{
    int nCharacters = 0;
    for (int z = 0; z < GetSize(); z++)
        nCharacters += ElementAt(z).GetLength();
    
    if (bAccountForNullTerminators)
        nCharacters += (int) GetSize();

    return nCharacters;
}

void CStringArrayEx::RemoveDuplicates(BOOL bMatchCase)
{
    for (int z = 0; z < GetSize() - 1; z++)
        Remove(ElementAt(z), bMatchCase, z + 1);
}

void CStringArrayEx::Append(CStringArrayEx & aryAppend, BOOL bRemoveDuplicates, BOOL bMatchCase)
{
    // add the new items
    for (int z = 0; z < aryAppend.GetSize(); z++)
        Add(aryAppend[z]);
    
    // remove duplicates (if specified)
    if (bRemoveDuplicates)
        RemoveDuplicates(bMatchCase);
}

void CStringArrayEx::SortAscending()
{
    if (GetSize() > 1)
    {
        qsort(GetData(), GetSize(), sizeof(CString *), SortAscendingCallback);
    }
}

void CStringArrayEx::SortDescending()
{
    if (GetSize() > 1)
    {
        qsort(GetData(), GetSize(), sizeof(CString *), SortDescendingCallback);
    }
}

int CStringArrayEx::SortAscendingCallback(const void * pStringA, const void * pStringB)
{
    return ((CString *) pStringA)->Compare((LPCTSTR) (*((CString *) pStringB)));
}

int CStringArrayEx::SortDescendingCallback(const void * pStringA, const void * pStringB)
{
    return ((CString *) pStringB)->Compare((LPCTSTR) (*((CString *) pStringA)));
}

