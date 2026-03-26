#pragma once
#include "afxdialogex.h"
#include "MenuInfo.h"
#include <vector>

class OptionChangeDlg : public CDialogEx
{
    DECLARE_DYNAMIC(OptionChangeDlg)
public:
    OptionChangeDlg(CWnd* pParent = nullptr);
    virtual ~OptionChangeDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_DIALOG_OPTION_CHANGE };
#endif
    CString                  m_strMenuName;
    int                      m_nQuantity  = 1;
    int                      m_nBasePrice = 0;
    std::vector<OptionGroup> m_vecOptionGroups;
    std::vector<int>         m_vecPreCheckedOptionIDs;  // ★ 추가
    CListCtrl                m_listOptions;
    void UpdateTotal();
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual BOOL OnInitDialog();
    afx_msg void OnBnClickedBtnCountMinus();
    afx_msg void OnBnClickedBtnCountPlus();
    afx_msg void OnLvnItemchangedListOptions(NMHDR* pNMHDR, LRESULT* pResult);
    DECLARE_MESSAGE_MAP()
};
