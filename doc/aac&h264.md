# RTP封装AAC&H264

## RTP相关

RTP header: RTP头不管在什么情况下永远都是如下图的格式：

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

- **V**: RTP协议的版本号，占2bits，当前协议版本号为2
- **P**: 填充标志，占1bit，如果P=1，则在该报文的尾部填充一个或多个额外的八位组，它们不是有效载荷的一部分。
- **X**: 扩展标志，占1bit，如果X=1，则在RTP报头后跟有一个扩展报头
- **CC**: CSRC计数器，占4位，指示CSRC 标识符的个数
- **M**: 1bit，标记解释由设置定义，目的在于允许重要事件在包流中标记出来。如不同的有效载荷有不同的含义，对于视频，标记一帧的结束；对于音频，标记会话的开始。
- **负载类型 Payload type(PT)**: 7bits
    **注：rfc里面对一些早期的格式定义了这个payload type。但是后来的，如h264并没有分配，那就用96来代替。因此现在96以上都不表示特定的格式，具体表示什么要用sdp或者其他协议来协商。**
- **序列号 Sequence number(SN)**: 16bits，用于标识发送者所发送的RTP报文的序列号，每发送一个报文，序列号增1，序列号的初始值是随机产生的。可以用于检查丢包以及进行数据包排序。
- **时间戳 Timestamp**: 32bits，必须使用90kHz时钟频率。
- **同步信源(SSRC)标识符**: 32bits，用于标识同步信源。该标识符是随机随机产生的，参加同一视频会议的两个同步信源不能有相同的SSRC。
- **特约信源(CSRC)标识符**: 每个CSRC标识符占32bits，可以有0～15个。每个CSRC标识了包含在该RTP报文有效载荷中的所有特约信源。



## AAC 相关知识

AAC音频文件格式：

AAC音频文件由一帧一帧的ADTS帧组成，每个ADTS帧又包含ADTS header 和 AAC Data.

```
+-----ADTS Frame-----+-----ADTS Frame-----+-----ADTS Frame-----+
|  ADTS  |    AAC    |  ADTS  |    AAC    |  ADTS  |    AAC    |
| header |   Data    | header |   Data    | header |   Data    |
+--------------------------------------------------------------+
```

将AAC封包为RTP主要是将ADTS头去掉，留下ADTS的数据即AAC的音频数据。其大小一般为7个字节。
**其ADTS heaser的参考如下:**

```C++
struct AdtsHeader
{
  unsigned int syncword;  //12 bit 同步字 总是'1111 1111 1111'，说明一个ADTS帧的开始
  unsigned int id;        //1 bit MPEG 标示符， 0 for MPEG-4，1 for MPEG-2
  unsigned int layer;     //2 bit 总是'00'
  unsigned int protectionAbsent;  //1 bit 1表示没有crc，0表示有crc
  unsigned int profile;           //1 bit 表示使用哪个级别的AAC
  unsigned int samplingFreqIndex; //4 bit 表示使用的采样频率 使用一个对应表可以对应
  unsigned int privateBit;        //1 bit
  unsigned int channelCfg; //3 bit 表示声道数
  unsigned int originalCopy;         //1 bit 
  unsigned int home;                  //1 bit 

  /*下面的为改变的参数即每一帧都不同*/
  unsigned int copyrightIdentificationBit;   //1 bit
  unsigned int copyrightIdentificationStart; //1 bit
  unsigned int aacFrameLength;               //13 bit 一个ADTS帧的长度包括ADTS头和AAC原始流
  unsigned int adtsBufferFullness;           //11 bit 0x7FF 说明是码率可变的码流

  /* number_of_raw_data_blocks_in_frame
   * 表示ADTS帧中有number_of_raw_data_blocks_in_frame + 1个AAC原始帧
   * 所以说number_of_raw_data_blocks_in_frame == 0
   * 表示说ADTS帧中有一个AAC数据块并不是说没有。(一个AAC原始帧包含一段时间内1024个采样及相关数据)
   */
  unsigned int numberOfRawDataBlockInFrame; //2 bit
};

```

因为AAC的一帧ADTS数据一般为几百个字节，不需要进行分片，所以一般。AAC的RTP打包方式就是将ADTS帧取出ADTS头部，取出AAC数据，每帧数据封装成一个RTP包。

**需要特别注意的是，并不是将aac数据直接拷贝到RTP的payload中，而是在payload的前面四个字节需要进行设置。**

```
+----------+----------------RTP payload-----------------------+
|   RTP    |0x00|0x01|size|size|                              |
|  header  |    |    |high| low|                              |
+----------+--------------------------------------------------+
```

