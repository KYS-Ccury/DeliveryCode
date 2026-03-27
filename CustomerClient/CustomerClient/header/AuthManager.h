#pragma once

#include <string>

#include <vector>



#include "UserInfo.h" 



// [COMMON] 인증 및 계정 관리 매니저 (Singleton)

class AuthManager

{

public:

    static AuthManager& GetInstance() {

        static AuthManager instance;

        return instance;

    }

    std::string GetCurrentUserID() const { return m_currentUserID; }

    // --- [공통 기능] ---



    // CUS-01: 회원가입 (ID, PW, 주소, ROLE 선택)

    bool RegisterUser(const std::string& id, const std::string& pw, const std::string& address, int role);



    // CUS-02: 로그인 / 로그아웃

    bool Login(const std::string& id, const std::string& pw);
    bool Login(const std::string& id, const std::string& pw,
               const std::string& token, const std::string& serverID);

    void Logout();



    // 인증 토큰 관리 (추가사항)

    std::string GetAccessToken() const;

    void RefreshToken();



    // 아이디 / 비번 찾기 (추가사항)

    std::string FindID(const std::string& phoneNumber);

    bool ResetPassword(const std::string& id, const std::string& phoneNumber);



    // 자동 로그인 (추가사항)

    bool EnableAutoLogin(bool enable);

    bool CheckAutoLogin(); // 앱 시작 시 호출하여 자동 로그인 시도



    // 고객 정보 마스킹 (3H: 앞 3글자 제외 마스킹)

    std::string GetMaskedInfo(const std::string& rawData);





    // --- [고객 기능] ---



    // 개인 정보 설정 (주소 변경, 비밀번호 변경 등)

    bool UpdateUserAddress(const std::string& newAddress);

    bool ChangePassword(const std::string& oldPw, const std::string& newPw);

    //std::string GetCurrentUserID() const { return m_currentUserID; }

private:

    AuthManager();

    ~AuthManager() {}



    UserInfo m_currentUser;



    // 내부 상태 변수

    bool m_isLoggedIn;

    bool m_isAutoLoginEnabled;

    std::string m_accessToken;

    std::string m_currentUserID;

};