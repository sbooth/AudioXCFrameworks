#pragma once

namespace APE
{

/**************************************************************************************************
CSmartPtr - a simple smart pointer class that can automatically initialize and free memory
    note: (doesn't do garbage collection / reference counting because of the many pitfalls)
**************************************************************************************************/
template <class TYPE> class CSmartPtr
{
public:
    TYPE * m_pObject;
    bool m_bArray;
    bool m_bDelete;

    CSmartPtr()
    {
        m_bDelete = true;
        m_pObject = NULL;
        m_bArray = false;
    }
    CSmartPtr(TYPE * pObject, bool bArray = false, bool bDelete = true)
    {
        m_bDelete = true;
        m_pObject = NULL;
        m_bArray = false;
        Assign(pObject, bArray, bDelete);
    }

    ~CSmartPtr()
    {
        Delete();
    }

    void Assign(TYPE * pObject, bool bArray = false, bool bDelete = true)
    {
        Delete();

        m_bDelete = bDelete;
        m_bArray = bArray;
        m_pObject = pObject;
    }

    void Delete()
    {
        if (m_bDelete && m_pObject)
        {
            if (m_bArray)
                delete [] m_pObject;
            else
                delete m_pObject;

            m_pObject = NULL;
        }
    }

    void SetDelete(const bool bDelete)
    {
        m_bDelete = bDelete;
    }

    __forceinline TYPE * GetPtr() const
    {
        return m_pObject;
    }

    __forceinline operator TYPE * () const
    {
        return m_pObject;
    }

    __forceinline TYPE * operator ->() const
    {
        return m_pObject;
    }

    // declare assignment, but don't implement (compiler error if we try to use)
    // that way we can't carelessly mix smart pointers and regular pointers
    __forceinline void * operator =(void *) const;
};

}
