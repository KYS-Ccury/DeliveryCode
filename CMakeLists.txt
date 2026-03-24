cmake_minimum_required(VERSION 3.10)
project(DeliveryServer VERSION 1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================
# 1. 소스 파일 수집
# =========================================================
file(GLOB_RECURSE SERVER_SOURCES
    "Server/main.cpp"
    "Server/core/src/*.cpp"
    "Server/db/src/*.cpp"
    "Server/handlers/src/*.cpp"
    "Server/network/src/*.cpp"
)

# ── 중복 구현 파일 제외 ──────────────────────────────────
# _final.cpp 가 최종 버전 → 구버전 전부 제외
list(REMOVE_ITEM SERVER_SOURCES
    # Customer: 원본 + _DB 제외, _final 유지
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/handlers/src/Customer/CustomerHandler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/handlers/src/Customer/CustomerHandler_DB.cpp"
    # Admin: 원본 + _DB 제외, _final 유지
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/handlers/src/Admin/AdminHandler.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/handlers/src/Admin/AdminHandler_DB.cpp"
    # Owner: 원본 제외, _DB 유지
    "${CMAKE_CURRENT_SOURCE_DIR}/Server/handlers/src/Owner/OwnerHandler.cpp"
)

file(GLOB_RECURSE COMMON_SOURCES "Common/src/*.cpp")

add_executable(delivery_server ${SERVER_SOURCES} ${COMMON_SOURCES})

# =========================================================
# 2. 헤더 경로
# =========================================================
target_include_directories(delivery_server PRIVATE
    "Common"
    "Common/header"
    "Server/core/header"
    "Server/db/header"
    "Server/handlers/header"
    "Server/network/header"
)

# =========================================================
# 3. 외부 패키지
# =========================================================
find_package(Threads REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB REQUIRED libmariadb)
find_package(nlohmann_json REQUIRED)

# =========================================================
# 4. 링크
# =========================================================
target_link_libraries(delivery_server PRIVATE
    Threads::Threads
    ${MARIADB_LIBRARIES}
    nlohmann_json::nlohmann_json
)

target_include_directories(delivery_server PRIVATE ${MARIADB_INCLUDE_DIRS})
target_link_directories(delivery_server PRIVATE ${MARIADB_LIBRARY_DIRS})