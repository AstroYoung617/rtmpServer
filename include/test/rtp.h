#ifndef _RTP_H_
#define _RTP_H_
#include <stdint.h>
 
#define RTP_VESION              2
 
#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97
 
#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400
 
/*
 *
 *    0                   1                   2                   3
 *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |                           timestamp                           |
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *   |           synchronization source (SSRC) identifier            |
 *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *   |            contributing source (CSRC) identifiers             |
 *   :                             ....                              :
 *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *		*/
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
struct RtpHeader
{
    /* byte 0 */
    uint8_t csrcLen:4;
    uint8_t extension:1;
    uint8_t padding:1;
    uint8_t version:2;
 
    /* byte 1 */
    uint8_t payloadType:7;
    uint8_t marker:1;
    
    /* bytes 2,3 */
    uint16_t seq;
    
    /* bytes 4-7 */
    uint32_t timestamp;
    
    /* bytes 8-11 */
    uint32_t ssrc;
};
 
struct RtpPacket
{
    struct RtpHeader rtpHeader;
    uint8_t payload[0];
};

void rtpHeaderInit(struct RtpPacket* rtpPacket, uint8_t csrcLen, uint8_t extension,
  uint8_t padding, uint8_t version, uint8_t payloadType, uint8_t marker,
  uint16_t seq, uint32_t timestamp, uint32_t ssrc);
int rtpSendPacket(int socket, char* ip, int16_t port, struct RtpPacket* rtpPacket, uint32_t dataSize);
void printRtpPacket(RtpPacket* _packet);
#endif //_RTP_H_