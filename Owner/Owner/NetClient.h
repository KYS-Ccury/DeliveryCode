#pragma once
#include <afxsock.h>
#include "json.hpp"
#include "Struct.h"
#define Server_port 8080
#define Server_ip _T("10.10.10.122")

using json = nlohmann::json;

class CNetClient
{
public:
    // 단발형 통신 함수: 프로콜 번호와 데이터를 넣으면 서버 응답을 반환합니다.
    static bool SendRequest(int protocol, const json& reqBody, json& resBody);
    static bool UploadImageFile(const CString& strFilePath, std::string& outServerPath);

private:
    static const int SERVER_PORT = Server_port;
	static constexpr const TCHAR* SERVER_IP = Server_ip;
};