#pragma once
#include <string>
#include <vector>
#include "UserInfo.h"

// ================================================================
//  AuthManager.h  ─  인증 및 세션 관리 (Singleton)
//  [변경점]
//    Login() 오버로드 추가: token, serverUserID를 함께 저장
//    GetToken() : NetworkManager SendPacket 시 헤더에 첨부 가능
// ================================================================
class AuthManager
{
public:
    static AuthManager& GetInstance() {
        static AuthManager instance;
        return instance;
    }

    std::string GetCurrentUserID() const { return m_currentUserID; }
    std::string GetAccessToken()   const { return m_accessToken; }
    bool        IsLoggedIn()       const { return m_isLoggedIn; }

    // ── 기존 (임시/테스트) ─────────────────────────────────────
    bool Login(const std::string& id, const std::string& pw);

    // ── 서버 연동 완성 버전 ────────────────────────────────────
    // token    : 서버 발급 JWT (이후 요청에 첨부)
    // serverID : 서버가 확인한 login_id
    bool Login(const std::string& id, const std::string& pw,
               const std::string& token, const std::string& serverID);

    void Logout();

    // 개인정보 설정
    bool RegisterUser(const std::string& id, const std::string& pw,
                      const std::string& address, int role);
    bool UpdateUserAddress(const std::string& newAddress);
    bool ChangePassword(const std::string& oldPw, const std::string& newPw);

    // 유틸
    std::string GetMaskedInfo(const std::string& rawData);
    bool        EnableAutoLogin(bool enable);
    bool        CheckAutoLogin();
    std::string FindID(const std::string& phoneNumber);
    bool        ResetPassword(const std::string& id, const std::string& phoneNumber);
    void        RefreshToken();

private:
    AuthManager();
    ~AuthManager() {}

    UserInfo    m_currentUser;
    bool        m_isLoggedIn         = false;
    bool        m_isAutoLoginEnabled = false;
    std::string m_accessToken;
    std::string m_currentUserID;  // login_id (서버 확인값)
};
