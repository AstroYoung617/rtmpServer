#pragma once
extern "C" {
#include "libswscale/swscale.h"
};
#include <other/loggerApi.h>
#include <media/other/Parser.h>
#include <media/other/common.h>
#include <list>
#include <mutex>
#include <map>
#include <memory>
#include <vector>

#define MAX_ENCODE_WIDTH 1280
#define MAX_ENCODE_HEIGHT 960

class Parser;

class Encoder {
private:
	//...
	AVCodecContext* encodeCtx = nullptr;
	AVCodec* encode = nullptr;
	SwsContext* swsCtx = nullptr;
	uint8_t* outBuff = nullptr;
	AVPixelFormat inputFmt = AV_PIX_FMT_YUV420P;
	std::shared_ptr<Parser> encodeParser = nullptr;

	//���
	AVFormatContext* pFormatCtx = nullptr;
	AVOutputFormat* fmt = nullptr;
	AVStream* video_st = nullptr;
	bool encodeReady = false;
	bool isSaveToFile = false;
	std::string out_file_path = "test_okk_87.h264";
	int count = 0;



	//lock
	std::mutex sendMtx{};
	//std::condition_variable sendCv{};
	std::list<AVPacket*> sendList{}; //TODO �ڴ���Ҫ�ͷ�

	AVCodecID codecFmt = AV_CODEC_ID_H264;			//����ѹ����ʽ��֧��h264��vp8
	AVPixelFormat pixFmt = AV_PIX_FMT_YUV420P;  //��������ͼ��ĸ�ʽ��֧��yuv��rgba���ָ�ʽ

	//�����yuv avframe�ĳߴ�
	int inputWidth = -1;
	int inputHeight = -1;

	//������Ƶ�ֱ���
	int outputWidth = -1;    //��
	int outputHeight = -1;   //��
	bool isScale = false;

	int bitRate = 10000;  //������Ƶ����
	int framerate = 10;     //������Ƶ֡��
	int gopsize = 10;       //���ùؼ�֮֡���ʱ���
	int maxBframe = 0;      //�������B֡��

	std::string Preset = "medium";//����presetȡֵ
	std::string tune = "zerolatency"; //����tuneȡֵ ����������Ƶ�����ݽ����Ż�
	std::string Crf = "32.0";  //�������ʿ��Ʒ������㶨����
	int Profile = 100; //�������������Ƶ����profile

	uint64_t dts = 1;       //

	bool flv_flag = false;	//����flv��sps��pps�Ļ�ȡ��ʽ�����ֵΪtrue������I֡��naluǰ������sps��pps��Ϣ����Ҫ��encoderCtx��extradata�л�ȡ��������rtmp����
							//���ֵΪfalse�������ÿ��I֡��naluǰ����sps��pps��Ϣ��������theia����

	bool isCrf = true;
	void initSws();
	int bufferToAVFrame(uint8_t* data, AVFrame* yuvFrame, size_t width, size_t height);
	//�ж��Ƿ���Ҫ�޸ķֱ����Խ��ʹ���
	bool needRescale();

	int updateOutSize();



public:
	Encoder();
	~Encoder();

	virtual void init();
	virtual void pushWatchFmt(uint8_t* data, int len, int width, int height, AVPixelFormat fmt);
	virtual void push(uint8_t* data, int len, int width, int height);
	virtual int poll(std::vector<std::pair<uint8_t*, int>>& outData);
	virtual AVPacket* pollAVPacket();
	virtual int copyParams(AVCodecParameters* codecpar);
	virtual void stop();
	virtual void setCodecFmt(std::string fmt);
	virtual void setPixFmt(AVPixelFormat fmt);
	virtual void setBitrate(int bitrate);
	virtual void setFrameRate(int frameRate);
	virtual void setFrameSize(int outWidth, int outHeight);
	virtual void setGopSize(int gopSize);
	virtual void setMaxBFrame(int maxBFrame);
	virtual void setPreset(std::string preset);
	virtual void setTune(std::string Tune);
	virtual void setCrf(std::string crf);
	virtual void setProfile(std::string profile);
	virtual void setIsCrf(bool setIsCrf);
	virtual void setIsRTMP(bool isRtmp);


	void initOutput();

	void deleteOutput();

	void setOutput();
};