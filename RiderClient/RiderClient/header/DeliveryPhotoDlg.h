#pragma once
#include <afxdialogex.h>

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
private:
    CString m_strPhotoPath;

    afx_msg void    OnBtnSelectPhoto();
    afx_msg void    OnBtnSkipPhoto();
    afx_msg void    OnBtnConfirmPhoto();
    afx_msg LRESULT OnSocketRecv(WPARAM w, LPARAM l);
};
