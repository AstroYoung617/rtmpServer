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

# �������һ����Ƶ��sdpʾ��


��ʽΪ m=<ý������> <�˿ں�> <����Э��> <ý���ʽ >
ý�����ͣ�audio����ʾ����һ����Ƶ��

�˿ںţ�9832����ʾUDP���͵�Ŀ�Ķ˿�Ϊ9832

����Э�飺RTP/AVP����ʾRTP OVER UDP��ͨ��UDP����RTP��

ý���ʽ����ʾ��������(payload type)��һ��ʹ��97��ʾAAC

a=rtpmap:97 mpeg4-generic/32000/1

��ʽΪa=rtpmap:<ý���ʽ><�����ʽ>/<ʱ��Ƶ��> /[channel]

mpeg4-generic��ʾ���룬32000��ʾʱ��Ƶ�ʣ�1��ʾ˫ͨ��

c=IN IP4 233.255.42.42

IN����ʾinternet

IP4����ʾIPV4

233.255.42.42����ʾUDP���͵�Ŀ�ĵ�ַΪ233.255.42.42
