#pragma once
// ================================================================
//  ImageLoader.h  —  선언만 있는 헤더
//
//  [수정] 기존 인라인 구현 제거 → ImageLoader.cpp로 이동
//         (LNK2005 중복 정의 오류 해결)
// ================================================================
#include <gdiplus.h>
#include <string>
#include <vector>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "gdiplus.lib")

class ImageLoader {
public:
    // ── 기존 함수 (선언만) ────────────────────────────────────
    static CString MakeServerPath(const CString& relPath,
                                  LPCWSTR serverIp = L"10.10.10.122");
    static HBITMAP Load(const CString& fullPath);
    static HBITMAP LoadResized(const CString& fullPath, int w, int h);

    // ── 신규: TCP base64 방식 ────────────────────────────────
    static std::vector<uint8_t> DecodeBase64(const std::string& b64);
    static HBITMAP BitmapFromBytes(const std::vector<uint8_t>& data,
                                   int w = 0, int h = 0);
};
