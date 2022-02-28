# rtmpServer🐥

## 简介
这个项目主要用于进行将RTP转发至RTMP服务器，它将实现将多路RTP流拉取、视频拼接缩放、音频混流，输出一份H.264视频及一份AAC音频至RTMP服务器上，可以是Bilibili、斗鱼、虎牙，或者是其他平台，更或者是推送至多平台同时播放。

*小声bb: 目前以上内容还未完成，只测试了发送h264及aac文件或者mp4文件至rtmp服务器，完成了把这句话删了*

## 框架 
    
    *RTPSENDER*                 *AUDIO/VIDEO RECEIVER*      *RTMPCLIENT*
    +---------------------------+---------------------------+------------------------------+
    |                           |                           |                              |
    |                demux      |               decode      |                              |
    |  mp4/file/ -------------->|  AVPacket  -------------->| AVFrame(YUV/RGB/pcm)         |
    |   stream                  | h264(NALU)                |    |                         |
    |                           |    aac                    |    |                         |
    +---------------------------+---------------------------+    |sws_scale / mix          |
    |                           |                           |    |    combine              |
    |                  mux      |                encode     |    V                         |
    |  rtmp/flv  <--------------|  AVPacket  <------------- |AVFrame(YUV/RGB/pcm)/uint8_t[]|
    |                           |  h264/aac                 |                              |
    |                           |                           |                              |
    +---------------------------+---------------------------+------------------------------+
    *NETMANAGER*                *AUDIO/VIDEO SENDER* 


## 发展

- 2022.1.12 完成项目搭建，进行了MP4、h264、aac的rtmp发送测试，使用ffmpeg尝试了rtp发送测试，因为jrtplib被ban了，采用了live555这个开源的rtp库，完成了编译以及发送测试，待进行接收测试。
- 2022.1.13 完成live555库的CMake链接，测试发送rtp遇到问题，使用官网中的测试程序发送ts文件过后，用vlc播放网络串流失败。
- 2022.1.17 发现之前的方向错了，对RTP的理解还不够，今天安排先看文档和asio(non-boost)的使用和测试，编写客户端和服务器的echo程序。完成asio的udp、tcp服务器和异步客户端的程序测试，对于RTP而言，仍然使用了live555进行测试rtp的发送，目前进行了h264的发送测试，使用wireshark保存264文件验证能够正常播放。
- 2022.1.18 编写RTP客户端及服务端，完成客户端发送RTP文本消息，服务端返回逆序RTP文本消息。
- 2022.1.19 编写程序发送RTP音频数据，经过ffplay测试播放sdp可以正常播放。
- 2022.1.19 编写程序发送RTP视频数据（h264），经过ffplay测试播放编写的sdp可以实现正常播放。
- 2022.1.20 编写程序接收RTP音频(aac)，测试可以直接播放，经过接收ffmpeg推送RTP流的测试及自己编写的RTP推流测试可以播放。
- 2022.1.20 编写程序接收RTP视频(h264)，测试可以直接播放，经过接收ffmpeg推送RTP流的测试及自己编写的RTP推流测试可以播放。
- 2022.1.24 按照之间的架构设计进行编写代码，实现将之前完成的RTP接收保存至文件，但是发现一个问题，根据需求来看后面需要进行画面的拼接以及音频的混音，虽然发送的文件格式为aac和h264，但是我们如果要用ffmpeg的api来实现发送rtmp的话，就需要将其封装成avpacket，并且为了满足拼接及混音的要求，需要将其先解码（pcm\yuv）再编码(aac\h264);计划明天继续完成架构的设计以及对接收的RTP数据进行解码保存为yuv和pcm的测试。
- 2022.1.25 完成对视频流的数据进行解码，将解码后的YUV保存到文件，经过测试可以正常播放。
- 2022.1.26 完成对yuv的编码，并发送到b站，完成对aac的音频流解码保存为pcm
- 2022.1.27 完成接收rtp流的h264、aac，发送rtmp到B站进行播放，进行了直接获取h264的AVPacket的操作，直接推送rtmp的AVPacket到B站，该方案可以在固定场景（只有一路音频或者视频的情况、或者只需要保存流的信息的情况）减少性能消耗（编解码），对于需要进行混音、拼接的情况不适用，并且一个功能完备的程序需要做到能够接收多种输入，一种输出，所以编解码可能还是必须的。
- 2022.2.14 完成对aac、h264解码编码后发送至rtmp服务器；发现出现视频卡顿的现象，优先对其进行处理。
    - 使用rtmpServer进行推流到b站，发现播放时会出现频繁卡顿的现象，通过b站播放器的参数可以看到在buffer length比较小时卡顿现象较为明显，而buffer length处于递减的状态，只能通过暂停缓冲的方式使buffer length增大为2s或更大才能正常播放，同时通过观察download bitrate发现下载比特率一直处于较小的水平，这个可能是影响播放效果的主要原因。
- 2022.2.15 解决播放卡顿的问题，一般情况下卡顿现象的发生是因为帧率，需要注意发送rtp的帧率是多少，然后发送rtmp的帧率又是多少，在某些情况下还不能忽略掉因为编解码而引发的延时，这一部分延时同样会影响帧率。如果发送的帧率大于接受的帧率那么出现卡顿的现象是必然的。
- 2022.2.16 尝试创建多个receiver，并创建多种stl容器对相关的对象进行存储管理。
- 2022.2.17 尝试了创建两个receiver，并同时接收两路rtp的音视频数据，发现出现花屏的现象，是因为还没有对视频进行拼接，但是目前也遇到一些问题，例如创建了多个receiver后，这个线程也相应的变多了，该怎么进行有效的管理？
- 2022.2.22-2022.2.25 进行音画同步的尝试，目前的代码是根据接收的帧率进行发送与接收视频帧率相同的视频数据，那么对于接收多路不同帧率的视频流时可能出现问题，所以需要进行补帧或者丢帧而将其以相同的帧率进行发送；
## LICENCE
[MIT](https://github.com/AstroYoung617/rtmpServer/blob/main/LICENSE)
