#ifndef GDI_BITMAP_PTR_HEADER
#define GDI_BITMAP_PTR_HEADER

/**************************************************************************************************
CGDIBitmapPtr
Doing a regular delete on a Gdiplus::Bitmap object makes Clang warn. So instead we wrap
it up and delete with Gdiplus::Bitmap::operator delete. This was found on 2/5/2025
and no other solution could be found to keep Clang happy. It could be a problem with only
a certain version of Clang so switching to a regular delete and trying in the future might make
sense.
**************************************************************************************************/
class CGDIBitmapPtr
{
public:
    CGDIBitmapPtr()
    {
        m_pBitmap = NULL;
    }
    ~CGDIBitmapPtr()
    {
        Delete();
    }

    __forceinline Gdiplus::Bitmap * operator ->() const
    {
        return m_pBitmap;
    }

    __forceinline operator Gdiplus::Bitmap * () const
    {
        return m_pBitmap;
    }

    __forceinline void Assign(Gdiplus::Bitmap * pObject)
    {
        Delete();
        m_pBitmap = pObject;
    }

    __forceinline void Delete()
    {
        if (m_pBitmap != NULL)
        {
            // this will cause Clang to lose it (2/5/2025)
            //delete m_pBitmap;

            // this is fine
            Gdiplus::Bitmap::operator delete(m_pBitmap);

            // reset pointer
            m_pBitmap = NULL;
        }
    }

private:
    Gdiplus::Bitmap * m_pBitmap;
};

#endif // GDI_BITMAP_PTR_HEADER
