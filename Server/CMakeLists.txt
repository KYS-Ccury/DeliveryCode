cmake_minimum_required(VERSION 3.10)
project(DeliveryServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 소스 파일 수집 (GLOB_RECURSE로 각 폴더별 자동 수집)
# =========================================================
set(SOURCES_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

file(GLOB_RECURSE SOURCES_CORE            "${CMAKE_CURRENT_SOURCE_DIR}/core/src/*.cpp")
file(GLOB_RECURSE SOURCES_DB              "${CMAKE_CURRENT_SOURCE_DIR}/db/src/*.cpp")
file(GLOB_RECURSE SOURCES_NETWORK         "${CMAKE_CURRENT_SOURCE_DIR}/network/src/*.cpp")
file(GLOB_RECURSE SOURCES_HANDLERS_COMMON "${CMAKE_CURRENT_SOURCE_DIR}/handlers/Common/src/*.cpp")
file(GLOB_RECURSE SOURCES_HANDLERS        "${CMAKE_CURRENT_SOURCE_DIR}/handlers/src/*.cpp")

set(SERVER_SOURCES
    ${SOURCES_MAIN}
    ${SOURCES_CORE}
    ${SOURCES_DB}
    ${SOURCES_NETWORK}
    ${SOURCES_HANDLERS_COMMON}
    ${SOURCES_HANDLERS}
)

# 실행 파일 생성
add_executable(delivery_server ${SERVER_SOURCES})

# =========================================================
# 2. 외부 라이브러리 설정 (MariaDB, Threads)
# =========================================================
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)

# =========================================================
# 3. 헤더 경로 설정 (디렉토리 구조와 1:1 매칭)
# =========================================================
target_include_directories(delivery_server PRIVATE
    ${MARIADB_INCLUDE_DIRS}
    
    # Core & Network
    "${CMAKE_CURRENT_SOURCE_DIR}/core/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/header"
    
    # DB 계층
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header/Admin"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header/Customer"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header/Owner"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header/Rider"
    
    # Handler 계층
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/Common/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Admin"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Customer"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Owner"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Rider"
)

# =========================================================
# 4. 링크 (Link Libraries)
# =========================================================
target_link_libraries(delivery_server PRIVATE
    Threads::Threads
    ${MARIADB_LIBRARIES}
    m
)