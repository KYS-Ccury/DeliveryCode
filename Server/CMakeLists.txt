cmake_minimum_required(VERSION 3.10)
project(DeliveryServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 소스 파일 수집 (BaseHandler 소스 경로 추가)
# =========================================================
file(GLOB_RECURSE SERVER_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/src/*.cpp"
    # ★ 추가: BaseHandler.cpp가 포함된 디렉토리
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/Common/src/*.cpp" 
)

# 실행 파일 생성
add_executable(delivery_server ${SERVER_SOURCES})

# =========================================================
# 2. 헤더 경로 설정 (채팅 핸들러 경로 추가)
# =========================================================
target_include_directories(delivery_server PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Common/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/core/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/db/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/network/header"
    
    # BaseHandler.h 경로
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/Common/header" 
    
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Rider"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Admin"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Customer"
    "${CMAKE_CURRENT_SOURCE_DIR}/handlers/header/Owner"
    # ★ 추가: ChatHandler.h 경로
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
target_link_libraries(delivery_server PRIVATE
    Threads::Threads
    ${MARIADB_LIBRARIES}
)

# MariaDB 헤더 경로 추가
target_include_directories(delivery_server PRIVATE ${MARIADB_INCLUDE_DIRS})