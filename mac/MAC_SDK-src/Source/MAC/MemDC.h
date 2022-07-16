#ifndef _MEMDC_H_
#define _MEMDC_H_

//////////////////////////////////////////////////
// CMemoryDC.h - memory DC
//
// This class implements a memory Device Context which allows
// flicker free drawing.

//WITH FIX FROM Feng Yuan
// explanation here:
// The return value from MFC's SelectObject is a pointer to a temporary CBitmap object,
// but GDI's SelectObject always return a handle. 
// Just save the GDI Handle and select it back.


class CMemoryDC : public CDC {
private:


    CBitmap    m_bitmap;        // Offscreen bitmap
    CBitmap* m_pbitmap;       // Offscreen bitmap

    CBitmap* m_poldBitmap; // bitmap originally found in CMemoryDC
    CDC* m_pDC;           // Saves CDC passed in constructor
    CRect      m_rect;          // Rectangle of drawing area.
    BOOL       m_bMemDC;        // TRUE if CDC really is a Memory DC.

    HGDIOBJ    m_hOldBitmap;
    HGDIOBJ    m_hBitmap;
public:


    CMemoryDC(CDC* pDC, const CRect* pRect = NULL) : CDC()
    {
        ASSERT(pDC != NULL);

        // Some initialization
        m_pDC = pDC;

        m_poldBitmap = NULL;
        m_bMemDC = !pDC->IsPrinting();

        // Get the rectangle to draw
        if (pRect == NULL) {
            pDC->GetClipBox(&m_rect);
        }
        else {
            m_rect = *pRect;
        }

        if (m_bMemDC) {

            // Create a Memory DC
            CreateCompatibleDC(pDC);

            pDC->LPtoDP(&m_rect);

            m_bitmap.CreateCompatibleBitmap(pDC, m_rect.Width(), m_rect.Height());

            m_pbitmap = &m_bitmap;
            m_hBitmap = ((HBITMAP)m_pbitmap->GetSafeHandle());

            m_poldBitmap = ((CBitmap*)SelectObject(&m_bitmap));

            m_hOldBitmap = ((HBITMAP)m_poldBitmap->GetSafeHandle());


            SetMapMode(pDC->GetMapMode());
            SetWindowExt(pDC->GetWindowExt());
            SetViewportExt(pDC->GetViewportExt());

            pDC->DPtoLP(&m_rect);

            SetWindowOrg(m_rect.left, m_rect.top);


        }
        else {
            // Make a copy of the relevent parts of the current 
            // DC for printing
            m_bPrinting = pDC->m_bPrinting;
            m_hDC = pDC->m_hDC;
            m_hAttribDC = pDC->m_hAttribDC;
        }



    }

    ~CMemoryDC()
    {

        if (m_bMemDC) {
            // Copy the offscreen bitmap onto the screen.
            m_pDC->BitBlt(m_rect.left, m_rect.top,
                m_rect.Width(), m_rect.Height(),
                this, m_rect.left, m_rect.top, SRCCOPY);

            SelectObject(m_hOldBitmap);
            ::DeleteObject(m_hBitmap);


        }
        else {
            // All we need to do is replace the DC with an illegal
            // value, this keeps us from accidentally deleting the 
            // handles associated with the CDC that was passed to 
            // the constructor.              
            m_hDC = m_hAttribDC = NULL;
        }

    }

    // Allow usage as a pointer    
    CMemoryDC* operator->()
    {
        return this;
    }

    // Allow usage as a pointer    
    operator CMemoryDC* ()
    {
        return this;
    }
};
#endif#pragma once
