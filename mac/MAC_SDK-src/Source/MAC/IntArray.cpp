#include "stdafx.h"
#include "MAC.h"
#include "IntArray.h"

CIntArray::CIntArray()
{
}

CIntArray::~CIntArray()
{
}

void CIntArray::SortAscending()
{
    if (GetSize() > 1)
    {
        qsort(GetData(), static_cast<size_t>(GetSize()), sizeof(ElementAt(0)), SortAscendingCallback);
    }
}

void CIntArray::SortDescending()
{
    if (GetSize() > 1)
    {
        qsort(GetData(), static_cast<size_t>(GetSize()), sizeof(ElementAt(0)), SortDescendingCallback);
    }
}

int CIntArray::SortAscendingCallback(const void * pNumberA, const void * pNumberB)
{
    return *(static_cast<const int *>(pNumberA)) - *(static_cast<const int *>(pNumberB));
}

int CIntArray::SortDescendingCallback(const void * pNumberA, const void * pNumberB)
{
    return *(static_cast<const int *>(pNumberB)) - *(static_cast<const int *>(pNumberA));
}
