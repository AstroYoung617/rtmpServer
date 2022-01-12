//
// Created by hopeworld on 2021/11/29.
//
#ifdef _WIN32
#include <Winsock2.h>
#endif

//seeker
#include "config.h"
#include "other/loggerApi.h"
#include "other/INIReader.h"
#include "nlohmann/json.hpp"
#include <seeker/common.h>
#include "http/HttpServer.h"
#include "http/HttpMsg.h"


template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

std::string HttpServer::log(const Request &req, const Response &res) {
    std::string s;
    s += req.method + " " + req.path + " " + req.get_header_value("REMOTE_ADDR") + " " +
         req.get_header_value("REMOTE_PORT") + "\n";
    s += "================================\n";
    return s;
}

void HttpServer::startPush(){//解析post信息
    (svr).Post(START_PUSH_HTTP_URL,
    [&](const Request& req, Response& res, const ContentReader& content_reader) {
            std::string body;
            content_reader([&](const char* data, size_t data_length) {
                body = process_start_push_request(data, data_length);
                I_LOG(body);
                return true;
                });
            res.set_content(body, "application/json;charset=UTF-8");
        });
}

std::string HttpServer::process_start_push_request(const std::string& data, size_t data_length) {
    // 验证参数合法
    if (state == INIT) {
        state = LOGED;
    } else 
        return HttpMsg::errMsg(START_PUSH_FAIL_CODE, START_PUSH_FAIL);
    // new一个RTMPClient，把需要的参数传给它
    I_LOG("start_push_request body is {}", data.substr(0, data_length));
    auto j = json::parse(data.substr(0, data_length));  //解析参数

    if (j["email"].is_null() || j["password"].is_null() || j["serverName"].is_null() || j["rtmpURL"].is_null()||j["pushType"].is_null()) {
        E_LOG("parse ws json string error");
        return HttpMsg::errMsg(PARSE_ERROR_CODE, PARSE_ERROR);
    }
    else {
        auto email = j["email"];
        auto password = j["password"];
        auto serverName = j["serverName"];
        auto rtmpURL = j["rtmpURL"];
        auto pushType = j["pushType"];
       //if (rc->startPushRtmp() == 1)
       if(pushType != 0 && pushType != 1 && pushType != 2){
           E_LOG("Wrong pushType");
           return HttpMsg::errMsg(START_PUSH_FAIL_CODE, START_PUSH_FAIL);
       }
        //new client 执行登录及连接websocket等操作
       rc = std::make_unique<RTMPClient>(email, password, serverName, rtmpURL,pushType);
       if (rc->loginFlag)
           return HttpMsg::errMsg();
       else{
           state = INIT;
           return HttpMsg::errMsg(LOG_FAIL_CODE, LOG_FAIL);
       }
       //else
       //     return HttpMsg::errMsg(START_PUSH_FAIL_CODE, START_PUSH_FAIL);
    }
}

void HttpServer::joinRoom() {//解析post信息
    (svr).Post(JOIN_ROOM_HTTP_URL,
        [&](const Request& req, Response& res, const ContentReader& content_reader) {
            std::string body;
            content_reader([&](const char* data, size_t data_length) {
                body = process_join_room_request(data, data_length);
                I_LOG(body);
                return true;
                });
            res.set_content(body, "application/json;charset=UTF-8");
        });
}

std::string HttpServer::process_join_room_request(const std::string& data, size_t data_length) {
    I_LOG("join_room_request body is {}", data.substr(0, data_length));
    auto j = json::parse(data.substr(0, data_length));  //解析参数
    if (state == LOGED) {
        state = JOINED;
    }
    else
        return HttpMsg::errMsg(JOIN_ROOM_FAIL_CODE, JOIN_ROOM_FAIL);
    if (j["roomId"].is_null() || j["roomCode"].is_null()) {
        E_LOG("parse ws json string error");
        //return HttpMsg::errMsg(PARSE_ERROR_CODE, PARSE_ERROR);
    }
    else {
        auto roomId = j["roomId"];
        auto roomCode = j["roomCode"];
        int ret = rc->joinRoom(roomId, roomCode);
        if (ret == 1) {
            return HttpMsg::errMsg();
        }
        else if (ret == 0) {
            return HttpMsg::errMsg(CONNECT_RTMPURL_FAIL_CODE, CONNECT_RTMPURL_FAIL);
        }
        else
            return HttpMsg::errMsg(JOIN_ROOM_FAIL_CODE, JOIN_ROOM_FAIL);

    }

}

