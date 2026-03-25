#include <iostream>
#include <exception>

// 미리 정의해둔 핵심 헤더 파일들 포함
#include "MariaDBManager.h"
#include "ThreadPool.h"
#include "EpollServer.h"

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "  배달의 민족 프로토타입 서버 시작 중...  " << std::endl;
    std::cout << "==========================================" << std::endl;

    try {
        // 1. MariaDB 데이터베이스 연동 (싱글톤 패턴)
        std::cout << "[1] 데이터베이스(MariaDB) 연결 시도 중..." << std::endl;
        
        MariaDBManager& dbManager = MariaDBManager::getInstance();
        bool dbConnected = dbManager.connect(
            "10.10.10.122",   // Host
            "bemin",          // User
            "1234",           // Password
            "bemin_db",       // Database Name
            3306              // Port
        );

        if (!dbConnected) {
            std::cerr << "[ERROR] DB 연결에 실패했습니다. 서버를 종료합니다." << std::endl;
            return -1;
        }

        std::cout << "[1] 데이터베이스 연결 성공! (bemin_db)" << std::endl;

        // 2. 스레드 풀 생성 (워커 스레드 8개 예시)
        std::cout << "[2] 스레드 풀(Thread Pool) 생성 중... (Worker: 8)" << std::endl;
        ThreadPool threadPool(1); // 현재 1개로 돌리는중 
        std::cout << "[2] 스레드 풀 생성 완료!" << std::endl;

        // 3. Epoll 네트워크 서버 초기화 및 시작 (포트 8080, 스레드 풀 주입)
        int serverPort = 8080;
        std::cout << "[3] Epoll 서버를 포트 " << serverPort << " 에서 초기화합니다." << std::endl;
        EpollServer server(serverPort, &threadPool);
        EpollServer::s_instance = &server; // pushDispatch용 싱글턴
        
        std::cout << "[4] 서버 이벤트 루프 가동 시작. 클라이언트 접속 대기 중..." << std::endl;
        
        // 블로킹 호출 (이 안에서 epoll_wait 무한 루프가 돕니다)
        server.start(); 

    } catch (const std::exception& e) {
        // 예기치 못한 치명적 에러 발생 시 잡아내기
        std::cerr << "[FATAL ERROR] 서버 구동 중 예외 발생: " << e.what() << std::endl;
        return -1;
    }

    std::cout << "서버가 안전하게 종료되었습니다." << std::endl;
    return 0;
}