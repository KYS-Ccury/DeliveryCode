#include "pch.h"
#include "CClientSocket.h"
#include "Owner.h"
#include "Protocol.h"
#include "Struct.h"
#include "json.hpp" // JSON 처리를 위해 포함
#include <vector>
#include "CInquiryDlg.h"

using json = nlohmann::json;

extern CInquiryDlg* g_pInquiryDlg;
extern CString g_strOwnerID; // 저장해둔 ID/PW 가져오기
extern CString g_strOwnerPW;

#define WM_USER_NEW_ORDER (WM_USER + 100)

CClientSocket::CClientSocket() {}
CClientSocket::~CClientSocket() {}


void CClientSocket::ConnectForNotify(CString ip, int port) {
    Create(); // 소켓 생성
    Connect(ip, port); // 비동기 연결 시도
}

void CClientSocket::OnConnect(int nErrorCode) {
    if (nErrorCode == 0) {
        // 🚨 연결 성공 시, 서버에 내가 사장님임을 알림 (REQ_LOGIN 활용)
        json req;
        req["client_type"] = (int)ClientType::OWNER;
        req["id"] = std::string(CT2CA(g_strOwnerID));
        req["pw"] = std::string(CT2CA(g_strOwnerPW));

        std::string bodyStr = req.dump();
        PacketHeader hdr;
        hdr.type = (uint8_t)ClientType::OWNER;
        hdr.protocol = htons(CmdCommon::REQ_LOGIN);
        hdr.len = htonl((uint32_t)bodyStr.size());

        Send(&hdr, sizeof(PacketHeader));
        Send(bodyStr.data(), (int)bodyStr.size());
    }
    CAsyncSocket::OnConnect(nErrorCode);
}




void CClientSocket::OnReceive(int nErrorCode) {
    if (nErrorCode != 0) return;

    PacketHeader hdr;
    if (Receive(&hdr, sizeof(PacketHeader)) < (int)sizeof(PacketHeader)) return;

    uint16_t protocol = ntohs(hdr.protocol);
    uint32_t bodyLen = ntohl(hdr.len);

    std::string jsonStr = "";
    if (bodyLen > 0) {
        std::vector<char> buffer(bodyLen);
        int nTotalRecv = 0;
        while (nTotalRecv < (int)bodyLen) {
            int nRecv = Receive(buffer.data() + nTotalRecv, bodyLen - nTotalRecv);
            if (nRecv <= 0) break;
            nTotalRecv += nRecv;
        }
        jsonStr.assign(buffer.begin(), buffer.end());
    }

    try {
        if (!jsonStr.empty()) {
            json res = json::parse(jsonStr);

            if (protocol == CmdOwner::NTF_NEW_ORDER) {
                MessageBeep(MB_ICONINFORMATION);
                ::PostMessage(AfxGetMainWnd()->GetSafeHwnd(), WM_USER_NEW_ORDER, 0, 0);
            }
            // 🚨 [추가] 600: 방 입장 완료 (과거 내역 수신)
            else if (protocol == CmdChat::REQ_CREATE_ROOM && g_pInquiryDlg != nullptr) {
                g_pInquiryDlg->OnReceiveChatHistory(jsonStr);
            }
            // 🚨 [추가] 604: 상대방(또는 나)이 보낸 새 메시지 실시간 수신!
            else if (protocol == CmdChat::NTF_RECV_MSG && g_pInquiryDlg != nullptr) {
                g_pInquiryDlg->OnReceiveNewMsg(jsonStr);
            }
        }
    }
    catch (...) {}

    CAsyncSocket::OnReceive(nErrorCode);
}


void CClientSocket::OnClose(int nErrorCode)
{
    AfxMessageBox(_T("서버와의 연결이 끊어졌습니다."));
    CAsyncSocket::OnClose(nErrorCode);
}

// 🚨 [추가] JSON 전송 함수 구현
bool CClientSocket::SendJson(uint16_t protocol, const json& payload) {
    std::string bodyStr = payload.dump();
    PacketHeader hdr;
    hdr.type = (uint8_t)ClientType::OWNER;
    hdr.protocol = htons(protocol);
    hdr.len = htonl((uint32_t)bodyStr.size());

    if (Send(&hdr, sizeof(PacketHeader)) == SOCKET_ERROR) return false;
    if (Send(bodyStr.data(), (int)bodyStr.size()) == SOCKET_ERROR) return false;
    return true;
}

