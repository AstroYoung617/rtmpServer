# rtmpServer🐥

## 简介
这个项目主要用于进行将RTP转发至RTMP服务器，它最终将实现将多路RTP流拉取、视频拼接缩放、音频混流，输出一份H.264视频及一份AAC音频至RTMP服务器上，可以是Bilibili、斗鱼、虎牙，或者是其他平台，更或者是推送至多平台同时播放。

*小声bb: 目前以上内容还未完成，只测试了发送h264及aac文件或者mp4文件至rtmp服务器，完成了把这句话删了*

## 发展

- 2022.1.12 完成项目搭建，进行了MP4、h264、aac的rtmp发送测试，使用ffmpeg尝试了rtp发送测试，因为jrtplib被ban了，采用了live555这个开源的rtp库，完成了编译以及发送测试，待进行接收测试。

## LICENCE
[MIT](https://github.com/AstroYoung617/rtmpServer/blob/main/LICENSE)
