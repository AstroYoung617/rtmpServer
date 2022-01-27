/**
 * Copyright (c) 2021, SeekLoud Team.
 * Date: 2021/09/06
  * Main Developer: duanqiulin
  * Developer:
 * Description: ��Ƶ������
 * Refer: null
 */

#pragma once
#include <string>

//��Ƶ������ʽ
enum class MyAVSampleFormat {
  AV_SAMPLE_FMT_NONE = -1,
  AV_SAMPLE_FMT_U8,   ///< unsigned 8 bits
  AV_SAMPLE_FMT_S16,  ///< signed 16 bits
  AV_SAMPLE_FMT_S32,  ///< signed 32 bits
  AV_SAMPLE_FMT_FLT,  ///< float
  AV_SAMPLE_FMT_DBL,  ///< double

  AV_SAMPLE_FMT_U8P,   ///< unsigned 8 bits, planar
  AV_SAMPLE_FMT_S16P,  ///< signed 16 bits, planar
  AV_SAMPLE_FMT_S32P,  ///< signed 32 bits, planar
  AV_SAMPLE_FMT_FLTP,  ///< float, planar
  AV_SAMPLE_FMT_DBLP,  ///< double, planar
  AV_SAMPLE_FMT_S64,   ///< signed 64 bits
  AV_SAMPLE_FMT_S64P,  ///< signed 64 bits, planar

  AV_SAMPLE_FMT_NB  ///< Number of sample formats. DO NOT USE if linking dynamically
};

enum class MuxType {
  ADTS, LATM, None
};

//�����������
enum class CodecType {
  AAC, MP3, PCMA, AMRWB, AMRNB
};


//��Ƶ��װ��Ϣ
struct CoderInfo {
  int inSampleRate;
  int inChannels;
  MyAVSampleFormat inFormate;
  int outSampleRate;
  int outChannels;
  MyAVSampleFormat outFormate;
  CodecType cdtype;                     //��������
  MuxType muxType;  //��װ����
};

