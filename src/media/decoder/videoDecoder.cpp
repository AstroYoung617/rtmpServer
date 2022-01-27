#include "media/decoder/VideoDecoder.h"

#define NONPARSER 0

#if _WIN32
#define ALIGN 1
#else
#define ALIGN 1
#endif

//copy from kankan
VdDecoder::VdDecoder() {
	// 使用 init
}

VdDecoder::~VdDecoder() {
	//stop();
}

void VdDecoder::init() {
	if (!decodeParser) {
		decodeParser = std::make_shared<Parser>();
	}
	codec = avcodec_find_decoder(AV_CODEC_ID_H264);
	//codec = avcodec_find_decoder(AV_CODEC_ID_VP8);
	if (!codec) {
		throw std::runtime_error("Codec not found");
	}

	pkt = av_packet_alloc();
	if (!pkt)
		throw std::runtime_error("pkt empty");

	c = avcodec_alloc_context3(codec);

	if (formatCtx) {
		I_LOG("AVFormatContext exist, init AVFormatContext");
		if (avcodec_parameters_to_context(c, formatCtx->streams[0]->codecpar) != 0) {
			throw std::runtime_error("video Could not copy codec context");
		}
		I_LOG("codec_id: {}", formatCtx->streams[0]->codecpar->codec_id);
		I_LOG("bit_rate: {}", formatCtx->streams[0]->codecpar->bit_rate);
		I_LOG("format: {}", formatCtx->streams[0]->codecpar->format);
		I_LOG("codec_tag: {}", formatCtx->streams[0]->codecpar->codec_tag);
		I_LOG("extradata_size: {}", formatCtx->streams[0]->codecpar->extradata_size);
		I_LOG("profile: {}", formatCtx->streams[0]->codecpar->profile);
		I_LOG("level: {}", formatCtx->streams[0]->codecpar->level);
		I_LOG("field_order: {}", formatCtx->streams[0]->codecpar->field_order);
		I_LOG("color_range: {}", formatCtx->streams[0]->codecpar->color_range);
		I_LOG("color_primaries: {}", formatCtx->streams[0]->codecpar->color_primaries);
		I_LOG("color_trc: {}", formatCtx->streams[0]->codecpar->color_trc);
		I_LOG("color_space: {}", formatCtx->streams[0]->codecpar->color_space);
		I_LOG("chroma_location: {}", formatCtx->streams[0]->codecpar->chroma_location);

		if (avcodec_open2(c, codec, nullptr) < 0) {
			throw std::runtime_error("video Could not open codec");
		}
	}
	else {

		c->pix_fmt = AV_PIX_FMT_YUV420P; // todo set

		c->color_range = AVCOL_RANGE_MPEG;

		c->codec_id = codec->id;
		AVDictionary* dic = nullptr;

		if (avcodec_open2(c, codec, &dic) < 0) {
			throw std::runtime_error("video Could not open codec");
		}
	}

	if (!c) {
		throw std::runtime_error("Could not allocate video codec context");
	}

	parser = av_parser_init(codec->id);
	if (!parser) {
		throw std::runtime_error("parser not found");
	}

	initSws();

	decodeReady = true;
}

void VdDecoder::push(uint8_t* data, int len, int64_t timestamp) {
	std::unique_lock<std::mutex> locker(sendMtx);
	if (!decodeReady) {
		E_LOG("decode not init");
		return;
	}
#if NONPARSER
	av_packet_unref(pkt);
	while (len > 0) {
		int ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, data,
			len, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			throw std::runtime_error("get parsed data error");
		}
		data += ret;
		len -= ret;
		if (pkt->size) {
			decode(pkt, timestamp);
		}
	}
#else 
	h264Data = new uint8_t[1024 * 1024];

	// 获取的数据先存放起来，拼接获取完整的数据
	decodeParser->inputPayLoad(data, len, timestamp);
	uint64_t ts = 0;
	int inputLen = decodeParser->getH264(h264Data, ts);
	int count = 0;
	av_packet_unref(pkt);
	while (inputLen > 0) {
		int ret = av_parser_parse2(parser, c, &pkt->data, &pkt->size, h264Data + count,
			inputLen, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
		if (ret < 0) {
			throw std::runtime_error("get parsed data error");
		}
		count += ret;
		inputLen -= ret;
		if (pkt->size) {
			decode(pkt, ts);
		}
	}
	delete h264Data;
	h264Data = nullptr;
#endif
}

int VdDecoder::poll(AVFrame* outData) {
	std::unique_lock<std::mutex> locker(sendMtx);
	//sendCv.wait(locker, [&] {return !playList.empty(); });
	if (playList.empty()) {
		return 0;
	}
	auto frameRGB = playList.front();
	playList.pop_front();
	av_frame_ref(outData, frameRGB);
	av_frame_free(&frameRGB);
	return 0;
}

