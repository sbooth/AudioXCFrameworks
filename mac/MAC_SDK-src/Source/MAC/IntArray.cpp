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
        qsort(GetData(), GetSize(), sizeof(int), SortAscendingCallback);
    }
}

void CIntArray::SortDescending()
{
    if (GetSize() > 1)
    {
        qsort(GetData(), GetSize(), sizeof(int), SortDescendingCallback);
    }
}

int CIntArray::SortAscendingCallback(const void * pNumberA, const void * pNumberB)
{
    return *((int *) pNumberA) - *((int *) pNumberB);
}

int CIntArray::SortDescendingCallback(const void * pNumberA, const void * pNumberB)
{
    return *((int *) pNumberB) - *((int *) pNumberA);
}

