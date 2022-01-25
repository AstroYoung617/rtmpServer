/**
 * Copyright (c) 2020, SeekLoud Team.
* Date: 2020/6/1
	* Main Developer: xsr
	* Developer:
* Date: 2021/3/25
	* Main Developer: lrj
	* Developer: gy lyj
* Date: 2020/6/1
	* Main Developer: lrj
	* Developer: rys
 * Description: api
 * Refer:
 */

#pragma once
extern "C" {
#include "libswscale/swscale.h"
};
#include <media/common.h>
#include <list>
#include <mutex>
#include <map>
#include <memory>

#define MAX_ENCODE_WIDTH 1280
#define MAX_ENCODE_HEIGHT 960

class Parser; //先声明，定义在内部

class Decoder{
private:
	//编码相关
	const AVCodec* codec = nullptr;
	AVFormatContext* formatCtx = nullptr;
	AVCodecParserContext* parser = NULL;
	AVCodecContext* c = NULL;
	AVPacket* pkt = NULL;
	int inputWidth = 640;
	int inputHeight = 480;
	int outputWidth = 640;
	int outputHeight = 480;
	uint8_t* outBuff = nullptr;
	uint8_t* h264Data = nullptr;

	SwsContext* sws_ctx = nullptr;
	AVPixelFormat dstPixFmt = AV_PIX_FMT_NONE;

	std::list<AVFrame*> playList{};
	std::shared_ptr<Parser> decodeParser = nullptr;

	//locker
	std::mutex sendMtx{};
	//std::condition_variable sendCv{};
	bool decodeReady = false;

	void decode(AVPacket* pkt, uint64_t ts);
	void changeFmtAndSave(AVFrame* frameYUV);
	void initSws();
public:
	Decoder();
	~Decoder();

	virtual void init();
	virtual void push(uint8_t* data, int len, int64_t timestamp);
	virtual int poll(AVFrame* outData);
	virtual void setPixFmt(AVPixelFormat fmt);
	virtual void stop();
	virtual void scale(AVFrame* frame, int width, int height);
	void setAVFormatContext(AVFormatContext* formatContext);
};

