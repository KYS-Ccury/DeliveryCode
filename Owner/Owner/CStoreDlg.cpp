#pragma execution_character_set("utf-8")
#include "pch.h"
#include "Owner.h"
#include "afxdialogex.h"
#include "CStoreDlg.h"
#include "resource.h"
#include "NetClient.h"
#include "Protocol.h"

using json = nlohmann::json;

// 🚨 [추가] 진짜 사장님 ID 연동
//extern int 1;

IMPLEMENT_DYNAMIC(CStoreDlg, CDialogEx)

CStoreDlg::CStoreDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STORE_MGR_DIALOG, pParent) {
}

CStoreDlg::~CStoreDlg() {}

void CStoreDlg::DoDataExchange(CDataExchange* pDX) { CDialogEx::DoDataExchange(pDX); }

BEGIN_MESSAGE_MAP(CStoreDlg, CDialogEx)
	ON_NOTIFY(NM_CLICK, IDC_LIST_STORE_MENU, &CStoreDlg::OnNMClickListStoreMenu)
	ON_BN_CLICKED(IDC_BTN_ADD_MENU, &CStoreDlg::OnBnClickedBtnAddMenu)
	ON_BN_CLICKED(IDC_BTN_EDIT_MENU, &CStoreDlg::OnBnClickedBtnEditMenu)
	ON_BN_CLICKED(IDC_BTN_DEL_MENU, &CStoreDlg::OnBnClickedBtnDelMenu)
	ON_BN_CLICKED(IDC_BTN_SELECT_IMAGE, &CStoreDlg::OnBnClickedBtnSelectImage)
END_MESSAGE_MAP()

// ==========================================
// 1. 창이 열릴 때 실행되는 초기화 함수
// ==========================================
BOOL CStoreDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 🚨 [추가] 카테고리 콤보박스 기본 목록 채우기
	CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_CATEGORY);
	if (pCombo) {
		pCombo->AddString(_T("치킨"));
		pCombo->AddString(_T("구이"));
		pCombo->AddString(_T("사이드메뉴"));
		pCombo->AddString(_T("세트메뉴"));
		pCombo->AddString(_T("음료/주류"));
	}

	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
	if (pList)
	{
		pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
		pList->InsertColumn(0, _T("카테고리"), LVCFMT_LEFT, 60);
		pList->InsertColumn(1, _T("메뉴명"), LVCFMT_LEFT, 110);
		pList->InsertColumn(2, _T("가격"), LVCFMT_RIGHT, 0);
		pList->InsertColumn(3, _T("설명"), LVCFMT_LEFT, 0);
		pList->InsertColumn(4, _T("이미지경로"), LVCFMT_LEFT, 0);

		pList->DeleteAllItems();

		json req, res;
		req["client_type"] = (int)ClientType::OWNER;
		req["owner_id"] = 1; // 🚨 [수정] 1 대신 내 진짜 아이디!

		if (CNetClient::SendRequest(CmdOwner::REQ_MENU_LIST, req, res)) {
			if (res["status"] == Status::SUCCESS && res.contains("menus")) {
				for (const auto& item : res["menus"]) {
					CString cat = CA2T(item.value("category", "").c_str(), CP_UTF8);
					CString name = CA2T(item.value("name", "").c_str(), CP_UTF8);
					CString desc = CA2T(item.value("desc", "").c_str(), CP_UTF8);
					CString img = CA2T(item.value("image_url", "").c_str(), CP_UTF8);
					CString price; price.Format(_T("%d"), item.value("price", 0));

					int nIdx = pList->InsertItem(pList->GetItemCount(), cat);
					pList->SetItemText(nIdx, 1, name);
					pList->SetItemText(nIdx, 2, price);
					pList->SetItemText(nIdx, 3, desc);
					pList->SetItemText(nIdx, 4, img);
					pList->SetItemData(nIdx, item.value("menu_id", 0));
				}
			}
		}
	}
	return TRUE;
}

