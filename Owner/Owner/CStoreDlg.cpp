#pragma execution_character_set("utf-8")
#include "pch.h"
#include "Owner.h"
#include "afxdialogex.h"
#include "CStoreDlg.h"
#include "resource.h"
#include "NetClient.h"
#include "Protocol.h"

using json = nlohmann::json;

// CStoreDlg 대화 상자
IMPLEMENT_DYNAMIC(CStoreDlg, CDialogEx)

CStoreDlg::CStoreDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_STORE_MGR_DIALOG, pParent)
{
}

CStoreDlg::~CStoreDlg()
{
}

void CStoreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

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
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);

	if (pList)
	{
		// 표 스타일 적용
		pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

		// 컬럼(헤더) 추가 (카테고리, 메뉴명, 가격, 설명은 숨겨두거나 나중에 사용)
		pList->InsertColumn(0, _T("카테고리"), LVCFMT_LEFT, 60);
		pList->InsertColumn(1, _T("메뉴명"), LVCFMT_LEFT, 110);
		pList->InsertColumn(2, _T("가격"), LVCFMT_RIGHT, 0);
		pList->InsertColumn(3, _T("설명"), LVCFMT_LEFT, 0);
		pList->InsertColumn(4, _T("이미지경로"), LVCFMT_LEFT, 0);

		// 🚨 더미 데이터 삭제 후 리스트 비우기 (이제 서버에서 직접 가져옵니다)
		pList->DeleteAllItems();

		// 서버 통신 로직을 pList 안전 구역 안으로 이동
		json req, res;
		req["client_type"] = (int)ClientType::OWNER;
		req["owner_id"] = 1; // 임시 사장님 ID

		if (CNetClient::SendRequest(CmdOwner::REQ_MENU_LIST, req, res)) {
			if (res["status"] == Status::SUCCESS && res.contains("menus")) {
				for (const auto& item : res["menus"]) {
					CString cat = CA2T(item.value("category", "").c_str(), CP_UTF8);
					CString name = CA2T(item.value("name", "").c_str(), CP_UTF8);
					CString desc = CA2T(item.value("desc", "").c_str(), CP_UTF8);
					CString img = CA2T(item.value("image_url", "").c_str(), CP_UTF8);
					CString price; price.Format(_T("%d"), item.value("price", 0));

					// 항목 개수를 이용해 항상 맨 아래에 추가
					int nIdx = pList->InsertItem(pList->GetItemCount(), cat);
					pList->SetItemText(nIdx, 1, name);
					pList->SetItemText(nIdx, 2, price);
					pList->SetItemText(nIdx, 3, desc);
					pList->SetItemText(nIdx, 4, img);

					// 핵심: DB의 menu_id를 몰래 저장
					pList->SetItemData(nIdx, item.value("menu_id", 0));
				}
			}
		}
	}
	return TRUE;
}

// ==========================================
// 2. 리스트 항목 클릭 시 우측 창에 데이터 연동
// ==========================================
void CStoreDlg::OnNMClickListStoreMenu(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	int nRow = pNMItemActivate->iItem;

	if (nRow >= 0)
	{
		CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);

		CString strCategory = pList->GetItemText(nRow, 0);
		CString strMenuName = pList->GetItemText(nRow, 1);
		CString strPrice = pList->GetItemText(nRow, 2);
		CString strDesc = pList->GetItemText(nRow, 3);
		m_strSelectedImagePath = pList->GetItemText(nRow, 4);

		DisplayImage(m_strSelectedImagePath);

		SetDlgItemText(IDC_COMBO_CATEGORY, strCategory);
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

	if (strMenuName.IsEmpty() || strPrice.IsEmpty()) {
		AfxMessageBox(_T("필수 항목 누락!"));
		return;
	}

	// 🚨 [추가] 1단계: 사진이 선택되었다면 서버로 파일 업로드 먼저 수행!
	std::string serverImagePath = "";
	if (!m_strSelectedImagePath.IsEmpty()) {
		// 이 함수 안에서 파일이 서버로 날아갑니다. (잠깐 멈추지만 0.1초 컷!)
		if (!CNetClient::UploadImageFile(m_strSelectedImagePath, serverImagePath)) {
			AfxMessageBox(_T("이미지 업로드에 실패했습니다. 다시 시도해주세요."));
			return; // 사진 업로드 실패하면 메뉴 등록도 중단
		}
	}

	// 🚨 2단계: 기존 메뉴 등록 로직 (이미지 경로는 서버가 준 경로로 세팅!)
	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["owner_id"] = 1; // 임시
	req["category"] = std::string(CT2CA(strCategory, CP_UTF8));
	req["name"] = std::string(CT2CA(strMenuName, CP_UTF8));
	req["price"] = _ttoi(strPrice);
	req["desc"] = std::string(CT2CA(strDesc, CP_UTF8));

	// 내 PC 경로(C:\...)가 아닌 서버 경로(images/...)를 DB로 보냄!
	req["image_url"] = serverImagePath;

	if (CNetClient::SendRequest(CmdOwner::REQ_ADD_MENU, req, res) && res["status"] == Status::SUCCESS) {
		CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
		if (pList) {
			int nIdx = pList->InsertItem(pList->GetItemCount(), strCategory);
			pList->SetItemText(nIdx, 1, strMenuName);
			pList->SetItemText(nIdx, 2, strPrice);
			pList->SetItemText(nIdx, 3, strDesc);
			pList->SetItemText(nIdx, 4, CString(serverImagePath.c_str())); // 리스트에도 서버 경로 표시

			pList->SetItemData(nIdx, res.value("menu_id", 0));
			AfxMessageBox(_T("메뉴 등록 및 사진 업로드 완료!"));
		}
	}
	else {
		AfxMessageBox(_T("메뉴 등록 실패 (서버 에러)"));
	}
}

