#include "pch.h"
#include "Owner.h"
#include "afxdialogex.h"
#include "CSalesDlg.h"
#include "NetClient.h"
#include "Protocol.h"

extern int g_nOwnerId;

IMPLEMENT_DYNAMIC(CSalesDlg, CDialogEx)

CSalesDlg::CSalesDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_SALES_MGR_DIALOG, pParent)
{
}

CSalesDlg::~CSalesDlg()
{
}

void CSalesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

// 2. 메시지 맵 연결
BEGIN_MESSAGE_MAP(CSalesDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_SEARCH_SALES, &CSalesDlg::OnBnClickedBtnSearchSales)
	ON_BN_CLICKED(IDC_BTN_EXPORT_EXCEL, &CSalesDlg::OnBnClickedBtnExportExcel)
	// 4011번(1월)부터 4022번(12월)까지 버튼이 눌리면 OnBnClickedMonthBtn 함수를 실행하라!
	ON_COMMAND_RANGE(IDC_BTN_MONTH_1, IDC_BTN_MONTH_12, &CSalesDlg::OnBnClickedMonthBtn)
END_MESSAGE_MAP()


// ==========================================
// 3. 창 초기화 (리스트 뼈대 만들기)
// ==========================================
BOOL CSalesDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_SALES);
	if (pList)
	{
		// 표 스타일 적용
		pList->SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

		// 매출 내역 헤더 추가
		pList->InsertColumn(0, _T("주문 번호"), LVCFMT_CENTER, 80);
		pList->InsertColumn(1, _T("판매 시간"), LVCFMT_CENTER, 100);
		pList->InsertColumn(2, _T("메뉴명"), LVCFMT_LEFT, 150);
		pList->InsertColumn(3, _T("결제 금액"), LVCFMT_RIGHT, 80);
	}

	return TRUE;
}

void CSalesDlg::OnBnClickedBtnSearchSales()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_SALES);
	if (!pList) return;

	// 🚨 사장님의 resource.h에 정의된 이름(IDC_DATETIME_START)으로 완벽하게 맞췄습니다!
	CDateTimeCtrl* pDateStart = (CDateTimeCtrl*)GetDlgItem(IDC_DATETIME_START);
	CDateTimeCtrl* pDateEnd = (CDateTimeCtrl*)GetDlgItem(IDC_DATETIME_END);

	CTime timeStart, timeEnd;
	pDateStart->GetTime(timeStart);
	pDateEnd->GetTime(timeEnd);

	// 날짜를 서버가 이해할 수 있는 "YYYY-MM-DD" 형태의 문자열로 변환
	CString strStart = timeStart.Format(_T("%Y-%m-%d"));
	CString strEnd = timeEnd.Format(_T("%Y-%m-%d"));

	// 2. 서버로 보낼 JSON 구성
	json req, res;
	req["client_type"] = (int)ClientType::OWNER;
	req["owner_id"] = g_nOwnerId; // 🚨 임시 로그인 ID (실제 연동 시 전역 변수 등으로 교체)
	req["start_date"] = std::string(CT2CA(strStart, CP_UTF8));
	req["end_date"] = std::string(CT2CA(strEnd, CP_UTF8));

	// 3. 서버 요청 및 UI 갱신
	if (CNetClient::SendRequest(CmdOwner::REQ_SALES_STATS, req, res)) {
		if (res["status"] == Status::SUCCESS) {

			// 리스트 초기화
			pList->SetRedraw(FALSE);
			pList->DeleteAllItems();

			// 응답받은 매출 배열(sales_list) 파싱
			if (res.contains("sales_list") && res["sales_list"].is_array()) {
				int i = 0;
				for (const auto& item : res["sales_list"]) {
					// CString으로 변환
					CString strNo;    strNo.Format(_T("%d"), item.value("order_id", 0));
					CString strTime = CA2T(item.value("time", "").c_str(), CP_UTF8);
					CString strMenu = CA2T(item.value("menu_name", "").c_str(), CP_UTF8);

					CString strPrice; strPrice.Format(_T("%d"), item.value("price", 0));

					// 리스트에 삽입
					int nIdx = pList->InsertItem(i++, strNo);
					pList->SetItemText(nIdx, 1, strTime);
					pList->SetItemText(nIdx, 2, strMenu);
					pList->SetItemText(nIdx, 3, strPrice + _T("원"));
				}
			}
			pList->SetRedraw(TRUE);

			// 4. 상단 총 매출액 텍스트 업데이트
			int totalRev = res.value("total_revenue", 0);
			int totalCnt = res.value("total_count", 0);

			CString strTotalMsg;
			strTotalMsg.Format(_T("총 %d건 / 누적 매출: %d원"), totalCnt, totalRev);

			// 🚨 사장님의 resource.h에 있는 IDC_STATIC_TOTAL_SALES 사용
			SetDlgItemText(IDC_STATIC_TOTAL_SALES, strTotalMsg);

		}
		else {
			AfxMessageBox(_T("매출 데이터를 가져오지 못했습니다."));
		}
	}
	else {
		AfxMessageBox(_T("서버와의 통신에 실패했습니다."));
	}
}

