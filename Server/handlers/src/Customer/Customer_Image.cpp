// ================================================================
//  Customer_Image.cpp  — 이미지 TCP 전송 + 동적 이미지 생성
//
//  REQ_GET_IMAGE (217)
//
//  요청: { "image_url": "upload_22_xxx.jpg" }
//        또는 { "image_url": "placeholder_1" }  ← 가게 ID 기반 플레이스홀더
//
//  응답: { "status":2000, "image_url":"...", "data":"<base64 PNG>" }
//
//  동작:
//    1. image_url이 "placeholder_N" 이면 → 가게 ID(N) 기반 컬러 PNG 동적 생성
//    2. 실제 파일명이면 → 디스크에서 읽어 base64 전송
//    3. 파일이 없으면 → 동적 컬러 PNG 생성 (폴백)
// ================================================================
#include "CustomerHandler.h"
#include "Protocol.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <iostream>
#include <cstring>

using json = nlohmann::json;

// ================================================================
//  이미지 저장 기본 디렉터리
//  Session.cpp의 IMAGE_SAVE_DIR과 반드시 동일하게 맞출 것
// ================================================================
static const std::string IMAGE_BASE_DIR = "";  // 빈 문자열 = 서버 실행 디렉터리

// ================================================================
//  base64 인코딩
// ================================================================
static const char B64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64Encode(const std::vector<uint8_t>& d)
{
    std::string out;
    out.reserve(((d.size() + 2) / 3) * 4);
    size_t i = 0;
    while (i + 2 < d.size()) {
        uint32_t v = ((uint32_t)d[i]<<16)|((uint32_t)d[i+1]<<8)|(uint32_t)d[i+2];
        out+=B64[(v>>18)&63]; out+=B64[(v>>12)&63];
        out+=B64[(v>>6)&63];  out+=B64[v&63];
        i+=3;
    }
    if (i+1==d.size()) {
        uint32_t v=(uint32_t)d[i]<<16;
        out+=B64[(v>>18)&63]; out+=B64[(v>>12)&63]; out+='='; out+='=';
    } else if (i+2==d.size()) {
        uint32_t v=((uint32_t)d[i]<<16)|((uint32_t)d[i+1]<<8);
        out+=B64[(v>>18)&63]; out+=B64[(v>>12)&63]; out+=B64[(v>>6)&63]; out+='=';
    }
    return out;
}

// ================================================================
//  동적 컬러 PNG 생성 (외부 라이브러리 없이 순수 C++)
//
//  가게 ID를 시드로 고유 색상 결정 → 60x60 단색 PNG 생성
//  PNG 스펙: IHDR + IDAT(zlib deflate) + IEND
//
//  단순화: zlib 없이 deflate "stored" 블록으로 압축 없이 저장
//  (PNG는 표준상 zlib 필수지만 stored 블록은 유효한 deflate임)
// ================================================================
static void writePngU32BE(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back((v>>24)&0xFF);
    out.push_back((v>>16)&0xFF);
    out.push_back((v>>8)&0xFF);
    out.push_back(v&0xFF);
}

static uint32_t crc32Table[256] = {};
static bool crcInited = false;
static void initCrc32()
{
    if (crcInited) return;
    for (uint32_t i=0;i<256;i++){
        uint32_t c=i;
        for(int j=0;j<8;j++) c=(c&1)?(0xEDB88320^(c>>1)):(c>>1);
        crc32Table[i]=c;
    }
    crcInited=true;
}
static uint32_t crc32(const uint8_t* data, size_t len)
{
    initCrc32();
    uint32_t c=0xFFFFFFFF;
    for(size_t i=0;i<len;i++) c=crc32Table[(c^data[i])&0xFF]^(c>>8);
    return c^0xFFFFFFFF;
}

static void writePngChunk(std::vector<uint8_t>& out,
                          const char type[4],
                          const std::vector<uint8_t>& data)
{
    writePngU32BE(out, (uint32_t)data.size());
    out.push_back(type[0]); out.push_back(type[1]);
    out.push_back(type[2]); out.push_back(type[3]);
    out.insert(out.end(), data.begin(), data.end());

    // CRC: type(4) + data
    std::vector<uint8_t> crcBuf;
    crcBuf.push_back(type[0]); crcBuf.push_back(type[1]);
    crcBuf.push_back(type[2]); crcBuf.push_back(type[3]);
    crcBuf.insert(crcBuf.end(), data.begin(), data.end());
    writePngU32BE(out, crc32(crcBuf.data(), crcBuf.size()));
}

// adler32 (zlib 체크섬)
static uint32_t adler32(const uint8_t* d, size_t len)
{
    uint32_t s1=1, s2=0;
    for(size_t i=0;i<len;i++){
        s1=(s1+d[i])%65521;
        s2=(s2+s1)%65521;
    }
    return (s2<<16)|s1;
}

