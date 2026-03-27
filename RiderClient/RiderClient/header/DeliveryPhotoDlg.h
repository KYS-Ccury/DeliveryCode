#pragma once
#include <afxdialogex.h>
#include <gdiplus.h>

class DeliveryPhotoDlg : public CDialogEx {
    DECLARE_DYNAMIC(DeliveryPhotoDlg)
public:
    DeliveryPhotoDlg(CWnd* pParent = nullptr);
    virtual ~DeliveryPhotoDlg();
    enum { IDD = IDD_DELIVERY_PHOTO_DLG };
protected:
    virtual void DoDataExchange(CDataExchange* pDX) override;
    virtual BOOL OnInitDialog() override;
    DECLARE_MESSAGE_MAP()
    afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
    afx_msg void   OnPaint();
private:
    HBRUSH        m_hBrushBg     = nullptr;
    HWND          m_hParentWnd   = nullptr;   // stored in constructor, safe in destructor
    CString       m_strPhotoPath;
    CBitmap       m_bitmap;
    bool          m_bHasPhoto    = false;
    ULONG_PTR     m_gdiplusToken = 0;

    void LoadAndShowPhoto(const CString& path);

    afx_msg void    OnBtnSelectPhoto();
    afx_msg void    OnBtnSkipPhoto();
    afx_msg void    OnBtnConfirmPhoto();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
};
