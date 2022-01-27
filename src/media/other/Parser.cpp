#include <media/other/Parser.h>
#include <other/loggerApi.h>
#include <iostream>
#include <vector>


Parser::Parser() {
	I_LOG("parser start");
	buff = new uint8_t[1024 * 1024];
}

Parser::~Parser() {
	I_LOG("parser clean");
	if (buff) {
		free(buff);
	}
	/*while (!output_h264Data.empty()) {
		free(std::get<0>(output_h264Data.front()));
		output_h264Data.pop();
	}
	std::queue<std::tuple<uint8_t*, size_t, uint64_t>> empty;
	swap(empty, output_h264Data);*/
}

void Parser::H264Parse2NALU(AVPacket* input_pkt, std::vector<std::pair<uint8_t*, int>>& outData) {
	//todo
	try
	{
		auto data = input_pkt->data;
		auto pktSize = input_pkt->size;
		if (pktSize > 0) {
			//1. ��ȡ��ʼ�룬��ʼ����0x00 00 00 01 ��0x00 00 01���֣� 
			int startLen = 0;
			uint32_t startCode = 0;
			for (int i = 0; i < 4; ++i) {
				startCode = (startCode << 8) | data[i];
			}
			if (startCode != 1) {
				I_LOG("startCode: {}", (int)(startCode & 0x0000001f));
				if ((startCode & 0x00000f00) == 16) {
					startLen = 3;
				}
				else {
					throw std::runtime_error("startCode error");
				}
			}
			else {
				startLen = 4;
			}

			//2. ����pkt�е����ݣ�	
			if (pktSize - startLen <= MTU + 1) {
				//2.1 �����Ȳ�����MTU����ȥ����ʼ���ֱ�Ӵ���vector��push_data = ԭNALU = nalu_header��1byte�� + nalu_data
				uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * (pktSize - startLen));
				memcpy(buf, data + startLen, pktSize - startLen);
				outData.push_back(std::make_pair(buf, pktSize - startLen));
			}
			else {
				//2.2�����ȴ���MTU������Ҫ���зֶδ���vector�� push_data = FU_indicator(1byte) + FU_header(1byte) + nalu_data
				int index = startLen + 1; //ȥ����ʼ�����ʼ�����ĵ�һ���ֽ�(nalu header)
				bool isFirst = true;
				//static uint8_t buf[MTU + 2];
				//uint8_t buf[MTU + 2];

				uint8_t fuIndicator = 0xe0 & data[4]; //indicator��nalu��һ���ֽڵĸ�3λ��5��bit��FUA typeƴ�Ӷ���
				fuIndicator |= 0x1c; //typeΪ28
				//header ��S��E��R��nalu��һ���ֽڵĵ�5λƴ�Ӷ��ɣ� SΪ1��ʾ��һ����Ƭ��EΪ1��ʾ���һ����Ƭ��RΪ����λ��Ϊ0
				uint8_t fuHeader = 0x1f & data[4];
				do {
					uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * MTU + 2);
					if (isFirst) {  //����ǵ�һ����Ƭ�����λ��1�� E��R��0��
						fuHeader |= 0x80;
						isFirst = false;
					}
					else {//������м��Ƭ����S��E��R����0��
						fuHeader &= 0x1f;
					}
					memset(buf, 0, MTU + 2);
					memcpy(buf, &fuIndicator, 1);
					memcpy(buf + 1, &fuHeader, 1);
					memcpy(buf + 2, data + index, MTU);
					outData.push_back(std::make_pair(buf, MTU + 2));
					index += MTU;
				} while (index < pktSize - MTU);
				//���һ����Ƭ��E��Ϊ1��S��R��Ϊ0��
				uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * MTU + 2);
				fuHeader &= 0x1f;
				fuHeader |= 0x40;
				memset(buf, 0, MTU + 2);
				memcpy(buf, &fuIndicator, 1);
				memcpy(buf + 1, &fuHeader, 1);
				memcpy(buf + 2, data + index, pktSize - index);
				outData.push_back(std::make_pair(buf, pktSize - index + 2));

			}

			//3. �ж��Ƿ���Ҫ����sps
			//if (frameCount % SPS_INTERVAL == 0 || needUpdate) {
			//	outData.push_back(std::make_pair(spsData, spsLen));
			//	needUpdate = false;
			//}
			++frameCount;
		}
		else {
			std::cout << "input_pkt is empty" << std::endl;
		}
	}
	catch (...)
	{
		E_LOG("H264Parse2NALU fun error");
	}

}