void CStoreDlg::OnNMClickListStoreMenu(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nRow = pNMItemActivate->iItem;

	if (nRow >= 0) {
		CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);

		CString strCategory = pList->GetItemText(nRow, 0);
		CString strMenuName = pList->GetItemText(nRow, 1);
		CString strPrice = pList->GetItemText(nRow, 2);
		CString strDesc = pList->GetItemText(nRow, 3);
		m_strSelectedImagePath = pList->GetItemText(nRow, 4);

		DisplayImage(m_strSelectedImagePath);

		// 콤보박스에 저장된 텍스트 선택하기
		CComboBox* pCombo = (CComboBox*)GetDlgItem(IDC_COMBO_CATEGORY);
		if (pCombo) pCombo->SelectString(-1, strCategory);

		SetDlgItemText(IDC_EDIT_MENU_NAME, strMenuName);
		SetDlgItemText(IDC_EDIT_MENU_PRICE, strPrice);
		SetDlgItemText(IDC_EDIT_MENU_DESC, strDesc);
	}
	*pResult = 0;
}

void CStoreDlg::OnBnClickedBtnAddMenu()
{
	CString strCategory, strMenuName, strPrice, strDesc;
	GetDlgItemText(IDC_COMBO_CATEGORY, strCategory);
	GetDlgItemText(IDC_EDIT_MENU_NAME, strMenuName);
	GetDlgItemText(IDC_EDIT_MENU_PRICE, strPrice);
	GetDlgItemText(IDC_EDIT_MENU_DESC, strDesc);

	if (strCategory.IsEmpty() || strMenuName.IsEmpty() || strPrice.IsEmpty()) {
		AfxMessageBox(_T("카테고리, 메뉴명, 가격은 필수입니다!")); return;
	}

	std::string serverImagePath = "";
	if (!m_strSelectedImagePath.IsEmpty()) {
		// 🚨 [핵심] 실제 파일 업로드는 막아두더라도, 화면의 이미지 경로는 챙겨야 합니다!
		serverImagePath = std::string(CT2CA(m_strSelectedImagePath, CP_UTF8));
	}

	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["owner_id"] = 1;
	req["category"] = std::string(CT2CA(strCategory, CP_UTF8));
	req["name"] = std::string(CT2CA(strMenuName, CP_UTF8));
	req["price"] = _ttoi(strPrice);
	req["desc"] = std::string(CT2CA(strDesc, CP_UTF8));
	req["image_url"] = serverImagePath;

	if (CNetClient::SendRequest(CmdOwner::REQ_ADD_MENU, req, res)) {
		if (res["status"] == Status::SUCCESS) {
			CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
			if (pList) {
				int nIdx = pList->InsertItem(pList->GetItemCount(), strCategory);
				pList->SetItemText(nIdx, 1, strMenuName);
				pList->SetItemText(nIdx, 2, strPrice);
				pList->SetItemText(nIdx, 3, strDesc);
				pList->SetItemText(nIdx, 4, CString(serverImagePath.c_str()));
				pList->SetItemData(nIdx, res.value("menu_id", 0));

				// 등록 성공 후 텍스트 박스 예쁘게 싹 비워주기
				SetDlgItemText(IDC_EDIT_MENU_NAME, _T(""));
				SetDlgItemText(IDC_EDIT_MENU_PRICE, _T(""));
				SetDlgItemText(IDC_EDIT_MENU_DESC, _T(""));
				m_strSelectedImagePath = _T("");
				DisplayImage(_T(""));

				AfxMessageBox(_T("메뉴 등록 완료!"));
			}
		}
		else {
			// 🚨 서버가 알려준 진짜 실패 이유 출력!
			CString errMsg = CA2T(res.value("message", "알 수 없는 에러").c_str(), CP_UTF8);
			AfxMessageBox(_T("메뉴 등록 실패: ") + errMsg);
		}
	}
	else AfxMessageBox(_T("서버 통신 에러"));
}