void CSalesDlg::OnBnClickedMonthBtn(UINT nID)
{
	// 누른 버튼 번호를 가지고 몇 월인지 계산합니다. (1~12)
	int nMonth = nID - IDC_BTN_MONTH_1 + 1;

	// 현재 시간(올해 연도) 가져오기
	CTime curTime = CTime::GetCurrentTime();
	int nYear = curTime.GetYear();

	// 시작일 세팅: O월 1일 0시 0분 0초
	CTime startTime(nYear, nMonth, 1, 0, 0, 0);

	// 종료일 세팅: 다음 달 1일에서 딱 1초를 빼면 이번 달 말일이 됩니다! (윤달 자동 계산)
	int nNextMonth = nMonth + 1;
	int nNextYear = nYear;
	if (nNextMonth > 12) { nNextMonth = 1; nNextYear++; }
	CTime endTime(nNextYear, nNextMonth, 1, 0, 0, 0);
	endTime -= CTimeSpan(0, 0, 0, 1);

	// 화면에 있는 두 개의 달력 컨트롤에 날짜를 밀어넣습니다.
	CDateTimeCtrl* pStart = (CDateTimeCtrl*)GetDlgItem(IDC_DATETIME_START);
	CDateTimeCtrl* pEnd = (CDateTimeCtrl*)GetDlgItem(IDC_DATETIME_END);
	if (pStart) pStart->SetTime(&startTime);
	if (pEnd) pEnd->SetTime(&endTime);

	// 자동으로 조회 버튼을 누른 효과를 줍니다.
	OnBnClickedBtnSearchSales();
}

// ==========================================
// 3. 엑셀 파일로 받기 (.csv 변환)
// ==========================================
void CSalesDlg::OnBnClickedBtnExportExcel()
{
	CListCtrl* pList = (CListCtrl*)GetDlgItem(IDC_LIST_SALES);
	if (!pList || pList->GetItemCount() == 0)
	{
		AfxMessageBox(_T("저장할 판매 내역이 없습니다. 먼저 조회를 해주세요."));
		return;
	}

	// 1. 저장할 곳을 묻는 윈도우 기본 팝업창 띄우기 (CSV 형식)
	CFileDialog dlg(FALSE, _T("csv"), _T("매출내역"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Excel CSV 파일 (*.csv)|*.csv|모든 파일 (*.*)|*.*||"));

	if (dlg.DoModal() == IDOK)
	{
		CString strPath = dlg.GetPathName();
		CStdioFile file;

		// 2. 파일 쓰기 모드로 열기
		if (file.Open(strPath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary))
		{
			// 3. 엑셀에서 한글이 깨지지 않도록 UTF-8 BOM(서명)을 파일 맨 앞에 심어줍니다.
			BYTE bom[] = { 0xEF, 0xBB, 0xBF };
			file.Write(bom, 3);

			// 4. 엑셀의 헤더(첫 줄) 쓰기 (쉼표로 열을 구분합니다)
			CStringA strHeader = "주문 번호,판매 시간,메뉴명,결제 금액\n";
			file.Write(strHeader, strHeader.GetLength());

			// 5. 리스트에 있는 데이터를 한 줄씩 엑셀에 적기
			int nCount = pList->GetItemCount();
			for (int i = 0; i < nCount; i++)
			{
				CString strCol0 = pList->GetItemText(i, 0);
				CString strCol1 = pList->GetItemText(i, 1);
				CString strCol2 = pList->GetItemText(i, 2);
				CString strCol3 = pList->GetItemText(i, 3);

				// 가격에 콤마가 있으면 엑셀 셀이 분리되므로 제거해 줍니다.
				strCol3.Replace(_T(","), _T(""));

				// 한 줄로 합치기
				CString strLine;
				strLine.Format(_T("%s,%s,%s,%s\n"), strCol0, strCol1, strCol2, strCol3);

				// 유니코드(CString)를 UTF-8 문자열로 변환 후 저장 (가장 안전한 한글 저장법)
				CW2A utf8String(strLine, CP_UTF8);
				file.Write(utf8String.m_psz, strlen(utf8String.m_psz));
			}

			file.Close();
			AfxMessageBox(_T("엑셀 파일이 성공적으로 저장되었습니다!\n바탕화면 등 지정하신 폴더에서 열어보세요."));
		}
	}
}