其中RTP载荷的第一个字节为0x00，第二个字节为0x10

第三个字节和第四个字节保存AAC Data的大小，最多只能保存13bit，第三个字节保存数据大小的高八位，第四个字节的高5位保存数据大小的低5位。

## H264

NALU*(网络抽象层单元  Network Abstract Layer Unit）*头的结构：

```
      +---------------+
      |0|1|2|3|4|5|6|7|
      +-+-+-+-+-+-+-+-+
      |F|NRI|  Type   |
      +---------------+
```



```C++
F: 1 bit.
  forbidden_zero_bit. 在 H.264 规范中规定了这一位必须为 0.

NRI: 2 bit.
  nal_ref_idc. 取00~11,似乎指示这个NALU的重要性,如00的NALU解码器可以丢弃它而不影响图像的回放.  
    
Type: 5 bit
  0     没有定义
  1-23  NAL单元  单个 NAL 单元包
  24    STAP-A   单一时间的组合包
  25    STAP-B   单一时间的组合包
  26    MTAP16   多个时间的组合包
  27    MTAP24   多个时间的组合包
  28    FU-A     分片的单元
  29    FU-B     分片的单元
  30-31 没有定义
    
  eg：
    sps (sequence parameter sets) 序列参数集 type : 7
    pps (picture parameter sets) 图像参数集 type : 8
```



 **h264仅用1-23,24以后的用在RTP H264负载类型头中**

SDP文件描述和封包的关联：

```
m=video 1234 RTP/AVP 96
a=rtpmap:96 H264/90000
a=fmtp:96 profile-level-id=42A01E; packetization-mode=1; sprop-parameter-sets=Z0IACpZTBYmI,aMljiA==
```

packetization-mode:  表示支持的封包模式. 

- 为 0 时或不存在时, 必须使用单一 NALU 单元模式.
- 为 1 时必须使用非交错(non-interleaved)封包模式.
- 为 2 时必须使用交错(interleaved)封包模式.

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

### H264封包介绍

- **单一NAL单元模式**

  当一个NALU小于MTU（*最大传输单元Maximum Transmission Unit，MTU*）时，一般采用单一模式，即一个RTP包承载一个NALU。

   对于一个原始的 H.264 NALU 单元常由 [Start Code] [NALU Header] [NALU Payload] 三部分组成, 其中 Start Code 用于标示这是一个

  **NALU 单元的开始, 必须是 "00 00 00 01" 或 "00 00 01", NALU 头仅一个字节, 其后都是 NALU 单元内容.
    打包时去除 "00 00 01" 或 "00 00 00 01" 的开始码, 把其他数据封包的 RTP 包即可.**

  **eg:**

  ![image-20220120095609139](C:\Users\Astro\AppData\Roaming\Typora\typora-user-images\image-20220120095609139.png)

  这是一个67开头的sps在RTP中的状态，如果在H264中它应该有00000001或者000001的start code.

  **其格式如下：**

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

  

- **组合封包模式**

  当NALU长度特别小时，可以将几个NALU放进一个RTP包中，在实际使用场景中，这种模式是使用最少的。

  **其格式如下：**

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

  

- **分片的单元**

  当NALU长度特别大时（超过了MTU），这个时候就要将一个NALU进行分片封包*（Fragmentation Units FUs）*

![image-20220120101218370](C:\Users\Astro\AppData\Roaming\Typora\typora-user-images\image-20220120101218370.png)

**其格式如下：**

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

**下面的文章给出了一个很好的例子：**

0x7C85=01111100 10000101 (开始包)

0x7C05=01111100 00000101 (中间包)

0x7C45=01111100 01000101 (结束包)

如有一个 H.264 的 NALU 是这样的:

  [00 00 00 01 65 42 A0 1E 23 56 0E 2F ...  02 17 C8 FD F1 B9 C7 53 59 72 ... CB FF FF F4 1A D5 C4 18 A8 ... F1 B9 C7 1D A5 FA 13 0B ...]

封装成 RTP 包将如下:

[ RTP Header ] [ 7C 85 42 A0 1E 23 56 0E 2F ...]

[ RTP Header ] [ 7C 05 02 17 C8 FD F1 B9 C7 53 59 72 ...]

[ RTP Header ] [ 7C 05 CB FF FF F4 1A D5 C4 18 A8 ...]

[ RTP Header ] [ 7C 45 F1 B9 C7 1D A5 FA 13 0B ...]



**参考**：[(126条消息) RTP封装h264_jwybobo2007的博客-CSDN博客_h264 rtp](https://blog.csdn.net/jwybobo2007/article/details/7054140)