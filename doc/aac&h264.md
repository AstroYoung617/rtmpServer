# RTP��װAAC&H264

## RTP���

RTP header: RTPͷ������ʲô�������Զ��������ͼ�ĸ�ʽ��

```
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |V=2|P|X|  CC   |M|     PT      |       sequence number         |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                           timestamp                           |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |           synchronization source (SSRC) identifier            |
      +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
      |            contributing source (CSRC) identifiers             |
      |                             ....                              |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

- **V**: RTPЭ��İ汾�ţ�ռ2bits����ǰЭ��汾��Ϊ2
- **P**: ����־��ռ1bit�����P=1�����ڸñ��ĵ�β�����һ����������İ�λ�飬���ǲ�����Ч�غɵ�һ���֡�
- **X**: ��չ��־��ռ1bit�����X=1������RTP��ͷ�����һ����չ��ͷ
- **CC**: CSRC��������ռ4λ��ָʾCSRC ��ʶ���ĸ���
- **M**: 1bit����ǽ��������ö��壬Ŀ������������Ҫ�¼��ڰ����б�ǳ������粻ͬ����Ч�غ��в�ͬ�ĺ��壬������Ƶ�����һ֡�Ľ�����������Ƶ����ǻỰ�Ŀ�ʼ��
- **�������� Payload type(PT)**: 7bits
    **ע��rfc�����һЩ���ڵĸ�ʽ���������payload type�����Ǻ����ģ���h264��û�з��䣬�Ǿ���96�����档�������96���϶�����ʾ�ض��ĸ�ʽ�������ʾʲôҪ��sdp��������Э����Э�̡�**
- **���к� Sequence number(SN)**: 16bits�����ڱ�ʶ�����������͵�RTP���ĵ����кţ�ÿ����һ�����ģ����к���1�����кŵĳ�ʼֵ����������ġ��������ڼ�鶪���Լ��������ݰ�����
- **ʱ��� Timestamp**: 32bits������ʹ��90kHzʱ��Ƶ�ʡ�
- **ͬ����Դ(SSRC)��ʶ��**: 32bits�����ڱ�ʶͬ����Դ���ñ�ʶ���������������ģ��μ�ͬһ��Ƶ���������ͬ����Դ��������ͬ��SSRC��
- **��Լ��Դ(CSRC)��ʶ��**: ÿ��CSRC��ʶ��ռ32bits��������0��15����ÿ��CSRC��ʶ�˰����ڸ�RTP������Ч�غ��е�������Լ��Դ��



## AAC ���֪ʶ

AAC��Ƶ�ļ���ʽ��

AAC��Ƶ�ļ���һ֡һ֡��ADTS֡��ɣ�ÿ��ADTS֡�ְ���ADTS header �� AAC Data.

```
+-----ADTS Frame-----+-----ADTS Frame-----+-----ADTS Frame-----+
|  ADTS  |    AAC    |  ADTS  |    AAC    |  ADTS  |    AAC    |
| header |   Data    | header |   Data    | header |   Data    |
+--------------------------------------------------------------+
```

��AAC���ΪRTP��Ҫ�ǽ�ADTSͷȥ��������ADTS�����ݼ�AAC����Ƶ���ݡ����Сһ��Ϊ7���ֽڡ�
**��ADTS heaser�Ĳο�����:**

```C++
struct AdtsHeader
{
  unsigned int syncword;  //12 bit ͬ���� ����'1111 1111 1111'��˵��һ��ADTS֡�Ŀ�ʼ
  unsigned int id;        //1 bit MPEG ��ʾ���� 0 for MPEG-4��1 for MPEG-2
  unsigned int layer;     //2 bit ����'00'
  unsigned int protectionAbsent;  //1 bit 1��ʾû��crc��0��ʾ��crc
  unsigned int profile;           //1 bit ��ʾʹ���ĸ������AAC
  unsigned int samplingFreqIndex; //4 bit ��ʾʹ�õĲ���Ƶ�� ʹ��һ����Ӧ����Զ�Ӧ
  unsigned int privateBit;        //1 bit
  unsigned int channelCfg; //3 bit ��ʾ������
  unsigned int originalCopy;         //1 bit 
  unsigned int home;                  //1 bit 