void VdDecoder::setPixFmt(AVPixelFormat fmt) {
	dstPixFmt = fmt;
}

void VdDecoder::stop() {
	std::unique_lock<std::mutex> locker(sendMtx);
	if (outBuff) {
		av_free(outBuff);
		outBuff = nullptr;
	}
	if (sws_ctx) {
		sws_freeContext(sws_ctx);
		sws_ctx = nullptr;
	}

	if (parser) {
		av_parser_close(parser);
		parser = nullptr;
	}
	if (c) {
		avcodec_free_context(&c);
		c = nullptr;
	}
	if (pkt) {
		av_packet_free(&pkt);
	}
	for (auto data : playList) {
		av_frame_free(&data);
	}
	playList.clear();
	decodeParser = nullptr;
	decodeReady = false;
}

void VdDecoder::scale(AVFrame* frame, int width, int height) {
	if (outputWidth != width || outputHeight != height) {
		outputWidth = width;
		outputHeight = height;
		initSws();
	}
	if (!frame) {

	}
	else if (frame->width != width || frame->height != height) {
		SwsContext* sws_ctx = nullptr;
		if (!sws_ctx) {
			sws_ctx = sws_getCachedContext(sws_ctx, frame->width, frame->height, dstPixFmt, width, height, dstPixFmt, SWS_BILINEAR, NULL, NULL, NULL);
		}
		auto frameRGB = av_frame_alloc();
		frameRGB->format = dstPixFmt;
		frameRGB->width = width;
		frameRGB->height = height;
		//frameRGB->pts = frame->pts;
		av_frame_get_buffer(frameRGB, ALIGN);
		int ret = sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height, frameRGB->data, frameRGB->linesize);
		if (ret <= 0) {
			throw std::runtime_error("sws_scale error");
		}
		av_frame_get_buffer(frame, ALIGN);
		av_frame_ref(frame, frameRGB);
		av_frame_free(&frameRGB);
		sws_freeContext(sws_ctx);
	}
}

// private
void VdDecoder::setAVFormatContext(AVFormatContext* formatContext) {
	formatCtx = formatContext;
}
void VdDecoder::decode(AVPacket* pkt, uint64_t ts) {
	int ret;
	ret = avcodec_send_packet(c, pkt);
	if (ret < 0) {
		if (ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
			E_LOG("decode send pkt error, {}", ret);
		}
	}

	while (ret >= 0) {
		auto frameYUV = av_frame_alloc();
		ret = avcodec_receive_frame(c, frameYUV);
		if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
			av_frame_free(&frameYUV);
			return;
		}
		else if (ret < 0) {
			av_frame_free(&frameYUV);
			E_LOG("decode recv frame error");
			return;
		}
		frameYUV->pts = ts;

		//std::unique_lock<std::mutex> locker(*playMtx);
		changeFmtAndSave(frameYUV);
		//locker.unlock();
		//av_packet_unref(pkt);
		av_frame_free(&frameYUV);
	}
}
void VdDecoder::initSws()
{
	AVPixelFormat srcPixFmt = AV_PIX_FMT_YUV420P;
	sws_ctx = sws_getCachedContext(sws_ctx, inputWidth, inputHeight, srcPixFmt, outputWidth, outputHeight, dstPixFmt, SWS_BILINEAR, NULL, NULL, NULL);

	if (!playList.empty()) {
		AVFrame* frame = playList.front();
		av_frame_free(&frame);
		playList.pop_front();
	}
	if (outBuff) {
		av_free(outBuff);
		outBuff = nullptr;
	}
	outBuff = (uint8_t*)av_malloc(av_image_get_buffer_size(dstPixFmt, outputWidth, outputHeight, ALIGN) * sizeof(uint8_t));
	I_LOG("init decoder sws success");
}
void VdDecoder::changeFmtAndSave(AVFrame* frameYUV) {
	if (frameYUV->width != inputWidth || frameYUV->height != inputHeight) {
		inputWidth = frameYUV->width;
		inputHeight = frameYUV->height;
		outputWidth = inputWidth;
		outputHeight = inputHeight;
		initSws();
	}
	auto frameRGB = av_frame_alloc();
	frameRGB->format = dstPixFmt;
	frameRGB->width = outputWidth;
	frameRGB->height = outputHeight;
	frameRGB->pts = frameYUV->pts;
	av_frame_get_buffer(frameRGB, ALIGN);
	int ret = sws_scale(sws_ctx, frameYUV->data, frameYUV->linesize, 0, frameYUV->height, frameRGB->data, frameRGB->linesize);
	if (ret <= 0) {
		throw std::runtime_error("sws_scale error");
	}

	if (playList.size() > 150) {
		E_LOG("decode push data failed");
	}
	else {
		playList.push_back(frameRGB);
	}
	//sendCv.notify_one();
}
