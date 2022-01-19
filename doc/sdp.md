SDP:
v=0
o=- 0 0 IN IP4 233.255.42.42
s=No Name
c=IN IP4 233.255.42.42
t=0 0
a=tool:libavformat 58.29.100
m=audio 9832 RTP/AVP 97
b=AS:69
a=rtpmap:97 MPEG4-GENERIC/32000/1
a=fmtp:97 profile-level-id=1;mode=AAC-hbr;sizelength=13;indexlength=3;indexdeltalength=3; config=128856E500

# 上面就是一个音频的sdp示例


格式为 m=<媒体类型> <端口号> <传输协议> <媒体格式 >
媒体类型：audio，表示这是一个音频流

端口号：9832，表示UDP发送的目的端口为9832

传输协议：RTP/AVP，表示RTP OVER UDP，通过UDP发送RTP包

媒体格式：表示负载类型(payload type)，一般使用97表示AAC

a=rtpmap:97 mpeg4-generic/32000/1

格式为a=rtpmap:<媒体格式><编码格式>/<时钟频率> /[channel]

mpeg4-generic表示编码，32000表示时钟频率，1表示双通道

c=IN IP4 233.255.42.42

IN：表示internet

IP4：表示IPV4

233.255.42.42：表示UDP发送的目的地址为233.255.42.42
