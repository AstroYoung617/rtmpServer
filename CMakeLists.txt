cmake_minimum_required(VERSION 3.8)
project("rtmpServer" VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 14)


if(MSVC)
    SET(CMAKE_CXX_FLAGS "/EHsc")  # deal with MSVC Warning C4530, mac
else()
    SET(CMAKE_CXX_FLAGS "/utf-8" "${CMAKE_CXX_FLAGS}")    # for CPP
    SET(CMAKE_C_FLAGS "/utf-8" "${CMAKE_C_FLAGS}")        # for C
endif()
set(CMAKE_CXX_FLAGS "-D__STDC_CONSTANT_MACROS")
# ----------------------------------------
# SET_LOCAL_PATH
# ----------------------------------------
include(./.SET_LOCAL_PATH.cmake)

#ADD_DEFINITIONS(-D__STDC_LIMIT_MACROS -D_WEBSOCKETPP_CPP11_STL_ -DASIO_STANDALONE)
if (UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}  -lpthread -std=c++11 -Wl,--no-as-needed")
endif()

#-------------------------------------
#  compile settings
#-------------------------------------
If (UNIX)
    add_compile_options(-x objective-c++)
    add_compile_options(-g)
    #    add_compile_options(-target arm64-apple-macos11)
    #add_compile_options(-std=c++11)
    #set(CMAKE_EXE_LINKER_FLAGS
    #       "-framework Cocoa -framework AppKit -framework CoreData -framework Foundation -framework CoreVideo -framework VideoToolbox -framework CoreMedia -framework AVFoundation  -framework CoreAudio -framework AudioToolbox")
    #add_definitions(-Wall)
elseif(WIN32)
    add_compile_options(-bigobj)
endif ()


#-------------------------------------
#  src files
#-------------------------------------
list(APPEND RESOURCE_FILES resources/application.ini)

#添加cpp文件，相当于引入include
aux_source_directory(src/media CPPLIST)
aux_source_directory(src/net CPPLIST)
aux_source_directory(src/business CPPLIST)
aux_source_directory(src/media/decoder CPPLIST)
aux_source_directory(src/media/encoder CPPLIST)
aux_source_directory(src/media/other CPPLIST)

if(WIN32)
    add_executable (${PROJECT_NAME}
            ${RESOURCE_FILES}
            ${CPPLIST}
            #tests/mp42rtmp.cpp
            #tests/h264aac2rtmp.cpp
            #tests/h2642rtmp.cpp
            tests/test.cpp
            #tests/rtpSystem.cpp
            #tests/rtpStudy/rtp.cpp
            #tests/rtpStudy/rtpSendaac.cpp
            #tests/rtpStudy/rtpServer2.cpp
            #tests/rtpStudy/rtpClient2.cpp
            #tests/rtpStudy/rtpSendaac.cpp
            #tests/rtpStudy/serverUDP.cpp
            #tests/rtpStudy/rtpSendh264.cpp
            #tests/rtpStudy/rtpRecvh264.cpp
            #tests/rtpStudy/unPackrtph264.cpp
            #tests/rtpStudy/unPackrtpaac.cpp
            #tests/ffmpeg/h264ToYuv.cpp
            #tests/ffmpeg/decoder.cpp
            #tests/ffmpeg/combineTwo.cpp
            #tests/rtmpstudy/h264aac2rtmp.cpp
            #tests/rtpStudy/rtpSendaach264.cpp
            #tests/sometest.cpp
            #tests/testArgv.cpp
            )
elseif(UNIX)
    add_executable (${PROJECT_NAME}
            ${RESOURCE_FILES}
            ${CPPLIST}
            )
endif()


