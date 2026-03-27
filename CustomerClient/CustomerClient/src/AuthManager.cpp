#include "pch.h"

#include "AuthManager.h"



AuthManager::AuthManager()

    : m_isLoggedIn(false)

    , m_isAutoLoginEnabled(false)

    , m_accessToken("")

    , m_currentUserID("")

{

    m_currentUser.Clear(); // UserInfo의 데이터를 깨끗하게 초기화함.

}



// CUS-01: 회원가입 (ID/PW 기반, 주소 필수)

bool AuthManager::RegisterUser(const std::string& id, const std::string& pw, const std::string& address, int role)

{

    // TODO: 서버 연동 및 DB 저장 로직



    // 테스트를 위해 현재 유저 객체에 정보 세팅

    m_currentUser.id = id;

    m_currentUser.pw = pw;

    m_currentUser.address = address;

    m_currentUser.role = role;



    return true;

}



// CUS-02: 로그인

bool AuthManager::Login(const std::string& id, const std::string& pw)

{

    // TODO: 서버 인증 및 세션 생성

    m_isLoggedIn = true;

    m_currentUserID = id;

    m_accessToken = "dummy_token_12345";

    return true;

}

// 서버 연동 버전 - 토큰 저장
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

    m_isLoggedIn = false;

    m_accessToken = "";

    m_currentUserID = "";

}



// 인증 토큰 관리

std::string AuthManager::GetAccessToken() const

{

    return m_accessToken;

}



void AuthManager::RefreshToken()

{

    // TODO: 토큰 갱신 로직

}



// 아이디 찾기 (전화번호 기준)

std::string AuthManager::FindID(const std::string& phoneNumber)

{

    // TODO: DB 조회 후 ID 반환

    return "found_user_id";

}



// 비밀번호 초기화/찾기

bool AuthManager::ResetPassword(const std::string& id, const std::string& phoneNumber)

{

    // TODO: 본인 확인 후 임시 비번 발송 혹은 재설정 창 연결

    return true;

}



// 자동 로그인 설정 및 체크

bool AuthManager::EnableAutoLogin(bool enable)

{

    m_isAutoLoginEnabled = enable;

    // TODO: 로컬 파일이나 레지스트리에 설정 저장

    return true;

}



bool AuthManager::CheckAutoLogin()

{

    // TODO: 저장된 토큰이 유효한지 확인 후 자동 로그인 처리

    if (m_isAutoLoginEnabled) {

        return true;

    }

    return false;

}



// 고객 정보 마스킹 (3H 방식: 앞 3글자 유지, 나머지는 * 처리)

std::string AuthManager::GetMaskedInfo(const std::string& rawData)

{

    if (rawData.length() <= 3) return rawData;



    std::string masked = rawData.substr(0, 3);

    for (size_t i = 3; i < rawData.length(); ++i) {

        masked += "*";

    }

    return masked;

}



// 고객 개인정보 설정 변경

bool AuthManager::UpdateUserAddress(const std::string& newAddress)

{

    // TODO: 현재 로그인 유저의 주소 정보 서버 업데이트

    return true;

}



bool AuthManager::ChangePassword(const std::string& oldPw, const std::string& newPw)

{

    // TODO: 기존 비번 확인 후 새 비번으로 교체

    return true;

}