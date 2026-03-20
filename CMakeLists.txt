cmake_minimum_required(VERSION 3.10)
project(DeliveryServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 소스 파일(.cpp) 수집 (정확한 경로 지정)
# =========================================================

# Server 내부의 모든 cpp (main.cpp 포함)
file(GLOB_RECURSE SERVER_SOURCES 
    "Server/main.cpp"
    "Server/core/src/*.cpp"
    "Server/db/src/*.cpp"
    "Server/handlers/src/*.cpp"
    "Server/network/src/*.cpp"
)

# Common 내부의 모든 cpp
file(GLOB_RECURSE COMMON_SOURCES "Common/src/*.cpp")

# 실행 파일 생성
add_executable(delivery_server ${SERVER_SOURCES} ${COMMON_SOURCES})

# =========================================================
# 2. 헤더 파일(.h) 경로 지정 (에러의 핵심 해결책)
# =========================================================
target_include_directories(delivery_server PRIVATE
    "Common"                 # json.hpp 위치
    "Common/header"          # Packet.h, Types.h 위치
    "Server/core/header"     # Dispatcher.h 등
    "Server/db/header"       # MariaDBManager.h (이게 누락되어 에러 났던 것)
    "Server/handlers/header" # AdminHandler.h 등
    "Server/network/header"  # Session.h, EpollServer.h 등
)

# =========================================================
# 3. 외부 패키지 설정
# =========================================================

# 3-1. 멀티스레드
find_package(Threads REQUIRED)

# 3-2. MariaDB
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)

# 3-3. JSON
find_package(nlohmann_json REQUIRED)

# =========================================================
# 4. 링크 및 추가 설정
# =========================================================
target_link_libraries(delivery_server PRIVATE 
    Threads::Threads 
    ${MARIADB_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# MariaDB 헤더 경로 추가
target_include_directories(delivery_server PRIVATE ${MARIADB_INCLUDE_DIRS})

# CMakeLists.txt 하단에 추가 권장
target_link_directories(delivery_server PRIVATE ${MARIADB_LIBRARY_DIRS})