  /*�����Ϊ�ı�Ĳ�����ÿһ֡����ͬ*/
  unsigned int copyrightIdentificationBit;   //1 bit
  unsigned int copyrightIdentificationStart; //1 bit
  unsigned int aacFrameLength;               //13 bit һ��ADTS֡�ĳ��Ȱ���ADTSͷ��AACԭʼ��
  unsigned int adtsBufferFullness;           //11 bit 0x7FF ˵�������ʿɱ������

  /* number_of_raw_data_blocks_in_frame
   * ��ʾADTS֡����number_of_raw_data_blocks_in_frame + 1��AACԭʼ֡
   * ����˵number_of_raw_data_blocks_in_frame == 0
   * ��ʾ˵ADTS֡����һ��AAC���ݿ鲢����˵û�С�(һ��AACԭʼ֡����һ��ʱ����1024���������������)
   */
  unsigned int numberOfRawDataBlockInFrame; //2 bit
};

```

��ΪAAC��һ֡ADTS����һ��Ϊ���ٸ��ֽڣ�����Ҫ���з�Ƭ������һ�㡣AAC��RTP�����ʽ���ǽ�ADTS֡ȡ��ADTSͷ����ȡ��AAC���ݣ�ÿ֡���ݷ�װ��һ��RTP����

**��Ҫ�ر�ע����ǣ������ǽ�aac����ֱ�ӿ�����RTP��payload�У�������payload��ǰ���ĸ��ֽ���Ҫ�������á�**

```
+----------+----------------RTP payload-----------------------+
|   RTP    |0x00|0x01|size|size|                              |
|  header  |    |    |high| low|                              |
+----------+--------------------------------------------------+
```

����RTP�غɵĵ�һ���ֽ�Ϊ0x00���ڶ����ֽ�Ϊ0x10

�������ֽں͵��ĸ��ֽڱ���AAC Data�Ĵ�С�����ֻ�ܱ���13bit���������ֽڱ������ݴ�С�ĸ߰�λ�����ĸ��ֽڵĸ�5λ�������ݴ�С�ĵ�5λ��

## H264

NALU*(�������㵥Ԫ  Network Abstract Layer Unit��*ͷ�Ľṹ��

```
      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+
```



```C++
F: 1 bit.
  forbidden_zero_bit. �� H.264 �淶�й涨����һλ����Ϊ 0.

NRI: 2 bit.
  nal_ref_idc. ȡ00~11,�ƺ�ָʾ���NALU����Ҫ��,��00��NALU���������Զ���������Ӱ��ͼ��Ļط�.  
    
Type: 5 bit
  0     û�ж���
  1-23  NAL��Ԫ  ���� NAL ��Ԫ��
  24    STAP-A   ��һʱ�����ϰ�
  25    STAP-B   ��һʱ�����ϰ�
  26    MTAP16   ���ʱ�����ϰ�
  27    MTAP24   ���ʱ�����ϰ�
  28    FU-A     ��Ƭ�ĵ�Ԫ
  29    FU-B     ��Ƭ�ĵ�Ԫ
  30-31 û�ж���
    
  eg��
    sps (sequence parameter sets) ���в����� type : 7
    pps (picture parameter sets) ͼ������� type : 8
```



 **h264����1-23,24�Ժ������RTP H264��������ͷ��**

SDP�ļ������ͷ���Ĺ�����

```
m=video 1234 RTP/AVP 96
a=rtpmap:96 H264/90000
a=fmtp:96 profile-level-id=42A01E; packetization-mode=1; sprop-parameter-sets=Z0IACpZTBYmI,aMljiA==
```

packetization-mode:  ��ʾ֧�ֵķ��ģʽ. 

- Ϊ 0 ʱ�򲻴���ʱ, ����ʹ�õ�һ NALU ��Ԫģʽ.
- Ϊ 1 ʱ����ʹ�÷ǽ���(non-interleaved)���ģʽ.
- Ϊ 2 ʱ����ʹ�ý���(interleaved)���ģʽ.

```
      Type   Packet    Single NAL    Non-Interleaved    Interleaved
                       Unit Mode           Mode             Mode
      -------------------------------------------------------------
 
      0      undefined     ig               ig               ig
      1-23   NAL unit     yes              yes               no
      24     STAP-A        no              yes               no
      25     STAP-B        no               no              yes
      26     MTAP16        no               no              yes
      27     MTAP24        no               no              yes
      28     FU-A          no              yes              yes
      29     FU-B          no               no              yes
      30-31  undefined     ig               ig               ig