// ==========================================
// 4. [수정] 버튼 클릭 시
// ==========================================
void CStoreDlg::OnBnClickedBtnEditMenu()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
	if (!pList) return;

	POSITION pos = pList->GetFirstSelectedItemPosition();
	if (!pos) {
		AfxMessageBox(_T("수정할 메뉴를 먼저 리스트에서 선택해주세요."));
		return;
	}
	int nItem = pList->GetNextSelectedItem(pos);

	// 🚨 숨겨둔 고유 메뉴 ID 가져오기
	int menuId = (int)pList->GetItemData(nItem);

	// 🚨 누락되었던 데이터 읽기 로직 추가
	CString strCategory, strMenuName, strPrice, strDesc;
	GetDlgItemText(IDC_COMBO_CATEGORY, strCategory);
	GetDlgItemText(IDC_EDIT_MENU_NAME, strMenuName);
	GetDlgItemText(IDC_EDIT_MENU_PRICE, strPrice);
	GetDlgItemText(IDC_EDIT_MENU_DESC, strDesc);

	if (strMenuName.IsEmpty() || strPrice.IsEmpty()) {
		AfxMessageBox(_T("필수 항목 누락!"));
		return;
	}

	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["menu_id"] = menuId;
	req["name"] = std::string(CT2CA(strMenuName, CP_UTF8));
	req["price"] = _ttoi(strPrice);
	req["desc"] = std::string(CT2CA(strDesc, CP_UTF8));
	req["image_url"] = std::string(CT2CA(m_strSelectedImagePath, CP_UTF8));

	if (CNetClient::SendRequest(CmdOwner::REQ_UPDATE_MENU, req, res) && res["status"] == Status::SUCCESS) {
		pList->SetItemText(nItem, 0, strCategory);
		pList->SetItemText(nItem, 1, strMenuName);
		pList->SetItemText(nItem, 2, strPrice);
		pList->SetItemText(nItem, 3, strDesc);
		pList->SetItemText(nItem, 4, m_strSelectedImagePath); // 누락된 이미지경로 변경 처리
		AfxMessageBox(_T("메뉴 수정 완료!"));
	}
	else {
		AfxMessageBox(_T("메뉴 수정 실패 (서버 에러)"));
	}
}

// ==========================================
// 5. [삭제] 버튼 클릭 시
// ==========================================
void CStoreDlg::OnBnClickedBtnDelMenu()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_STORE_MENU);
	if (!pList) return;

	POSITION pos = pList->GetFirstSelectedItemPosition();
	if (!pos) {
		AfxMessageBox(_T("삭제할 메뉴를 먼저 리스트에서 선택해주세요."));
		return;
	}
	int nItem = pList->GetNextSelectedItem(pos);

	if (AfxMessageBox(_T("정말 삭제하시겠습니까?"), MB_YESNO | MB_ICONQUESTION) == IDYES) {
		int menuId = (int)pList->GetItemData(nItem);
		json req, res;
		req["client_type"] = (int)ClientType::OWNER;
		req["menu_id"] = menuId;

		if (CNetClient::SendRequest(CmdOwner::REQ_DEL_MENU, req, res) && res["status"] == Status::SUCCESS) {
			pList->DeleteItem(nItem);

			// 우측 폼 비우기
			SetDlgItemText(IDC_COMBO_CATEGORY, _T(""));
			SetDlgItemText(IDC_EDIT_MENU_NAME, _T(""));
			SetDlgItemText(IDC_EDIT_MENU_PRICE, _T(""));
			SetDlgItemText(IDC_EDIT_MENU_DESC, _T(""));
			m_strSelectedImagePath = _T("");
			DisplayImage(_T(""));

			AfxMessageBox(_T("메뉴 삭제 완료!"));
		}
		else {
			AfxMessageBox(_T("메뉴 삭제 실패 (서버 에러)"));
		}
	}
}

void CStoreDlg::OnBnClickedBtnSelectImage()
{
	CFileDialog dlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST,
		_T("이미지 파일 (*.jpg;*.png;*.bmp)|*.jpg;*.png;*.bmp|모든 파일 (*.*)|*.*||"));

	if (dlg.DoModal() == IDOK)
	{
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
	if (SUCCEEDED(image.Load(strPath)))
	{
		CClientDC dc(pStatic);
		CRect rect;
		pStatic->GetClientRect(&rect);

		dc.SetStretchBltMode(COLORONCOLOR);
		image.Draw(dc.GetSafeHdc(), rect);
	}
}