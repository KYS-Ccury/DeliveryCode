// ================================================================
//  ImageLoader.cpp  —  base64 디코딩 + GDI+ HBITMAP 변환
// ================================================================
#include "pch.h"
#include "ImageLoader.h"
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")

// ================================================================
//  DecodeBase64  — base64 문자열 → 원본 바이트 배열
// ================================================================
std::vector<uint8_t> ImageLoader::DecodeBase64(const std::string& b64)
{
    static const std::string chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    std::vector<uint8_t> out;
    if (b64.empty()) return out;
    out.reserve(b64.size() * 3 / 4);

    int val = 0, bits = -8;
    for (unsigned char c : b64) {
        if (c == '=') break;
        auto pos = chars.find(c);
        if (pos == std::string::npos) continue;
        val = (val << 6) + (int)pos;
        bits += 6;
        if (bits >= 0) {
            out.push_back((uint8_t)((val >> bits) & 0xFF));
            bits -= 8;
        }
    }
    return out;
}

// ================================================================
//  BitmapFromBytes  — 바이트 배열 → HBITMAP
//  IStream을 사용해 GDI+ Bitmap으로 디코딩
//  w, h > 0 이면 해당 크기로 리사이즈
// ================================================================
HBITMAP ImageLoader::BitmapFromBytes(const std::vector<uint8_t>& data, int w, int h)
{
    if (data.empty()) return nullptr;

    // 바이트 배열 → IStream
    HGLOBAL hGlobal = ::GlobalAlloc(GMEM_MOVEABLE, data.size());
    if (!hGlobal) return nullptr;

    void* pMem = ::GlobalLock(hGlobal);
    if (!pMem) { ::GlobalFree(hGlobal); return nullptr; }
    memcpy(pMem, data.data(), data.size());
    ::GlobalUnlock(hGlobal);

    IStream* pStream = nullptr;
    if (FAILED(::CreateStreamOnHGlobal(hGlobal, TRUE, &pStream)) || !pStream) {
        ::GlobalFree(hGlobal);
        return nullptr;
    }

    // IStream → GDI+ Bitmap
    Gdiplus::Bitmap* pBmp = Gdiplus::Bitmap::FromStream(pStream);
    pStream->Release(); // CreateStreamOnHGlobal(TRUE) 이면 pStream->Release()가 hGlobal 해제

    if (!pBmp || pBmp->GetLastStatus() != Gdiplus::Ok) {
        delete pBmp;
        return nullptr;
    }

    HBITMAP hResult = nullptr;

    if (w > 0 && h > 0 &&
        ((int)pBmp->GetWidth() != w || (int)pBmp->GetHeight() != h))
    {
        // 리사이즈
        Gdiplus::Bitmap dst(w, h, PixelFormat32bppRGB);
        Gdiplus::Graphics g(&dst);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.Clear(Gdiplus::Color(255, 255, 255, 255));

        UINT srcW = pBmp->GetWidth();
        UINT srcH = pBmp->GetHeight();
        float scale = min((float)w / srcW, (float)h / srcH);
        int   dw = (int)(srcW * scale);
        int   dh = (int)(srcH * scale);
        int   dx = (w - dw) / 2;
        int   dy = (h - dh) / 2;
        g.DrawImage(pBmp, dx, dy, dw, dh);
        dst.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hResult);
    }
    else {
        pBmp->GetHBITMAP(Gdiplus::Color(255, 255, 255), &hResult);
    }

    delete pBmp;
    return hResult;
}

// ================================================================
//  MakeServerPath  — 하위 호환 유지 (로컬/UNC 접근용)
// ================================================================
CString ImageLoader::MakeServerPath(const CString& relPath, LPCWSTR serverIp)
{
    if (relPath.IsEmpty()) return _T("");

    std::wstring rel = (LPCWSTR)(LPCTSTR)relPath;
    for (auto& c : rel) if (c == L'/') c = L'\\';

    const std::wstring prefix1 = L"\\images\\";
    const std::wstring prefix2 = L"\\images";
    if (rel.size() >= prefix1.size() && rel.substr(0, prefix1.size()) == prefix1)
        rel = rel.substr(prefix1.size());
    else if (rel.size() >= prefix2.size() && rel.substr(0, prefix2.size()) == prefix2)
        rel = rel.substr(prefix2.size());

    while (!rel.empty() && rel[0] == L'\\') rel = rel.substr(1);

    WCHAR result[MAX_PATH] = {};
    result[0] = L'\\'; result[1] = L'\\';
    wcscat_s(result, serverIp);
    wcscat_s(result, L"\\images\\");
    wcscat_s(result, rel.c_str());
    return CString(result);
}

// ================================================================
//  Load / LoadResized  — 하위 호환 (로컬 파일 직접 로드)
// ================================================================
HBITMAP ImageLoader::Load(const CString& fullPath)
{
    if (fullPath.IsEmpty()) return nullptr;
    Gdiplus::Bitmap bmp((LPCWSTR)(LPCTSTR)fullPath);
    if (bmp.GetLastStatus() != Gdiplus::Ok) return nullptr;
    HBITMAP hBmp = nullptr;
    bmp.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBmp);
    return hBmp;
}

HBITMAP ImageLoader::LoadResized(const CString& fullPath, int w, int h)
{
    if (fullPath.IsEmpty()) return nullptr;
    Gdiplus::Bitmap src((LPCWSTR)(LPCTSTR)fullPath);
    if (src.GetLastStatus() != Gdiplus::Ok) return nullptr;

    Gdiplus::Bitmap dst(w, h, PixelFormat32bppRGB);
    Gdiplus::Graphics g(&dst);
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.Clear(Gdiplus::Color(255, 255, 255, 255));

    UINT srcW = src.GetWidth(), srcH = src.GetHeight();
    if (!srcW || !srcH) return nullptr;
    float scale = min((float)w / srcW, (float)h / srcH);
    int dw = (int)(srcW*scale), dh = (int)(srcH*scale);
    g.DrawImage(&src, (w-dw)/2, (h-dh)/2, dw, dh);

    HBITMAP hBmp = nullptr;
    dst.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBmp);
    return hBmp;
}
