// #include "CustomerHandler.h"
// #include "MariaDBManager.h"
// #include "Protocol.h"

// using json = nlohmann::json;

// // 회원가입 직후 포인트 테이블 생성
// void CustomerHandler::onSignup(Session* session, const json& reqBody) {
//     auto& db = MariaDBManager::getInstance();
//     std::string id = reqBody.value("id", "");
    
//     auto rows = db.executeQuery("SELECT user_id FROM users WHERE login_id='" + escapeStr(id) + "' LIMIT 1");
//     if (!rows.empty()) {
//         int uid = std::stoi(rows[0].at("user_id"));
//         db.executeUpdate("INSERT INTO customer_profiles (user_id, point) VALUES (" + std::to_string(uid) + ", 0)");
//     }
//     json res; res["status"] = Status::SUCCESS;
//     session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCommon::REQ_SIGNUP, res.dump());
// }

// // 로그인 성공 직후 고객 프로필 전송
// void CustomerHandler::onLoginSuccess(Session* session, int userId, const json& reqBody) {
//     auto& db = MariaDBManager::getInstance();
//     auto rows = db.executeQuery(
//         "SELECT u.name, u.address, cp.point FROM users u "
//         "LEFT JOIN customer_profiles cp ON cp.user_id = u.user_id "
//         "WHERE u.user_id=" + std::to_string(userId));

//     if (!rows.empty()) {
//         json res;
//         res["status"]  = Status::SUCCESS;
//         res["token"]   = "ctkn_" + std::to_string(userId);
//         res["user_id"] = userId;
//         res["name"]    = rows[0].at("name");
//         res["address"] = rows[0].count("address") ? rows[0].at("address") : "";
//         res["point"]   = rows[0].count("point") && !rows[0].at("point").empty() ? std::stoi(rows[0].at("point")) : 0;
//         session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCommon::REQ_LOGIN, res.dump());
//     }
// }

// // 로그아웃
// void CustomerHandler::onLogout(Session* session, int userId) {
//     json res; res["status"] = Status::SUCCESS;
//     session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCommon::REQ_LOGOUT, res.dump());
// }

// // 프로필 조회 및 수정
// void CustomerHandler::onGetProfile(Session* session, int userId, const json& req) {
//     try {
//         auto& db = MariaDBManager::getInstance();
        
//         // 주소 변경
//         if (req.contains("address")) {
//             db.executeUpdate("UPDATE users SET address='" + escapeStr(req.value("address","")) + "' WHERE user_id=" + std::to_string(userId));
//         }

//         // 비밀번호 변경
//         if (req.contains("new_pw")) {
//             std::string old_pw = req.value("old_pw","");
//             auto chk = db.executeQuery("SELECT user_id FROM users WHERE user_id=" + std::to_string(userId) + " AND password='" + escapeStr(old_pw) + "'");
//             if (chk.empty()) { sendError(session, CmdCommon::REQ_GET_PROFILE, Status::UNAUTHORIZED, "현재 비밀번호 불일치"); return; }
//             db.executeUpdate("UPDATE users SET password='" + escapeStr(req.value("new_pw","")) + "' WHERE user_id=" + std::to_string(userId));
//         }

//         auto rows = db.executeQuery(
//             "SELECT u.name, u.address, u.phone, cp.point "
//             "FROM users u LEFT JOIN customer_profiles cp ON cp.user_id=u.user_id "
//             "WHERE u.user_id=" + std::to_string(userId));
            
//         if (rows.empty()) { sendError(session, CmdCommon::REQ_GET_PROFILE, Status::NOT_FOUND, "사용자 없음"); return; }

//         auto& r = rows[0];
//         json res;
//         res["status"]  = Status::SUCCESS;
//         res["name"]    = r.at("name");
//         res["address"] = r.count("address") ? r.at("address") : "";
//         res["phone"]   = r.at("phone");
//         res["point"]   = r.count("point") && !r.at("point").empty() ? std::stoi(r.at("point")) : 0;

//         session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCommon::REQ_GET_PROFILE, res.dump());

//     } catch (const std::exception& e) {
//         sendError(session, CmdCommon::REQ_GET_PROFILE, Status::SERVER_ERROR, e.what());
//     }
// }

// // 회원 탈퇴
// void CustomerHandler::handleWithdraw(Session* session, const std::string& body) {
//     int uid = getUserIdByFd(session->getFd());
//     if (!uid) { sendError(session, CmdCommon::REQ_WITHDRAW, Status::UNAUTHORIZED, "로그인 필요"); return; }

//     MariaDBManager::getInstance().executeUpdate("UPDATE users SET status='DELETED' WHERE user_id=" + std::to_string(uid));
//     unregisterSession(session->getFd()); // 세션 장부에서도 삭제

//     json res; res["status"] = Status::SUCCESS;
//     session->sendPacket(static_cast<uint8_t>(m_clientType), CmdCommon::REQ_WITHDRAW, res.dump());
// }