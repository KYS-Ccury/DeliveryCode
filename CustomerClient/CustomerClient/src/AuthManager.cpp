#include "pch.h"
#include "AuthManager.h"

AuthManager::AuthManager()
    : m_isLoggedIn(false)
    , m_isAutoLoginEnabled(false)
    , m_accessToken("")
    , m_currentUserID("")
{
    m_currentUser.Clear();
}

// 기존 테스트용 (더미 토큰)
bool AuthManager::Login(const std::string& id, const std::string& pw)
{
    m_isLoggedIn    = true;
    m_currentUserID = id;
    m_accessToken   = "dummy_token_12345";
    return true;
}

// 서버 연동 완성 버전
bool AuthManager::Login(const std::string& id, const std::string& pw,
                        const std::string& token, const std::string& serverID)
{
    m_isLoggedIn    = true;
    m_currentUserID = serverID.empty() ? id : serverID;
    m_accessToken   = token;
    m_currentUser.id = id;
    m_currentUser.pw = pw;
    return true;
}

void AuthManager::Logout()
{
    m_isLoggedIn    = false;
    m_accessToken   = "";
    m_currentUserID = "";
    m_currentUser.Clear();
}

bool AuthManager::RegisterUser(const std::string& id, const std::string& pw,
                               const std::string& address, int role)
{
    m_currentUser.id      = id;
    m_currentUser.pw      = pw;
    m_currentUser.address = address;
    m_currentUser.role    = role;
    return true;
}

bool AuthManager::UpdateUserAddress(const std::string& newAddress)
{
    // TODO: 서버 REQ_PROFILE_UPDATE 전송
    return true;
}

bool AuthManager::ChangePassword(const std::string& oldPw, const std::string& newPw)
{
    // TODO: 서버 REQ_CHANGE_PW 전송
    return true;
}

std::string AuthManager::GetMaskedInfo(const std::string& rawData)
{
    if (rawData.length() <= 3) return rawData;
    std::string masked = rawData.substr(0, 3);
    for (size_t i = 3; i < rawData.length(); ++i) masked += "*";
    return masked;
}

bool AuthManager::EnableAutoLogin(bool enable)
{
    m_isAutoLoginEnabled = enable;
    // TODO: 레지스트리 또는 로컬 파일에 토큰 저장
    return true;
}

bool AuthManager::CheckAutoLogin()
{
    if (m_isAutoLoginEnabled && !m_accessToken.empty()) return true;
    return false;
}

std::string AuthManager::FindID(const std::string& phoneNumber) { return ""; }
bool        AuthManager::ResetPassword(const std::string& id, const std::string& phoneNumber) { return true; }
void        AuthManager::RefreshToken() { /* TODO: CmdCommon::REQ_TOKEN_REFRESH */ }
