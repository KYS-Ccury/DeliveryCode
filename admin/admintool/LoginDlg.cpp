/**
 * LoginDlg.cpp
 * ============================================================
 * ★ 수정사항: 하드코딩 admin/1234 로그인 제거
 *   → 서버 TCP 연결 (10.10.10.122:9000)
 *   → CMD_LOGIN(101) 패킷 송신/응답 수신
 * ============================================================
 */

#include "pch.h"
#include "framework.h"
#include "resource.h"
#include "LoginDlg.h"
#include "admintool.h"      // ★ 추가: GetSocket() 접근
#include "PacketDef.h"      // ★ 추가: CMD_LOGIN, STATUS 상수

 // ★ 서버 연결 정보 (서버 main.cpp 기준 포트 9000)
static const char* SERVER_IP = "10.10.10.122";
// static const char* SERVER_IP = "10.10.10.122";
static const int   SERVER_PORT = 8080;

CLoginDlg::CLoginDlg(CWnd* pParent)
    : CDialogEx(IDD_LOGIN_DLG, pParent)
{
}

CLoginDlg::~CLoginDlg()
{
}

void CLoginDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_ID, m_editId);
    DDX_Control(pDX, IDC_EDIT_PW, m_editPw);
}

BOOL CLoginDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();
    return TRUE;
}

void CLoginDlg::OnBtnLogin()
{
    // --------------------------------------------------------
    // 1) UI에서 입력값 가져오기
    // --------------------------------------------------------
    CString strId, strPw;
    m_editId.GetWindowText(strId);
    m_editPw.GetWindowText(strPw);

    if (strId.IsEmpty() || strPw.IsEmpty())
    {
        AfxMessageBox(_T("아이디와 비밀번호를 입력하세요."));
        return;
    }

    // --------------------------------------------------------
    // 2) App에서 소켓 가져오기
    // --------------------------------------------------------
    CAdminToolApp* pApp = (CAdminToolApp*)AfxGetApp();
    CClientSocket& sock = pApp->GetSocket();

    // --------------------------------------------------------
    // 3) 서버 TCP 연결
    // --------------------------------------------------------
    if (!sock.IsConnected())
    {
        if (!sock.Connect(SERVER_IP, SERVER_PORT))
        {
            CString errMsg;
            errMsg.Format(_T("서버 연결 실패:\n%S"),
                sock.GetLastErrorMsg().c_str());
            AfxMessageBox(errMsg);
            return;
        }
    }

    // --------------------------------------------------------
    // 4) 로그인 패킷 송신 (CMD_LOGIN = 101)
    //    ★ TODO: 서버 CommonDB.cpp 확인 후 JSON 필드명 수정
    // --------------------------------------------------------
    CT2A ansiId(strId);    // CString(유니코드) → char* 변환
    CT2A ansiPw(strPw);

    json loginReq;
    loginReq["login_id"] = std::string(ansiId);// TODO: 서버 필드명 확인
    loginReq["password"] = std::string(ansiPw);     // TODO: 서버 필드명 확인

    // clientType=4(관리자), protocol=101(로그인)
    if (!sock.SendAdminPacket(CMD_LOGIN, loginReq))
    {
        CString errMsg;
        errMsg.Format(_T("로그인 요청 전송 실패:\n%S"),
            sock.GetLastErrorMsg().c_str());
        AfxMessageBox(errMsg);
        sock.Disconnect();
        return;
    }

    // --------------------------------------------------------
    // 5) 로그인 응답 수신
    // --------------------------------------------------------
    RecvResult res = sock.RecvPacket();
    if (!res.success)
    {
        CString errMsg;
        errMsg.Format(_T("로그인 응답 수신 실패:\n%S"),
            sock.GetLastErrorMsg().c_str());
        AfxMessageBox(errMsg);
        sock.Disconnect();
        return;
    }

    // --------------------------------------------------------
    // 6) 응답 처리
    //    ★ TODO: 서버 응답 JSON 스키마 확인 후 수정
    //    예상: { "status": 2000, ... }
    // --------------------------------------------------------
    if (res.body.contains("status"))
    {
        int status = res.body["status"].get<int>();
        if (status == STATUS_SUCCESS)
        {
            // 로그인 성공 → MainDialog로 전환
            EndDialog(IDOK);
            return;
        }
        else if (status == STATUS_UNAUTHORIZED)
        {
            AfxMessageBox(_T("아이디 또는 비밀번호가 틀렸습니다."));
        }
        else
        {
            CString msg;
            msg.Format(_T("로그인 실패 (status: %d)"), status);
            AfxMessageBox(msg);
        }
    }
    else
    {
        // status 필드 없으면 raw 응답 표시 (디버그용)
        CString rawMsg;
        rawMsg.Format(_T("예상치 못한 응답:\n%S"), res.rawBody.c_str());
        AfxMessageBox(rawMsg);
    }

    // 실패해도 연결은 유지 (재시도 가능)
}

void CLoginDlg::OnOK()
{
    // 엔터키 → 로그인 시도
    OnBtnLogin();
}

BEGIN_MESSAGE_MAP(CLoginDlg, CDialogEx)
    ON_BN_CLICKED(IDC_BTN_LOGIN, &CLoginDlg::OnBtnLogin)
END_MESSAGE_MAP()