// ==========================================
// 4. [수정] 버튼 클릭 시 완벽 보완 로직
// ==========================================
void CStoreDlg::OnBnClickedBtnEditMenu()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
	if (!pList) return;

	POSITION pos = pList->GetFirstSelectedItemPosition();
	if (!pos) { AfxMessageBox(_T("수정할 메뉴를 먼저 선택해주세요.")); return; }

	int nItem = pList->GetNextSelectedItem(pos);
	int menuId = (int)pList->GetItemData(nItem);

	CString strCategory, strMenuName, strPrice, strDesc;
	GetDlgItemText(IDC_COMBO_CATEGORY, strCategory);
	GetDlgItemText(IDC_EDIT_MENU_NAME, strMenuName);
	GetDlgItemText(IDC_EDIT_MENU_PRICE, strPrice);
	GetDlgItemText(IDC_EDIT_MENU_DESC, strDesc);

	if (strMenuName.IsEmpty() || strPrice.IsEmpty()) {
		AfxMessageBox(_T("메뉴명과 가격은 필수입니다!")); return;
	}

	std::string serverImagePath = std::string(CT2CA(m_strSelectedImagePath, CP_UTF8));

	// 만약 내 PC의 드라이브 경로(C:\ 등)가 포함되어 있다면 새 사진이므로 업로드!
	if (m_strSelectedImagePath.Find(_T(":\\")) != -1) {
		//
		//if (!CNetClient::UploadImageFile(m_strSelectedImagePath, serverImagePath)) {
		//	AfxMessageBox(_T("새 이미지 업로드 실패!")); return;
		//}
		m_strSelectedImagePath = CString(serverImagePath.c_str()); // 경로를 서버용으로 덮어씀
	}

	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["menu_id"] = menuId;
	req["category"] = std::string(CT2CA(strCategory, CP_UTF8)); // 🚨 [수정] 카테고리 서버 전송 누락 해결!
	req["name"] = std::string(CT2CA(strMenuName, CP_UTF8));
	req["price"] = _ttoi(strPrice);
	req["desc"] = std::string(CT2CA(strDesc, CP_UTF8));
	req["image_url"] = serverImagePath;                         // 🚨 [수정] 새 서버 이미지 경로 전송

	if (CNetClient::SendRequest(CmdOwner::REQ_UPDATE_MENU, req, res) && res["status"] == Status::SUCCESS) {
		pList->SetItemText(nItem, 0, strCategory);
		pList->SetItemText(nItem, 1, strMenuName);
		pList->SetItemText(nItem, 2, strPrice);
		pList->SetItemText(nItem, 3, strDesc);
		pList->SetItemText(nItem, 4, m_strSelectedImagePath);
		AfxMessageBox(_T("메뉴 수정 완료!"));
	}
	else AfxMessageBox(_T("메뉴 수정 실패 (서버 에러)"));
}

void CStoreDlg::OnBnClickedBtnDelMenu()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
	if (!pList) return;

	POSITION pos = pList->GetFirstSelectedItemPosition();
	if (!pos) { AfxMessageBox(_T("삭제할 메뉴를 먼저 선택해주세요.")); return; }

	int nItem = pList->GetNextSelectedItem(pos);

	if (AfxMessageBox(_T("정말 삭제하시겠습니까?"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
		int menuId = (int)pList->GetItemData(nItem);
		json req, res;
		req["client_type"] = (int)ClientType::OWNER;
		req["menu_id"] = menuId;

		if (CNetClient::SendRequest(CmdOwner::REQ_DEL_MENU, req, res) && res["status"] == Status::SUCCESS) {
			pList->DeleteItem(nItem);

			SetDlgItemText(IDC_COMBO_CATEGORY, _T(""));
			SetDlgItemText(IDC_EDIT_MENU_NAME, _T(""));
			SetDlgItemText(IDC_EDIT_MENU_PRICE, _T(""));
			SetDlgItemText(IDC_EDIT_MENU_DESC, _T(""));
			m_strSelectedImagePath = _T("");
			DisplayImage(_T(""));

			AfxMessageBox(_T("메뉴 삭제 완료!"));
		}
		else AfxMessageBox(_T("메뉴 삭제 실패 (서버 에러)"));
	}
}

void CStoreDlg::OnBnClickedBtnSelectImage()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		_T("이미지 파일 (*.jpg;*.png;*.bmp)|*.jpg;*.png;*.bmp|모든 파일 (*.*)|*.*||"));

	if (dlg.DoModal() == IDOK) {
		m_strSelectedImagePath = dlg.GetPathName();
		DisplayImage(m_strSelectedImagePath);
	}
}

void CStoreDlg::DisplayImage(CString strPath)
{
	CStatic* pStatic = (CStatic*)GetDlgItem(IDC_STATIC_MENU_IMAGE);
	if (!pStatic) return;

	pStatic->Invalidate();
	pStatic->UpdateWindow();

	if (strPath.IsEmpty()) return;

	CImage image;
	if (SUCCEEDED(image.Load(strPath))) {
		CClientDC dc(pStatic);
		CRect rect;
		pStatic->GetClientRect(&rect);
		dc.SetStretchBltMode(COLORONCOLOR);
		image.Draw(dc.GetSafeHdc(), rect);
	}
}