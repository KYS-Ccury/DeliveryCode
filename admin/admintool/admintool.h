/**
 * admintool.h
 * ============================================================
 * ★ 수정사항: CClientSocket m_socket 멤버 추가
 *   → LoginDlg / 모든 Page에서 GetSocket()으로 접근
 * ============================================================
 */

#pragma once

#ifndef __AFXWIN_H__
#error "pch.h를 먼저 포함해야 합니다."
#endif

#include "resource.h"
#include "ClientSocket.h"   // ★ 추가

class CAdminToolApp : public CWinAppEx
{
public:
    CAdminToolApp();
    virtual BOOL InitInstance();

    // ★ 추가: 소켓 객체 접근자 (어디서든 사용 가능)
    CClientSocket& GetSocket() { return m_socket; }

    DECLARE_MESSAGE_MAP()

private:
    CClientSocket m_socket;   // ★ 추가: TCP 소켓 (프로그램 전체 공유)
};

// 기존 빈 클래스 (사용되지 않으나 호환성 유지)
class admintool
{
};