void HttpServer::createChannel() {
    (svr).Post(CREATE_CHANNEL_HTTP_URL,
        [&](const Request& req, Response& res, const ContentReader& content_reader) {
            std::string body;
            content_reader([&](const char* data, size_t data_length) {
                body = process_create_channel_request(data, data_length);
                I_LOG(body);
                return true;
                });
            res.set_content(body, "application/json;charset=UTF-8");
        });
}

std::string HttpServer::process_create_channel_request(const std::string& data, size_t data_length) {
    I_LOG("create_channel_request body is {}", data.substr(0, data_length));
    auto j = json::parse(data.substr(0, data_length));  //解析参数
    if (state == JOINED) {
        state = PUSHED;
    }
    else
        return HttpMsg::errMsg(START_PUSH_FAIL_CODE, START_PUSH_FAIL);
    if (rc->joinFlag) {
        if (rc->createAudioChannel() == 1 && rc->createVideoChannel() == 1) {
            if (rc->startPushRtmp() == 1)
                return HttpMsg::errMsg();
            else {
                state = JOINED;
                return HttpMsg::errMsg(START_PUSH_FAIL_CODE, START_PUSH_FAIL);
            }
        }
        else {
            state = JOINED;
            return HttpMsg::errMsg(CREATE_CHANNEL_FAIL_CODE, CREATE_CHANNEL_FAIL);
        }
    }
    else {
        state = LOGED;
        return HttpMsg::errMsg(JOIN_ROOM_FAIL_CODE, JOIN_ROOM_FAIL);
    }


}

void HttpServer::leaveRoom() {
    (svr).Post(LEAVE_ROOM_HTTP_URL,
        [&](const Request& req, Response& res, const ContentReader& content_reader) {
            std::string body;
            content_reader([&](const char* data, size_t data_length) {
                body = process_leave_room_request(data, data_length);
                I_LOG(body);
                return true;
                });
            res.set_content(body, "application/json;charset=UTF-8");
        });
}

std::string HttpServer::process_leave_room_request(const std::string& data, size_t data_length) {
    I_LOG("leave_room_request body is {}", data.substr(0, data_length));
    auto j = json::parse(data.substr(0, data_length));  //解析参数
    if (state == PUSHED) {
        state = LEAVED;
    }
    else
        return HttpMsg::errMsg(LEAVE_ROOM_FAIL_CODE, LEAVE_ROOM_FAIL);
    if (j["roomId"].is_null()) {
        E_LOG("parse ws json string error");
        //return HttpMsg::errMsg(PARSE_ERROR_CODE, PARSE_ERROR);
    }
    else {
        auto roomId = j["roomId"];
        I_LOG("post leaveroom");
        rc->stopPush();
        rc->leaveRoom(roomId);
        //rc->~RTMPClient();
        state = LOGED;
        return HttpMsg::errMsg();
    }
}

int HttpServer::init() {
    I_LOG("server is starting......");
    if (!(svr).is_valid()) {
        E_LOG("server has an error...");
        return -1;
    }

    //设置打印日志
    (svr).set_logger([this](const Request &req, const Response &res) {
        std::cout << log(req, res) << std::endl;
    });
    //todo 各种接口的方法
    startPush();
    joinRoom();
    createChannel();
    leaveRoom();
    (svr).listen(ip.c_str(), port);

    return 0;
}