# rtmpServer🐥

## 简介
这个项目主要用于进行将RTP转发至RTMP服务器，它将实现将多路RTP流拉取、视频拼接缩放、音频混流，输出一份H.264视频及一份AAC音频至RTMP服务器上，可以是Bilibili、斗鱼、虎牙，或者是其他平台，更或者是推送至多平台同时播放。

*小声bb: 目前以上内容还未完成，只测试了发送h264及aac文件或者mp4文件至rtmp服务器，完成了把这句话删了*

## 发展

- 2022.1.12 完成项目搭建，进行了MP4、h264、aac的rtmp发送测试，使用ffmpeg尝试了rtp发送测试，因为jrtplib被ban了，采用了live555这个开源的rtp库，完成了编译以及发送测试，待进行接收测试。
- 2022.1.13 完成live555库的CMake链接，测试发送rtp遇到问题，使用官网中的测试程序发送ts文件过后，用vlc播放网络串流失败。
- 2022.1.17 发现之前的方向错了，对RTP的理解还不够，今天安排先看文档和asio(non-boost)的使用和测试，编写客户端和服务器的echo程序。完成asio的udp、tcp服务器和异步客户端的程序测试，对于RTP而言，仍然使用了live555进行测试rtp的发送，目前进行了h264的发送测试，使用wireshark保存264文件验证能够正常播放。
- 2022.1.18 编写RTP客户端及服务端，完成客户端发送RTP文本消息，服务端返回逆序RTP文本消息。
- 2022.1.19 编写程序发送RTP音频数据，经过ffplay测试播放sdp可以正常播放。
## LICENCE
[MIT](https://github.com/AstroYoung617/rtmpServer/blob/main/LICENSE)
