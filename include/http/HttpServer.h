//
// Created by hopeworld on 2021/11/29.
//

#pragma once
#ifdef _WIN32
#define NOMINMAX
#include <Winsock2.h>
#endif
#include <http/httplib.h>
#include "config.h"
#include "seeker/logger.h"
#include <string>
#include <nlohmann/json.hpp>
#include "nlohmann/fifomap.hpp"
#include <seeker/common.h>
#include "business/RTMPClient.h"

using httplib::Server;
using httplib::Request;
using httplib::Response;
using httplib::ContentReader;

#define START_PUSH_HTTP_URL "/rtmp/startPush"
#define JOIN_ROOM_HTTP_URL "/rtmp/joinRoom"
#define CREATE_CHANNEL_HTTP_URL "/rtmp/createChannel"
#define LEAVE_ROOM_HTTP_URL "/rtmp/leaveRoom"

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

enum HttpState {
    INIT = 100,
    LOGED = 101,
    JOINED = 102,
    PUSHED = 103,
    LEAVED = 104
};

class HttpServer {
    std::string ip;
    int port;
    Server svr;
    uint64_t startTime;
    std::unique_ptr<RTMPClient> rc = nullptr;
    HttpState state = INIT;
    std::string log(const Request& req, const Response& res);

    void startPush();

    void joinRoom();

    void createChannel();

    void leaveRoom();

    std::string process_start_push_request(const std::string& data, size_t data_length);

    std::string process_join_room_request(const std::string& data, size_t data_length);

    std::string process_create_channel_request(const std::string& data, size_t data_length);

    std::string process_leave_room_request(const std::string& data, size_t data_length);
public:
    HttpServer(
        const std::string& ip,
        int port) : ip{ ip }, port{ port } {
        startTime = seeker::Time::currentTime();
    };

    int init();
};