static std::vector<uint8_t> makeColorPng(int W, int H, uint8_t R, uint8_t G, uint8_t B)
{
    // 필터 바이트(0) + RGB × W 픽셀, H행
    std::vector<uint8_t> raw;
    raw.reserve((size_t)(1 + W*3) * H);
    for(int y=0;y<H;y++){
        raw.push_back(0); // filter type None
        for(int x=0;x<W;x++){
            raw.push_back(R); raw.push_back(G); raw.push_back(B);
        }
    }

    // deflate stored block (압축 없음)
    // zlib header: CMF=0x78, FLG=0x01
    std::vector<uint8_t> zlib;
    zlib.push_back(0x78); zlib.push_back(0x01);

    size_t pos=0, total=raw.size();
    while(pos<total){
        size_t chunk=std::min((size_t)65535, total-pos);
        bool last=(pos+chunk>=total);
        zlib.push_back(last?0x01:0x00); // BFINAL|BTYPE(stored)
        uint16_t len16=(uint16_t)chunk;
        uint16_t nlen16=~len16;
        zlib.push_back(len16&0xFF);  zlib.push_back(len16>>8);
        zlib.push_back(nlen16&0xFF); zlib.push_back(nlen16>>8);
        zlib.insert(zlib.end(), raw.begin()+pos, raw.begin()+pos+chunk);
        pos+=chunk;
    }
    // adler32
    uint32_t a=adler32(raw.data(), raw.size());
    zlib.push_back((a>>24)&0xFF); zlib.push_back((a>>16)&0xFF);
    zlib.push_back((a>>8)&0xFF);  zlib.push_back(a&0xFF);

    // PNG 조립
    std::vector<uint8_t> png;
    // 시그니처
    const uint8_t sig[]={137,80,78,71,13,10,26,10};
    png.insert(png.end(),sig,sig+8);

    // IHDR
    std::vector<uint8_t> ihdr;
    auto pu=[&](uint32_t v){ ihdr.push_back((v>>24)&0xFF);ihdr.push_back((v>>16)&0xFF);
                              ihdr.push_back((v>>8)&0xFF);ihdr.push_back(v&0xFF); };
    pu(W); pu(H);
    ihdr.push_back(8);  // bit depth
    ihdr.push_back(2);  // color type RGB
    ihdr.push_back(0);  // compression
    ihdr.push_back(0);  // filter
    ihdr.push_back(0);  // interlace
    writePngChunk(png,"IHDR",ihdr);

    // IDAT
    writePngChunk(png,"IDAT",zlib);

    // IEND
    writePngChunk(png,"IEND",{});

    return png;
}

// 가게 ID → 고유 색상
static void idToColor(int id, uint8_t& R, uint8_t& G, uint8_t& B)
{
    // 파스텔 계열 색상 팔레트 (10가지)
    static const uint8_t palette[][3] = {
        {255,179,186}, // 핑크
        {255,223,186}, // 피치
        {255,255,186}, // 옐로우
        {186,255,201}, // 민트
        {186,225,255}, // 스카이블루
        {218,186,255}, // 라벤더
        {255,186,255}, // 라일락
        {186,255,255}, // 아쿠아
        {255,214,186}, // 살몬
        {220,220,220}, // 그레이
    };
    int idx = ((id % 10) + 10) % 10;
    R = palette[idx][0];
    G = palette[idx][1];
    B = palette[idx][2];
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

        std::vector<uint8_t> imageData;

        // ── 플레이스홀더 처리 ("placeholder_N") ──────────────
        if (imageUrl.rfind("placeholder_", 0) == 0) {
            int storeId = 0;
            try { storeId = std::stoi(imageUrl.substr(12)); } catch (...) {}
            uint8_t R, G, B;
            idToColor(storeId, R, G, B);
            imageData = makeColorPng(60, 60, R, G, B);
        }
        else {
            // ── 실제 파일 읽기 ────────────────────────────────
            // 보안: 경로 탐색 공격 방지
            if (imageUrl.find("..") != std::string::npos ||
                imageUrl.find('/') != std::string::npos  ||
                imageUrl.find('\\') != std::string::npos) {
                sendError(session, CmdCustomer::REQ_GET_IMAGE,
                          Status::BAD_REQUEST, "유효하지 않은 경로입니다.");
                return;
            }

            std::string filePath = IMAGE_BASE_DIR + imageUrl;
            std::ifstream ifs(filePath, std::ios::binary);

            if (!ifs.is_open()) {
                // 파일 없음 → 폴백: 플레이스홀더 컬러 이미지
                std::cerr << "[Image] 파일 없음, 폴백 사용: " << filePath << "\n";
                imageData = makeColorPng(60, 60, 200, 200, 200); // 회색
            } else {
                constexpr size_t MAX_SIZE = 5 * 1024 * 1024;
                ifs.seekg(0, std::ios::end);
                size_t sz = (size_t)ifs.tellg();
                ifs.seekg(0, std::ios::beg);

                if (sz == 0 || sz > MAX_SIZE) {
                    sendError(session, CmdCustomer::REQ_GET_IMAGE,
                              Status::BAD_REQUEST, "파일 크기 오류");
                    return;
                }
                imageData.resize(sz);
                ifs.read(reinterpret_cast<char*>(imageData.data()), sz);
                ifs.close();
                std::cout << "[Image] 파일 전송: " << imageUrl
                          << " (" << sz << " bytes)\n";
            }
        }

        json res;
        res["status"]    = Status::SUCCESS;
        res["image_url"] = imageUrl;
        res["data"]      = base64Encode(imageData);

        session->sendPacket(static_cast<uint8_t>(m_clientType),
                            CmdCustomer::REQ_GET_IMAGE, res.dump());

    } catch (const std::exception& e) {
        std::cerr << "[Image] 예외: " << e.what() << "\n";
        sendError(session, CmdCustomer::REQ_GET_IMAGE,
                  Status::SERVER_ERROR, "서버 오류");
    }
}
