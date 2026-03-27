#include "pch.h"
#include "NetClient.h"
#include "Struct.h"
#include "Protocol.h"
#include <vector>
#include <fstream>

bool CNetClient::SendRequest(int protocol, const json& reqBody, json& resBody)
{
    CSocket socket;
    if (!socket.Create()) return false;

    // 1. 서버 연결 (단발성)
    if (!socket.Connect(SERVER_IP, SERVER_PORT)) return false;

    // 2. 바디 데이터 준비
    std::string bodyStr = reqBody.dump();

    // 3. 7바이트 헤더 구성
    PacketHeader hdr;
    hdr.type = (uint8_t)ClientType::OWNER;         // 1바이트 (순서 주의)
    hdr.protocol = htons((uint16_t)protocol);      // 2바이트 (Little -> Big Endian)
    hdr.len = htonl((uint32_t)bodyStr.size());     // 4바이트 (Little -> Big Endian)

    // 4. 전송 (헤더 7바이트 + 바디 n바이트)
    if (socket.Send(&hdr, sizeof(PacketHeader)) == SOCKET_ERROR) return false;
    if (socket.Send(bodyStr.data(), (int)bodyStr.size()) == SOCKET_ERROR) return false;

    // 5. 응답 수신
    PacketHeader resHdr;
    // 먼저 헤더 7바이트를 읽음
    int nRead = socket.Receive(&resHdr, sizeof(PacketHeader));
    if (nRead < (int)sizeof(PacketHeader)) return false;

    // 바디 길이 파싱
    uint32_t resLen = ntohl(resHdr.len);
    if (resLen > 0) {
        std::vector<char> buffer(resLen + 1, 0);
        int nTotalRecv = 0;
        // 남은 바디 데이터를 모두 읽음
        while (nTotalRecv < (int)resLen) {
            int nRecv = socket.Receive(buffer.data() + nTotalRecv, (int)resLen - nTotalRecv);
            if (nRecv <= 0) break;
            nTotalRecv += nRecv;
        }

        try {
            resBody = json::parse(buffer.data());
        }
        catch (...) { return false; }
    }

    return true;
}

bool CNetClient::UploadImageFile(const CString& strFilePath, std::string& outServerPath)
{
    // 1. 전송할 파일 열기
    CFile file;
    if (!file.Open(strFilePath, CFile::modeRead | CFile::typeBinary)) {
        return false; // 파일 열기 실패
    }
    uint32_t fileSize = (uint32_t)file.GetLength();

    // 2. 서버 연결
    CSocket socket;
    if (!socket.Create()) return false;
    if (!socket.Connect(SERVER_IP, SERVER_PORT)) return false;

    // 3. 파일 업로드용 헤더 전송 (프로토콜 9999)
    PacketHeader hdr;
    hdr.type = (uint8_t)ClientType::OWNER;
    hdr.protocol = htons(9999); // 서버의 PROTOCOL_FILE_UPLOAD
    hdr.len = htonl(fileSize);  // 바디 길이 대신 파일의 전체 크기를 넣음

    if (socket.Send(&hdr, sizeof(PacketHeader)) == SOCKET_ERROR) return false;

    // 4. 파일 데이터 쪼개서 전송 (서버의 FILE_CHUNK_SIZE에 맞춤)
    const int CHUNK_SIZE = 65536; // 64KB
    char buffer[CHUNK_SIZE];
    UINT bytesRead = 0;

    while ((bytesRead = file.Read(buffer, CHUNK_SIZE)) > 0) {
        int sent = socket.Send(buffer, bytesRead);
        if (sent == SOCKET_ERROR) {
            file.Close();
            return false;
        }
    }
    file.Close();

    // 5. 서버로부터 완료 응답(JSON) 수신
    PacketHeader resHdr;
    if (socket.Receive(&resHdr, sizeof(PacketHeader)) < (int)sizeof(PacketHeader)) return false;

    uint32_t resLen = ntohl(resHdr.len);
    if (resLen > 0) {
        std::vector<char> resBuffer(resLen + 1, 0);
        int nTotalRecv = 0;
        while (nTotalRecv < (int)resLen) {
            int nRecv = socket.Receive(resBuffer.data() + nTotalRecv, resLen - nTotalRecv);
            if (nRecv <= 0) break;
            nTotalRecv += nRecv;
        }

        // 서버가 보내준 JSON 파싱하여 경로 추출
        std::string jsonStr(resBuffer.data());
        try {
            json resJson = json::parse(jsonStr);
            if (resJson.value("action", "") == "FILE_UPLOAD_SUCCESS") {
                outServerPath = resJson.value("file_path", ""); // 예: "images/uploaded_123.jpg"
                return true;
            }
        }
        catch (...) { return false; }
    }
    return false;
}