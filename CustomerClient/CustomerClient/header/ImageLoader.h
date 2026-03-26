#pragma once
#include <gdiplus.h>
#include <string>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

// ================================================================
//  ImageLoader.h  —  GDI+로 PNG/JPG 이미지를 HBITMAP으로 로드
// ================================================================
class ImageLoader {
public:

    // DB 경로("/images/menus/uuid.png") → UNC 전체 경로 조합
    static CString MakeServerPath(const CString& relPath,
                                  LPCWSTR serverIp = L"10.10.10.122")
    {
        if (relPath.IsEmpty()) return _T("");

        // relPath를 wstring으로 변환
        std::wstring rel = (LPCWSTR)(LPCTSTR)relPath;

        // '/' → '\' 교체
        for (auto& c : rel) if (c == L'/') c = L'\\';

        // 앞의 '\images\' 제거 (Samba 공유명 중복 방지)
        // rel = "\images\menus\uuid.png" → "menus\uuid.png"
        const std::wstring prefix1 = L"\\images\\";
        const std::wstring prefix2 = L"\\images";
        if (rel.size() >= prefix1.size() &&
            rel.substr(0, prefix1.size()) == prefix1)
            rel = rel.substr(prefix1.size());
        else if (rel.size() >= prefix2.size() &&
            rel.substr(0, prefix2.size()) == prefix2)
            rel = rel.substr(prefix2.size());

        // 남은 앞 백슬래시 제거
        while (!rel.empty() && rel[0] == L'\\')
            rel = rel.substr(1);

        // UNC 경로: \\server\images\subpath
        // WCHAR 배열로 직접 구성 (리터럴 이스케이프 문제 완전 회피)
        WCHAR result[MAX_PATH] = {};
        result[0] = L'\\';
        result[1] = L'\\';
        wcscat_s(result, serverIp);       // "10.10.10.122"
        wcscat_s(result, L"\\");
        wcscat_s(result, L"images");      // Samba 공유명
        wcscat_s(result, L"\\");
        wcscat_s(result, rel.c_str());    // "menus\uuid.png"

        return CString(result);
    }

    // PNG/JPG/BMP 로드 → HBITMAP 반환
    static HBITMAP Load(const CString& fullPath)
    {
        if (fullPath.IsEmpty()) return nullptr;
        Gdiplus::Bitmap bmp((LPCWSTR)(LPCTSTR)fullPath);
        if (bmp.GetLastStatus() != Gdiplus::Ok) return nullptr;
        HBITMAP hBmp = nullptr;
        bmp.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBmp);
        return hBmp;
    }

    // 로드 + 지정 크기로 리사이즈
    static HBITMAP LoadResized(const CString& fullPath, int w, int h)
    {
        if (fullPath.IsEmpty()) return nullptr;

        Gdiplus::Bitmap src((LPCWSTR)(LPCTSTR)fullPath);
        if (src.GetLastStatus() != Gdiplus::Ok) return nullptr;

        // 타겟 비트맵 생성
        Gdiplus::Bitmap dst(w, h, PixelFormat32bppRGB);
        Gdiplus::Graphics g(&dst);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
        g.Clear(Gdiplus::Color(255, 255, 255, 255));

        // 비율 유지 리사이즈
        UINT srcW = src.GetWidth();
        UINT srcH = src.GetHeight();
        if (srcW == 0 || srcH == 0) return nullptr;

        float scaleX = (float)w / srcW;
        float scaleY = (float)h / srcH;
        float scale  = min(scaleX, scaleY);
        int   dw = (int)(srcW * scale);
        int   dh = (int)(srcH * scale);
        int   dx = (w - dw) / 2;
        int   dy = (h - dh) / 2;

        g.DrawImage(&src, dx, dy, dw, dh);

        HBITMAP hBmp = nullptr;
        dst.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBmp);
        return hBmp;
    }
};
