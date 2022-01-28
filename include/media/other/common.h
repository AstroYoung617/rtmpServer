extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
};

//���ڴ�Ŷ���ṹ��
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
		//intel ��cpu ��intelΪС���ֽ��򣨵Ͷ˴浽�׵�ַ�� ��������Ϊ����ֽ��򣨸߶˴浽�͵�ַ��
		/*intel ��cpu �� �߶�->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->�Ͷ�
		 ���ڴ��д洢 ��
		 ��->4001���ڴ��ַ��version:2
				 4002���ڴ��ַ��padding:1
			 4003���ڴ��ַ��extension:1
		 ��->4004���ڴ��ַ��csrc_len:4
			 ���紫����� �� �߶�->version:2->padding:1->extension:1->csrc_len:4->�Ͷ�  (Ϊ��ȷ���ĵ�������ʽ)
		 ��������ڴ� ��
		 ��->4001���ڴ��ַ��version:2
				 4002���ڴ��ַ��padding:1
				 4003���ڴ��ַ��extension:1
		 ��->4004���ڴ��ַ��csrc_len:4
		 �����ڴ���� ���߶�->csrc_len:4 -> extension:1-> padding:1 -> version:2 ->�Ͷ� ��
		 ����
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
	unsigned int syncword;  //12 bit ͬ���� '1111 1111 1111'��˵��һ��ADTS֡�Ŀ�ʼ
	unsigned int id;        //1 bit MPEG ��ʾ���� 0 for MPEG-4��1 for MPEG-2
	unsigned int layer;     //2 bit ����'00'
	unsigned int protectionAbsent;  //1 bit 1��ʾû��crc��0��ʾ��crc
	unsigned int profile;           //1 bit ��ʾʹ���ĸ������AAC
	unsigned int samplingFreqIndex; //4 bit ��ʾʹ�õĲ���Ƶ��
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

typedef struct
{
	unsigned char forbidden_bit;           //! Should always be FALSE
	unsigned char nal_reference_idc;       //! NALU_PRIORITY_xxxx
	unsigned char nal_unit_type;           //! NALU_TYPE_xxxx  
	unsigned int startcodeprefix_len;      //! ǰ׺�ֽ���
	unsigned int len;                      //! ����nal ͷ��nal ���ȣ��ӵ�һ��00000001����һ��000000001�ĳ���
	unsigned int max_size;                 //! ����һ��nal �ĳ���
	unsigned char* buf;                   //! ����nal ͷ��nal ����
	unsigned int lost_packets;             //! Ԥ��
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