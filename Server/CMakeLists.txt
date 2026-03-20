# CMake 최소 버전 지정
cmake_minimum_required(VERSION 3.10)

# 프로젝트 이름 설정
project(DeliveryServer VERSION 1.0)

# C++17 표준 사용
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 소스 파일(.cpp) 모으기
# =========================================================

# 1-1. Server 내부의 src 폴더 하위 모든 cpp 파일
file(GLOB_RECURSE SERVER_SOURCES "src/*.cpp")

# 1-2. Common 폴더 내부의 src 하위 모든 cpp 파일 (상위 디렉토리 참조)
file(GLOB_RECURSE COMMON_SOURCES "../Common/src/*.cpp")

# 실행 파일 생성: 상위 폴더의 main.cpp와 수집된 모든 소스 포함
add_executable(delivery_server 
    "../main.cpp" 
    ${SERVER_SOURCES} 
    ${COMMON_SOURCES}
)

# =========================================================
# 2. 헤더 파일(.h) 경로 지정 (인클루드 디렉토리)
# =========================================================
target_include_directories(delivery_server PRIVATE
    "../Common"                # json.hpp가 있는 위치
    "../Common/header"         # Packet.h, Types.h 위치
    "src/core/header"
    "src/db/header"
    "src/handlers/header"
    "src/network/header"
)

# =========================================================
# 3. 외부 패키지 및 라이브러리 링크
# =========================================================

# 3-1. 멀티스레드 지원
find_package(Threads REQUIRED)

# 3-2. MariaDB C 커넥터 (libmariadb)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)
target_include_directories(delivery_server PRIVATE ${MARIADB_INCLUDE_DIRS})

# 3-3. JSON 라이브러리 (nlohmann_json)
find_package(nlohmann_json REQUIRED)

# =========================================================
# 4. 최종 라이브러리 병합
# =========================================================
target_link_libraries(delivery_server PRIVATE 
    Threads::Threads 
    ${MARIADB_LIBRARIES}
    nlohmann_json::nlohmann_json
)

# MariaDB 라이브러리 디렉토리 설정 (필요 시)
if(MARIADB_LIBRARY_DIRS)
    target_link_directories(delivery_server PRIVATE ${MARIADB_LIBRARY_DIRS})
endif()