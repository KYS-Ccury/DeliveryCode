#pragma once
// ================================================================
//  ImageLoader.h  —  GDI+로 PNG/JPG 이미지를 HBITMAP으로 로드
//
//  사용법:
//    HBITMAP hBmp = ImageLoader::Load(_T("\\\\10.10.10.122\\images\\abc.png"));
//    HBITMAP hResized = ImageLoader::LoadResized(path, 60, 60);
// ================================================================
#include <gdiplus.h>

class ImageLoader {
public:
    // UNC/로컬 경로에서 PNG/JPG/BMP 로드 → HBITMAP 반환 (실패 시 nullptr)
    static HBITMAP Load(const CString& fullPath)
    {
        if (fullPath.IsEmpty()) return nullptr;

        Gdiplus::Bitmap bmp(fullPath);
        if (bmp.GetLastStatus() != Gdiplus::Ok) return nullptr;

        HBITMAP hBmp = nullptr;
        // 흰색 배경으로 합성 (PNG 투명도 처리)
        bmp.GetHBITMAP(Gdiplus::Color(255, 255, 255), &hBmp);
        return hBmp;
    }

    // 로드 + 지정 크기로 리사이즈
    static HBITMAP LoadResized(const CString& fullPath, int w, int h)
    {
        if (fullPath.IsEmpty()) return nullptr;

        Gdiplus::Bitmap src(fullPath);
        if (src.GetLastStatus() != Gdiplus::Ok) return nullptr;

        // 타겟 비트맵 생성
        Gdiplus::Bitmap dst(w, h, PixelFormat32bppARGB);
        Gdiplus::Graphics g(&dst);
        g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
        g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

        // 흰색 배경 채우기
        g.Clear(Gdiplus::Color(255, 255, 255, 255));

        // 비율 유지 리사이즈 (letterbox)
        UINT srcW = src.GetWidth();
        UINT srcH = src.GetHeight();
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

    // 상대경로 → UNC 전체경로 조합
    // relPath: "abc123.png" → fullPath: "\\10.10.10.122\images\abc123.png"
    static CString MakeServerPath(const CString& relPath,
                                  const CString& serverIp = _T("10.10.10.122"))
    {
        if (relPath.IsEmpty()) return _T("");
        CString path = relPath;
        path.Replace(_T('/'), _T('\\'));
        // 이미 UNC 경로면 그대로
        if (path.Left(2) == _T("\\\\")) return path;
        return _T("\\\\") + serverIp + _T("\\images\\") + path;
    }
};
