extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

//用于存放多个结构体
#pragma once
typedef struct
{
	unsigned char version : 2;          	//!< Version, 2 bits, MUST be 0x2
	unsigned char padding : 1;			 	//!< Padding bit, Padding MUST NOT be used
	unsigned char extension : 1;			//!< Extension, MUST be zero
	unsigned char cc : 4;       	   		//!< CSRC count, normally 0 in the absence of RTP mixers 		
	unsigned char marker : 1;			   	//!< Marker bit
	unsigned char pt : 7;			   		//!< 7 bits, Payload Type, dynamically established
	uint16_t seq_no;			   	//!< RTP sequence number, incremented by one for each sent packet 
	uint32_t timestamp;	       //!< timestamp, 27 MHz for H.264
	uint32_t ssrc;			   //!< Synchronization Source, chosen randomly
	unsigned char* payload;      //!< the payload including payload headers
	unsigned int paylen;		   //!< length of payload in bytes
} RtpPacket;

typedef struct
{
	/*  0                   1                   2                   3
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
		*/
		//intel 的cpu 是intel为小端字节序（低端存到底地址） 而网络流为大端字节序（高端存到低地址）
		/*intel 的cpu ： 高端->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->低端
		 在内存中存储 ：
		 低->4001（内存地址）version:2
				 4002（内存地址）padding:1
			 4003（内存地址）extension:1
		 高->4004（内存地址）csrc_len:4
			 网络传输解析 ： 高端->version:2->padding:1->extension:1->csrc_len:4->低端  (为正确的文档描述格式)
		 存入接收内存 ：
		 低->4001（内存地址）version:2
				 4002（内存地址）padding:1
				 4003（内存地址）extension:1
		 高->4004（内存地址）csrc_len:4
		 本地内存解析 ：高端->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->低端 ，
		 即：
		 unsigned char csrc_len:4;        // expect 0
		 unsigned char extension:1;       // expect 1
		 unsigned char padding:1;         // expect 0
		 unsigned char version:2;         // expect 2
		*/
		/* byte 0 */
	unsigned char csrc_len : 4;        /* expect 0 */
	unsigned char extension : 1;       /* expect 1, see RTP_OP below */
	unsigned char padding : 1;         /* expect 0 */
	unsigned char version : 2;         /* expect 2 */
 /* byte 1 */
	unsigned char payloadtype : 7;     /* RTP_PAYLOAD_RTSP */
	unsigned char marker : 1;          /* expect 1 */
 /* bytes 2,3 */
	uint16_t seq_no;
	/* bytes 4-7 */
	uint32_t timestamp;
	/* bytes 8-11 */
	uint32_t ssrc;              /* stream number is used here. */
} RtpHeader;

struct AdtsHeader
{
	unsigned int syncword;  //12 bit 同步字 '1111 1111 1111'，说明一个ADTS帧的开始
	unsigned int id;        //1 bit MPEG 标示符， 0 for MPEG-4，1 for MPEG-2
	unsigned int layer;     //2 bit 总是'00'
	unsigned int protectionAbsent;  //1 bit 1表示没有crc，0表示有crc
	unsigned int profile;           //1 bit 表示使用哪个级别的AAC
	unsigned int samplingFreqIndex; //4 bit 表示使用的采样频率
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

typedef struct
{
	unsigned char forbidden_bit;           //! Should always be FALSE
	unsigned char nal_reference_idc;       //! NALU_PRIORITY_xxxx
	unsigned char nal_unit_type;           //! NALU_TYPE_xxxx  
	unsigned int startcodeprefix_len;      //! 前缀字节数
	unsigned int len;                      //! 包含nal 头的nal 长度，从第一个00000001到下一个000000001的长度
	unsigned int max_size;                 //! 做多一个nal 的长度
	unsigned char* buf;                   //! 包含nal 头的nal 数据
	unsigned int lost_packets;             //! 预留
} NALU_t;

/*
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
*/
typedef struct
{
	//byte 0
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} NALU_HEADER; // 1 BYTE 

/*
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|F|NRI|  Type   |
+---------------+
*/
typedef struct
{
	//byte 0
	unsigned char TYPE : 5;
	unsigned char NRI : 2;
	unsigned char F : 1;
} FU_INDICATOR; // 1 BYTE 

/*
+---------------+
|0|1|2|3|4|5|6|7|
+-+-+-+-+-+-+-+-+
|S|E|R|  Type   |
+---------------+
*/
typedef struct
{
	//byte 0
	unsigned char TYPE : 5;
	unsigned char R : 1;
	unsigned char E : 1;
	unsigned char S : 1;
} FU_HEADER;   // 1 BYTES 