```

### H264�������

- **��һNAL��Ԫģʽ**

  ��һ��NALUС��MTU��*����䵥ԪMaximum Transmission Unit��MTU*��ʱ��һ����õ�һģʽ����һ��RTP������һ��NALU��

   ����һ��ԭʼ�� H.264 NALU ��Ԫ���� [Start Code] [NALU Header] [NALU Payload] ���������, ���� Start Code ���ڱ�ʾ����һ��

  **NALU ��Ԫ�Ŀ�ʼ, ������ "00 00 00 01" �� "00 00 01", NALU ͷ��һ���ֽ�, ����� NALU ��Ԫ����.
    ���ʱȥ�� "00 00 01" �� "00 00 00 01" �Ŀ�ʼ��, ���������ݷ���� RTP ������.**

  **eg:**

  ![image-20220120095609139](C:\Users\Astro\AppData\Roaming\Typora\typora-user-images\image-20220120095609139.png)

  ����һ��67��ͷ��sps��RTP�е�״̬�������H264����Ӧ����00000001����000001��start code.

  **���ʽ���£�**

  ```
         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |F|NRI|  type   |                                               |
        +-+-+-+-+-+-+-+-+                                               |
        |                                                               |
        |               Bytes 2..n of a Single NAL unit                 |
        |                                                               |
        |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                               :...OPTIONAL RTP padding        |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  ```

  

- **��Ϸ��ģʽ**

  ��NALU�����ر�Сʱ�����Խ�����NALU�Ž�һ��RTP���У���ʵ��ʹ�ó����У�����ģʽ��ʹ�����ٵġ�

  **���ʽ���£�**

  ```
         0                   1                   2                   3
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                          RTP Header                           |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         NALU 1 Data                           |
        :                                                               :
        +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |               | NALU 2 Size                   | NALU 2 HDR    |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                         NALU 2 Data                           |
        :                                                               :
        |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        |                               :...OPTIONAL RTP padding        |
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  ```

  

- **��Ƭ�ĵ�Ԫ**

  ��NALU�����ر��ʱ��������MTU�������ʱ���Ҫ��һ��NALU���з�Ƭ���*��Fragmentation Units FUs��*

![image-20220120101218370](C:\Users\Astro\AppData\Roaming\Typora\typora-user-images\image-20220120101218370.png)

**���ʽ���£�**

```
       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      | FU indicator  |   FU header   |                               |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+                               |
      |                                                               |
      |                         FU payload                            |
      |                                                               |
      |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |                               :...OPTIONAL RTP padding        |
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```

**��������¸�����һ���ܺõ����ӣ�**

0x7C85=01111100 10000101 (��ʼ��)

0x7C05=01111100 00000101 (�м��)

0x7C45=01111100 01000101 (������)

����һ�� H.264 �� NALU ��������:

  [00 00 00 01 65 42 A0 1E 23 56 0E 2F ...  02 17 C8 FD F1 B9 C7 53 59 72 ... CB FF FF F4 1A D5 C4 18 A8 ... F1 B9 C7 1D A5 FA 13 0B ...]

��װ�� RTP ��������:

[ RTP Header ] [ 7C 85 42 A0 1E 23 56 0E 2F ...]

[ RTP Header ] [ 7C 05 02 17 C8 FD F1 B9 C7 53 59 72 ...]

[ RTP Header ] [ 7C 05 CB FF FF F4 1A D5 C4 18 A8 ...]

[ RTP Header ] [ 7C 45 F1 B9 C7 1D A5 FA 13 0B ...]



**�ο�**��[(126����Ϣ) RTP��װh264_jwybobo2007�Ĳ���-CSDN����_h264 rtp](https://blog.csdn.net/jwybobo2007/article/details/7054140)