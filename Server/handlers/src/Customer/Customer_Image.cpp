// ================================================================
//  Customer_Image.cpp  — 이미지 파일 TCP 전송 핸들러 (신규)
//
//  프로토콜: REQ_GET_IMAGE (217)
//
//  요청: { "image_url": "upload_22_1234567890.jpg" }
//  응답: { "status":2000, "image_url":"...", "data":"<base64>" }
//  실패: { "status":4004, "message":"파일 없음" }
//
//  동작:
//    1. image_url(파일명)로 서버 로컬에서 파일 읽기
//    2. 파일 내용을 base64로 인코딩
//    3. JSON에 담아 클라이언트로 전송
//    → Samba 마운트 불필요, 방화벽 포트 추가 불필요
// ================================================================
#include "CustomerHandler.h"
#include "MariaDBManager.h"
#include "Protocol.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>

using json = nlohmann::json;

// ================================================================
//  base64 인코딩 (표준 구현, 외부 라이브러리 불필요)
// ================================================================
static const char B64_TABLE[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::vector<uint8_t>& data)
{
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);

    size_t i = 0;
    while (i + 2 < data.size()) {
        uint32_t v = ((uint32_t)data[i] << 16) |
                     ((uint32_t)data[i+1] << 8) |
                      (uint32_t)data[i+2];
        out += B64_TABLE[(v >> 18) & 0x3F];
        out += B64_TABLE[(v >> 12) & 0x3F];
        out += B64_TABLE[(v >>  6) & 0x3F];
        out += B64_TABLE[(v      ) & 0x3F];
        i += 3;
    }
    if (i + 1 == data.size()) {
        uint32_t v = (uint32_t)data[i] << 16;
        out += B64_TABLE[(v >> 18) & 0x3F];
        out += B64_TABLE[(v >> 12) & 0x3F];
        out += '=';
        out += '=';
    } else if (i + 2 == data.size()) {
        uint32_t v = ((uint32_t)data[i] << 16) | ((uint32_t)data[i+1] << 8);
        out += B64_TABLE[(v >> 18) & 0x3F];
        out += B64_TABLE[(v >> 12) & 0x3F];
        out += B64_TABLE[(v >>  6) & 0x3F];
        out += '=';
    }
    return out;
}

// ================================================================
//  이미지 파일 경로 결정
//  Session.cpp가 파일을 저장하는 경로와 동일해야 함
//  현재 Session.cpp: m_currentFileName = "upload_uid_time.jpg"
//                    (서버 실행 디렉터리 기준 상대경로)
//
//  → 서버 실행 디렉터리가 /home/lms/공개/bemin/DeliveryCode/Server/build 이면
//    파일도 거기에 저장됨. IMAGE_BASE_DIR를 그에 맞게 설정.
// ================================================================
static const std::string IMAGE_BASE_DIR = ""; // 빈 문자열 = 실행 디렉터리와 동일

static std::string resolveImagePath(const std::string& imageUrl)
{
    // 보안: 경로 탐색 공격 방지 (../ 포함 시 거부)
    if (imageUrl.find("..") != std::string::npos ||
        imageUrl.find('/') != std::string::npos  ||
        imageUrl.find('\\') != std::string::npos) {
        return "";
    }
    return IMAGE_BASE_DIR + imageUrl;
}

// ================================================================
//  handleGetImage  (REQ_GET_IMAGE = 217)
// ================================================================
void CustomerHandler::handleGetImage(Session* session, const std::string& body)
{
    try {
        json req = json::parse(body.empty() ? "{}" : body);
        std::string imageUrl = req.value("image_url", "");

        if (imageUrl.empty()) {
            sendError(session, CmdCustomer::REQ_GET_IMAGE,
                      Status::BAD_REQUEST, "image_url이 비어있습니다.");
            return;
        }

        // 파일 경로 결정 (보안 검증 포함)
        std::string filePath = resolveImagePath(imageUrl);
        if (filePath.empty()) {
            sendError(session, CmdCustomer::REQ_GET_IMAGE,
                      Status::BAD_REQUEST, "유효하지 않은 파일 경로입니다.");
            return;
        }

        // 파일 읽기
        std::ifstream ifs(filePath, std::ios::binary);
        if (!ifs.is_open()) {
            // 파일이 없으면 빈 data로 응답 (클라이언트가 기본 이미지 표시)
            json res;
            res["status"]    = Status::NOT_FOUND;
            res["image_url"] = imageUrl;
            res["data"]      = "";
            session->sendPacket(static_cast<uint8_t>(m_clientType),
                                CmdCustomer::REQ_GET_IMAGE, res.dump());
            std::cerr << "[Image] 파일 없음: " << filePath << "\n";
            return;
        }

        // 파일 전체 읽기 (최대 5MB 제한)
        constexpr size_t MAX_IMAGE_SIZE = 5 * 1024 * 1024;
        ifs.seekg(0, std::ios::end);
        size_t fileSize = (size_t)ifs.tellg();
        ifs.seekg(0, std::ios::beg);

        if (fileSize == 0 || fileSize > MAX_IMAGE_SIZE) {
            sendError(session, CmdCustomer::REQ_GET_IMAGE,
                      Status::BAD_REQUEST, "파일 크기가 유효하지 않습니다.");
            return;
        }

        std::vector<uint8_t> buf(fileSize);
        ifs.read(reinterpret_cast<char*>(buf.data()), fileSize);
        ifs.close();

        // base64 인코딩
        std::string b64 = base64Encode(buf);

        json res;
        res["status"]    = Status::SUCCESS;
        res["image_url"] = imageUrl;
        res["data"]      = b64;

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_GET_IMAGE, res.dump());

        std::cout << "[Image] 전송 완료: " << imageUrl
                  << " (" << fileSize << " bytes → " << b64.size() << " b64)\n";

    } catch (const std::exception& e) {
        std::cerr << "[Image] handleGetImage 예외: " << e.what() << "\n";
        sendError(session, CmdCustomer::REQ_GET_IMAGE,
                  Status::SERVER_ERROR, "서버 오류");
    }
}
