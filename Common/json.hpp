#include "json.hpp" // json 라이브러리 포함

using json = nlohmann::json; // json 별칭

void test() {
    json j;
    j["id"] = "user1"; // key-value 저장
    j["pw"] = "1234";

    std::string str = j.dump(); // 문자열 변환
    std::cout << str << std::endl;
}