#-------------------------------------
#  ffmpeg lib and include
#-------------------------------------
find_path(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
find_library(AVCODEC_LIBRARY avcodec)

message(STATUS AVCODEC_LIBRARY ${AVCODEC_LIBRARY})
message(STATUS AVCODEC_INCLUDE_DIR ${AVCODEC_INCLUDE_DIR})

find_path(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
find_library(AVFORMAT_LIBRARY avformat)

message(STATUS AVFORMAT_LIBRARY ${AVFORMAT_LIBRARY})
message(STATUS AVFORMAT_INCLUDE_DIR ${AVFORMAT_INCLUDE_DIR})

find_path(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
find_library(AVUTIL_LIBRARY avutil)

message(STATUS AVUTIL_INCLUDE_DIR ${AVUTIL_INCLUDE_DIR})
message(STATUS AVUTIL_LIBRARY ${AVUTIL_LIBRARY})

find_path(AVDEVICE_INCLUDE_DIR libavdevice/avdevice.h)
find_library(AVDEVICE_LIBRARY avdevice)

message(STATUS AVDEVICE_INCLUDE_DIR ${AVDEVICE_INCLUDE_DIR})
message(STATUS AVDEVICE_LIBRARY ${AVDEVICE_LIBRARY})

find_path(AVFILTER_INCLUDE_DIR libavfilter/avfilter.h)
find_library(AVFILTER_LIBRARY avfilter)

message(STATUS AVFILTER_INCLUDE_DIR ${AVFILTER_INCLUDE_DIR})
message(STATUS AVFILTER_LIBRARY ${AVFILTER_LIBRARY})

#find_path(POSTPROC_INCLUDE_DIR libpostproc/postprocess.h)
#find_library(POSTPROC_LIBRARY postproc)

find_path(SWRESAMPLE_INCLUDE_DIR libswresample/swresample.h)
find_library(SWRESAMPLE_LIBRARY swresample)

message(STATUS SWRESAMPLE_INCLUDE_DIR ${SWRESAMPLE_INCLUDE_DIR})
message(STATUS SWRESAMPLE_LIBRARY ${SWRESAMPLE_LIBRARY})

find_path(SWSCALE_INCLUDE_DIR libswscale/swscale.h)
find_library(SWSCALE_LIBRARY swscale)

message(STATUS SWSCALE_INCLUDE_DIR ${SWSCALE_INCLUDE_DIR})
message(STATUS SWSCALE_LIBRARY ${SWSCALE_LIBRARY})

find_path(RTMP_INCLUDE_DIR librtmp/rtmp.h)
find_library(RTMP_LIBRARY rtmp)

message(STATUS RTMP_LIBRARY ${RTMP_LIBRARY})
message(STATUS RTMP_INCLUDE_DIR ${RTMP_INCLUDE_DIR})

#--------------------------------------
# asio lib and include
#--------------------------------------
find_path(ASIO_INCLUDE_DIR asio)

message(STATUS ASIO_INCLUDE_DIR = ${ASIO_INCLUDE_DIR})

#--------------------------------------
# live555 lib and include
#--------------------------------------
find_path(BASICUSAGEENVIRONMENT_INCLUDE_DIR BasicUsageEnvironment.hh)
find_library(BASICUSAGEENVIRONMENT_LIBRARY BasicUsageEnvironment)

message(STATUS BASICUSAGEENVIRONMENT_INCLUDE_DIR = ${BASICUSAGEENVIRONMENT_INCLUDE_DIR})
message(STATUS BASICUSAGEENVIRONMENT_LIBRARY ${BASICUSAGEENVIRONMENT_LIBRARY})

find_path(USAGEENVIRONMENT_INCLUDE_DIR UsageEnvironment.hh)
find_library(USAGEENVIRONMENT_LIBRARY UsageEnvironment)

message(STATUS USAGEENVIRONMENT_INCLUDE_DIR = ${USAGEENVIRONMENT_INCLUDE_DIR})
message(STATUS USAGEENVIRONMENT_LIBRARY ${USAGEENVIRONMENT_LIBRARY})

find_path(LIVEMEDIA_INCLUDE_DIR liveMedia.hh)
find_library(LIVEMEDIA_LIBRARY liveMedia)

message(STATUS LIVEMEDIA_INCLUDE_DIR = ${LIVEMEDIA_INCLUDE_DIR})
message(STATUS LIVEMEDIA_LIBRARY ${LIVEMEDIA_LIBRARY})


find_path(GROUPSOCK_INCLUDE_DIR groupsock.hh)
find_library(GROUPSOCK_LIBRARY groupsock)

message(STATUS GROUPSOCK_INCLUDE_DIR = ${GROUPSOCK_INCLUDE_DIR})
message(STATUS GROUPSOCK_LIBRARY ${GROUPSOCK_LIBRARY})

# 配置链接库
if(WIN32)
    target_include_directories( ${PROJECT_NAME}
            PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${ASIO_INCLUDE_DIR}
            ${AVCODEC_INCLUDE_DIR}
            ${AVFORMAT_INCLUDE_DIR}
            ${AVUTIL_INCLUDE_DIR}
            ${AVDEVICE_INCLUDE_DIR}
            ${AVFILTER_INCLUDE_DIR}
            ${SWRESAMPLE_INCLUDE_DIR}
            ${SWSCALE_INCLUDE_DIR}
            ${BASICUSAGEENVIRONMENT_INCLUDE_DIR}
            ${LIVEMEDIA_INCLUDE_DIR}
            ${USAGEENVIRONMENT_INCLUDE_DIR}
            ${GROUPSOCK_INCLUDE_DIR}
            )
    target_link_libraries( ${PROJECT_NAME}
            PRIVATE
            ${AVCODEC_LIBRARY}
            ${AVFORMAT_LIBRARY}
            ${AVUTIL_LIBRARY}
            ${AVDEVICE_LIBRARY}
            ${AVFILTER_LIBRARY}
            ${SWRESAMPLE_LIBRARY}
            ${SWSCALE_LIBRARY}
            ${BASICUSAGEENVIRONMENT_LIBRARY}
            ${LIVEMEDIA_LIBRARY}
            ${USAGEENVIRONMENT_LIBRARY}
            ${GROUPSOCK_LIBRARY}
        )

elseif(UNIX)
    target_include_directories( ${PROJECT_NAME}
        PRIVATE
            ${PROJECT_SOURCE_DIR}/include
            ${ASIO_INCLUDE_DIR}
        ${AVCODEC_INCLUDE_DIR}
        ${AVFORMAT_INCLUDE_DIR}
        ${AVUTIL_INCLUDE_DIR}
        ${AVDEVICE_INCLUDE_DIR}
        ${AVFILTER_INCLUDE_DIR}
        ${SWRESAMPLE_INCLUDE_DIR}
        ${SWSCALE_INCLUDE_DIR}
            )
    target_link_libraries( ${PROJECT_NAME}
        PRIVATE
        ${AVCODEC_LIBRARY}
        ${AVFORMAT_LIBRARY}
        ${AVUTIL_LIBRARY}
        ${AVDEVICE_LIBRARY}
        ${AVFILTER_LIBRARY}
        ${SWRESAMPLE_LIBRARY}
        ${SWSCALE_LIBRARY}
            )
endif()

add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        "${PROJECT_SOURCE_DIR}/resources/application.ini"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/"
        )