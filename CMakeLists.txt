cmake_minimum_required(VERSION 3.10)
project(DeliveryServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 소스 파일 수집
# GLOB_RECURSE는 하위 디렉토리(Admin, Customer 등)를 모두 포함합니다.
# =========================================================
file(GLOB_RECURSE SERVER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/Common/src/*.cpp" # Basehandler.cpp 포함
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/src/*.cpp"        # Admin, Customer, Owner, Rider, rider_chat 하위 폴더 자동 포함
)

# 실행 파일 생성
add_executable(delivery_server ${SERVER_SOURCES})

# =========================================================
# 2. 헤더 경로 설정
# =========================================================
target_include_directories(delivery_server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/core/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/Common/header" 
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Rider"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Admin"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Customer"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Owner"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/rider_chat"
)

# =========================================================
# 3. 외부 라이브러리 설정 (Threads, MariaDB)
# =========================================================
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)

# =========================================================
# 4. 링크 (Link Libraries)
# =========================================================
# MariaDB 헤더 경로 추가
target_include_directories(delivery_server PRIVATE ${MARIADB_INCLUDE_DIRS})

target_link_libraries(delivery_server PRIVATE
    Threads::Threads
    ${MARIADB_LIBRARIES}
    m
)