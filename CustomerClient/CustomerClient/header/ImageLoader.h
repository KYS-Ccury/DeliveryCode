#pragma once
// ================================================================
//  ImageLoader.h  —  GDI+로 이미지를 HBITMAP으로 변환
//
//  [수정] Samba UNC 경로 직접 접근 → TCP base64 수신 방식으로 교체
//
//  사용 흐름:
//    1. MainHomeDlg가 가게 목록 수신 후
//       ImageLoader::RequestAsync() 로 각 이미지 비동기 요청
//    2. 서버가 217 응답으로 base64 데이터 전송
//    3. OnImageResponse() 콜백에서 DecodeBase64() → BitmapFromBytes()
//       → ImageList에 추가 → ListView 갱신
//
//  기존 MakeServerPath / Load / LoadResized 는 하위 호환을 위해 유지
//  (오프라인 테스트 또는 로컬 파일 표시용)
// ================================================================
#include <gdiplus.h>
#include <string>
#include <vector>
#include <functional>
#pragma comment(lib, "gdiplus.lib")

class ImageLoader {
public:
    // ── TCP base64 방식 (신규) ────────────────────────────────

    // base64 문자열 → 바이트 배열
    static std::vector<uint8_t> DecodeBase64(const std::string& b64);

    // 바이트 배열 → HBITMAP (GDI+ Bitmap via IStream)
    static HBITMAP BitmapFromBytes(const std::vector<uint8_t>& data, int w = 0, int h = 0);

    // ── 로컬/UNC 경로 방식 (기존 — 하위 호환 유지) ──────────

    // DB 경로("/images/menus/uuid.png") → UNC 전체 경로 조합
    static CString MakeServerPath(const CString& relPath,
                                  LPCWSTR serverIp = L"10.10.10.122");

    // PNG/JPG/BMP 로컬 로드 → HBITMAP
    static HBITMAP Load(const CString& fullPath);

    // 로드 + 지정 크기로 리사이즈
    static HBITMAP LoadResized(const CString& fullPath, int w, int h);

private:
    // HBITMAP 리사이즈 헬퍼
    static HBITMAP ResizeBitmap(HBITMAP src, int w, int h);
};
