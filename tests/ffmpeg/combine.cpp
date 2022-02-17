//从数据的角度理解yuv拼接
#include <iostream>
#include <fstream>
using namespace std;
//首先进行全部Y分量的交错填充，然后全部U分量交错填充,然后为全部V分量交错填充.此时重点为UV分量的起始地址和填充的数据长度.获取出错的话可以在最终的画面上看到画面内存重复。
void horizontal_splice(FILE* pSrc1, FILE* pSrc2, FILE* pOut, int iLen, int iH, int iW)
{

	for (int Index = 0; Index < 200; Index++)
	{
		int iDuration = 0;
		for (int i = 0; i < iH; i++)
		{
			fread(pY, 1, iW, pSrc1);//y1
			memcpy(pBuf + iDuration, pY, iW);

			iDuration += iW;

			fread(p2Y, 1, iW, pSrc2);//y2
			memcpy(pBuf + iDuration, p2Y, iW);

			iDuration += iW;
		}
		for (int i = 0; i < iH / 2; i++)
		{
			fread(pU, 1, iW / 2, pSrc1);//u1
			memcpy(pBuf + iDuration, pU, iW / 2);

			iDuration += (iW / 2);

			fread(p2U, 1, iW / 2, pSrc2);//u2
			memcpy(pBuf + iDuration, p2U, iW / 2);

			iDuration += (iW / 2);
		}
		for (int i = 0; i < iH / 2; i++)
		{
			fread(pV, 1, iW / 2, pSrc1);//v1
			memcpy(pBuf + iDuration, pV, iW / 2);

			iDuration += (iW / 2);

			fread(p2V, 1, iW / 2, pSrc2);//v2
			memcpy(pBuf + iDuration, p2V, iW / 2);

			iDuration += (iW / 2);
		}
		fwrite(pBuf, 1, iDuration, pOut);
	}
}