void Parser::inputPayLoad(uint8_t* loaddata, int len, uint64_t timestamp) {

	uint8_t fu_ind = loaddata[0];  //��Ƭ��Ԫ��ʱ��FU_indicator
	uint8_t fu_hdr = loaddata[1];  //��Ƭ��Ԫ��ʱ��FU_header
	uint8_t nula_hdr = loaddata[0]; //������Ԫ��ʱ��nalu_header

	//1. ȡloadData�е�һ��byte�ĵ�5λ��TYPE�����ж�loadData�е���������
	//1.1 TYPE = 24�� loadData����������NALU����
	if (((nula_hdr) & (0x1f)) == 24) {
		position = 0;
		memcpy(&buff[position], &h264_startcode, 4);
		position += 4;
		uint16_t size1 = ((uint16_t)loaddata[1] << 8) | ((uint16_t)loaddata[2]);
		memcpy(&buff[position], &loaddata[3], (size_t)size1);
		position += size1;
		memcpy(&buff[position], &h264_startcode, 4);
		position += 4;
		uint16_t size2 = ((uint16_t)loaddata[3 + size1] << 8) | ((uint16_t)loaddata[4 + size1]);
		memcpy(&buff[position], &loaddata[5 + size1], (size_t)size2);
		position += size2;
		int outSize = position;
		output_h264Data.push(std::make_tuple(buff, outSize, timestamp));
		position = 0;

	}
	//1.2 0<TYPE<24, loadData�п�����sps����
	else if (((nula_hdr) & (0x1f)) > 0 && ((nula_hdr) & (0x1f)) < 24) {
		position = 0;
		uint8_t TYPE = (nula_hdr) & (0x1f);
		memcpy(&buff[position], &h264_startcode, 4);
		position += 4;
		memcpy(&buff[position], loaddata, len);
		position += len;
		int outSize = position;
		output_h264Data.push(std::make_tuple(buff, outSize, timestamp));
		position = 0;

	}
	//1.3 TYPE = 28�� loadData����NALU��Ƭ
	else if (((nula_hdr) & (0x1f)) == 28 && len >= 2) {
		//1.3.1 FU_header��Sλ=1��loadData���ǵ�һ��NALU��Ƭ
		if (((fu_hdr) >> 7) == 1) {
			position = 0;
			uint8_t F = (fu_ind) & (0x80);   //FU_indicator�����λ��1bit)
			uint8_t NRI = (fu_ind) & (0x60); //FU_indicator�ĵ�2��3λ��2bit��
			uint8_t TYPE = (fu_hdr) & (0x1f); //FU_header�ĵ�5λ��5bit��
			uint8_t nalu_header = F | NRI | TYPE;
			memcpy(&buff[position], &h264_startcode, 4);
			position += 4;
			memcpy(&buff[position], &nalu_header, 1);
			position += 1;
			memcpy(&buff[position], &loaddata[2], len - 2);
			position += len - 2;

		}
		//1.3.2 FU_header��S=0,E=1��loadData�������һ��NALU��Ƭ
		else if (((fu_hdr) >> 6) == 1) {
			if (position > 1024 * 1024) {
				E_LOG("position is too big, maybe some package loss");
				return;
			}
			memcpy(&buff[position], &loaddata[2], len - 2);
			position += len - 2;
			// I_LOG("outSize {}", (int)len);
			//int outSize = len - 2;
			//uint8_t* outData = new uint8_t[position];
			//memcpy(outData, buff, position);
			output_h264Data.push(std::make_tuple(buff, position, timestamp));
			position = 0;
		}
		//1.3.3 FU_header��S=0,E=0,R=0��loadData�����м�ĳ��NALU��Ƭ
		else if (((fu_hdr) >> 5) == 0) {
			if (position > 1024 * 1024) {
				E_LOG("position is too big, maybe some package loss");
				return;
			}
			memcpy(&buff[position], &loaddata[2], len - 2);
			position += len - 2;
		}
	}
}


int Parser::getH264(uint8_t* h264Data, uint64_t& timestamp) {
	//todo, edit by xusiran 10.20: return 1
	size_t len = 0;
	if (output_h264Data.empty()) {
		return 0;
	}
	else {
		std::tuple<uint8_t*, size_t, uint64_t> firstData = output_h264Data.front();
		len = std::get<1>(firstData);
		memcpy(h264Data, std::get<0>(firstData), len);
		output_h264Data.pop();
		return len;
	}
}


void Parser::updateSpsMsg(uint8_t* sps, size_t spsSize) {
	//�洢sps��Ϣ��sdpData
	std::cout << "sps size: " << spsSize << std::endl;
	if (spsSize > 0) {
		if (spsSize > SPS_LEN) {
			if (spsData) {
				delete spsData;
				spsData = nullptr;
			}
			try {
				spsData = new uint8_t[spsSize];
			}
			catch (...) {
				std::cout << "sps data need " << spsSize << " byte, allocate error" << std::endl;
				return;
			}
		}
		if (!spsData) {
			std::cout << "sps data is empty" << std::endl;
			return;
		}
		memcpy(spsData, sps, spsSize);
		spsLen = spsSize;
		free(sps);
		needUpdate = true;
	}
	else {
		std::cout << "sps data is null" << std::endl;
	}
}