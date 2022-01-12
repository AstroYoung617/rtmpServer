//
// Created by hopeworld on 2021/11/29.
//

#ifndef GAMBIT_HTTPMSG_H
#define GAMBIT_HTTPMSG_H

#include "nlohmann/json.hpp"
#include "nlohmann/fifomap.hpp"
//using json = nlohmann::json ;

template<class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

#define PARSE_ERROR "parse json string error"
#define PARSE_ERROR_CODE 10001

#define REMOVE_AUDIO4MCU_FAIL "remove audio fail"
#define REMOVE_AUDIO4MCU_FAIL_CODE 10002

#define ADD_AUDIO4MCU_FAIL "add audio fail"
#define ADD_AUDIO4MCU_FAIL_CODE 10003

#define CLOSE_ROOM_FAIL "room is not exist"
#define CLOSE_ROOM_FAIL_CODE 10004

#define NEW_ROOM_FAIL "room already exists"
#define NEW_ROOM_FAIL_CODE 10005

#define ADD_VIDEO4MCU_FAIL "add video fail"
#define ADD_VIDEO4MCU_FAIL_CODE 10006

#define REMOVE_VIDEO4MCU_FAIL "remove video fail"
#define REMOVE_VIDEO4MCU_FAIL_CODE 10007

#define CHANGE_LAYOUT_FAIL "change layout fail"
#define CHANGE_LAYOUT_FAIL_CODE 10008

#define NEW_CHANNEL_FAIL "create channel fail"
#define NEW_CHANNEL_FAIL_CODE 10009

#define CLOSE_CHANNEL_FAIL "close channel fail"
#define CLOSE_CHANNEL_FAIL_CODE 10010

#define INVALID_PARAMETER "invalid parameter input"
#define INVALID_PARAMETER_CODE 10011

#define SET_ROOM_PARAMETER_FAIL "set room parameter fail"
#define SET_ROOM_PARAMETER_FAIL_CODE 10012

#define SET_AUDIO_FOR_USER_FAIL "set audio for user fail"
#define SET_AUDIO_FOR_USER_FAIL_CODE 10013

#define START_PUSH_FAIL "start push fail"
#define START_PUSH_FAIL_CODE 10014

#define JOIN_ROOM_FAIL "join room fail"
#define JOIN_ROOM_FAIL_CODE 10015

#define CREATE_CHANNEL_FAIL "create channel fail"
#define CREATE_CHANNEL_FAIL_CODE 10016

#define LEAVE_ROOM_FAIL "leave room fail"
#define LEAVE_ROOM_FAIL_CODE 10017

#define CONNECT_RTMPURL_FAIL "open rtmpURL fail, please check your rtmpURL and try again"
#define CONNECT_RTMPURL_FAIL_CODE 10018

#define LOG_FAIL "log fail, please check your information and try again"
#define LOG_FAIL_CODE 10018




class HttpMsg {

public:
    static std::string errMsg(int errCode = 0, std::string msg = "ok") {
        json j;
        j["errCode"] = errCode;
        j["msg"] = msg;

        return j.dump();
    }


};

#endif //GAMBIT_HTTPMSG_H
