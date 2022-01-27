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
			//1. 获取起始码，起始码有0x00 00 00 01 和0x00 00 01两种； 
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

			//2. 分析pkt中的数据，	
			if (pktSize - startLen <= MTU + 1) {
				//2.1 若长度不大于MTU，则去掉起始码后直接存入vector，push_data = 原NALU = nalu_header（1byte） + nalu_data
				uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * (pktSize - startLen));
				memcpy(buf, data + startLen, pktSize - startLen);
				outData.push_back(std::make_pair(buf, pktSize - startLen));
			}
			else {
				//2.2若长度大于MTU，则需要进行分段存入vector， push_data = FU_indicator(1byte) + FU_header(1byte) + nalu_data
				int index = startLen + 1; //去掉起始码和起始码后面的第一个字节(nalu header)
				bool isFirst = true;
				//static uint8_t buf[MTU + 2];
				//uint8_t buf[MTU + 2];

				uint8_t fuIndicator = 0xe0 & data[4]; //indicator由nalu第一个字节的高3位和5个bit的FUA type拼接而成
				fuIndicator |= 0x1c; //type为28
				//header 由S，E，R和nalu第一个字节的低5位拼接而成， S为1表示第一个分片，E为1表示最后一个分片，R为保留位，为0
				uint8_t fuHeader = 0x1f & data[4];
				do {
					uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * MTU + 2);
					if (isFirst) {  //如果是第一个分片，最高位置1， E和R置0；
						fuHeader |= 0x80;
						isFirst = false;
					}
					else {//如果是中间分片，则S，E，R都置0；
						fuHeader &= 0x1f;
					}
					memset(buf, 0, MTU + 2);
					memcpy(buf, &fuIndicator, 1);
					memcpy(buf + 1, &fuHeader, 1);
					memcpy(buf + 2, data + index, MTU);
					outData.push_back(std::make_pair(buf, MTU + 2));
					index += MTU;
				} while (index < pktSize - MTU);
				//最后一个分片，E置为1，S和R置为0；
				uint8_t* buf = (uint8_t*)malloc(sizeof(uint8_t) * MTU + 2);
				fuHeader &= 0x1f;
				fuHeader |= 0x40;
				memset(buf, 0, MTU + 2);
				memcpy(buf, &fuIndicator, 1);
				memcpy(buf + 1, &fuHeader, 1);
				memcpy(buf + 2, data + index, pktSize - index);
				outData.push_back(std::make_pair(buf, pktSize - index + 2));

			}

			//3. 判断是否需要发送sps
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

	uint8_t fu_ind = loaddata[0];  //分片单元包时，FU_indicator
	uint8_t fu_hdr = loaddata[1];  //分片单元包时，FU_header
	uint8_t nula_hdr = loaddata[0]; //单个单元包时，nalu_header

	//1. 取loadData中第一个byte的低5位（TYPE），判断loadData中的数据类型
	//1.1 TYPE = 24， loadData中是完整的NALU数据
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
	//1.2 0<TYPE<24, loadData中可能是sps数据
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
	//1.3 TYPE = 28， loadData中是NALU切片
	else if (((nula_hdr) & (0x1f)) == 28 && len >= 2) {
		//1.3.1 FU_header的S位=1，loadData中是第一个NALU分片
		if (((fu_hdr) >> 7) == 1) {
			position = 0;
			uint8_t F = (fu_ind) & (0x80);   //FU_indicator的最高位（1bit)
			uint8_t NRI = (fu_ind) & (0x60); //FU_indicator的第2、3位（2bit）
			uint8_t TYPE = (fu_hdr) & (0x1f); //FU_header的低5位（5bit）
			uint8_t nalu_header = F | NRI | TYPE;
			memcpy(&buff[position], &h264_startcode, 4);
			position += 4;
			memcpy(&buff[position], &nalu_header, 1);
			position += 1;
			memcpy(&buff[position], &loaddata[2], len - 2);
			position += len - 2;

		}
		//1.3.2 FU_header的S=0,E=1，loadData中是最后一个NALU分片
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
		//1.3.3 FU_header的S=0,E=0,R=0，loadData中是中间某个NALU分片
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
	//存储sps信息到sdpData
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