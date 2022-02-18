#include<stdio.h>  
#include<stdlib.h>  
#include<string.h>  
#define PREWEIGHT 1920  
#define PREHEIGHT 1080  
#define RESWEIGHT 3840  
#define RESHEIGHT 1080  
#define PREYSIZE ((PREWEIGHT)*(PREHEIGHT))  
#define PREUSIZE ((PREWEIGHT/2)*(PREHEIGHT/2))  
#define PREVSIZE ((PREWEIGHT/2)*(PREHEIGHT/2))  
#define RESYSIZE ((RESWEIGHT)*(RESHEIGHT))  
#define RESUSIZE ((RESWEIGHT/2)*(RESHEIGHT/2))  
#define RESVSIZE ((RESWEIGHT/2)*(RESHEIGHT/2))  
#define PRESIZE ((PREYSIZE)+(PREUSIZE)+(PREVSIZE))  
#define RESSIZE ((RESYSIZE)+(RESUSIZE)+(RESVSIZE))  

int GetFrameNum(const char* File)
{
  FILE* fp;
  int size = 0;
  if (!(fp = fopen(File, "rb")))
  {
    printf("Open %s error !", File);
    exit(1);
  }
  else
  {
    fseek(fp, 0, SEEK_END);/*将文件指针移到YUV文件的末尾*/
    size = ftell(fp);/*计算文件的总大小*/
  }
  return (size / PRESIZE);
}
void ReadYUV(char* ResBuf, char* PreBuf, int resstart, int prestart, int resoffset, int preoffset, int size, int height)
{
  int k;
  for (k = 0; k < height; k++)
  {
    memmove(ResBuf + resstart + k * (resoffset), PreBuf + prestart + k * (preoffset), size);//注意这里用memmov不用strncpy  
  }
}
int main(int argc, char* argv[])
{
  const char* FileName[] = { "e:\BMX_L_1920x1080_240frms.yuv","e:\BMX_R_1920x1080_240frms.yuv" };/*两路YUV文件名*/
  FILE* FileResult;/*输出文件名*/
  FILE** fp_combine = (FILE**)malloc(sizeof(FILE*) * 3);/*申请文件指针*/
  int* FileFrameNum = (int*)malloc(sizeof(int) * 3);/*每个YUV的帧数*/
  char* PreBuf = (char*)malloc(sizeof(char) * (PRESIZE + 1));/*处理前每一帧图像的大小*/
  char* ResBuf = (char*)malloc(sizeof(char) * (RESSIZE + 1));/*处理后每一帧图像的大小*/
  int Y_start_section = 0;/*预处理图片Y分量存入目标区域的起始区域*/
  int U_start_section = 0;/*预处理图片U分量存入目标区域的起始区域*/
  int V_start_section = 0;/*预处理图片V分量存入目标区域的起始区域*/
  int File_offset = 0;/*预处理文件偏移值*/
  int i_combine = 0, j_combine = 0, k_combine = 0;/*控制循环*/
  /*判断申请内存是否成功*/
  if (!((fp_combine) && (FileFrameNum) && (PreBuf) && (ResBuf)))
  {
    printf("Allocate memeroy Faile !");
    exit(1);
  }
  /*初始化申请空间*/
  memset(fp_combine, 0, sizeof(FILE*) * 2);
  memset(FileFrameNum, 0, sizeof(int) * 2);
  memset(PreBuf, 0, sizeof(char) * PRESIZE);
  memset(ResBuf, 0, sizeof(char) * RESSIZE);
  if (!(FileResult = fopen("hua_result.YUV", "wb")))/*创建输出文件*/
  {
    printf("Creat File faile !");
    exit(1);
  }
  for (i_combine = 0; i_combine < 2; i_combine++)
  {
    if (!(fp_combine[i_combine] = fopen(FileName[i_combine], "rb")))/*打开输入文件*/
    {
      printf("Open File %s Faile !", FileName[i_combine]);
      exit(1);
    }
    else
    {
      FileFrameNum[i_combine] = GetFrameNum(FileName[i_combine]);/*存储每一个视频的帧数*/
    }
  }
  i_combine = 0;
  k_combine = FileFrameNum[i_combine];
  while (i_combine < k_combine)
  {
    File_offset = i_combine * PRESIZE;
    j_combine = 0;
    while (j_combine < 2)
    {
      fseek(fp_combine[j_combine], File_offset, SEEK_SET);/*移动文件指针至需要处理的数据的位置*/
      fread(PreBuf, 1, PRESIZE, fp_combine[j_combine]);/*读取一幅图像*/
      if (j_combine == 0)
      {
        /*把读取预处理图片Y/U/V分量的起始位置放置目标对应位置*/
        Y_start_section = 0;
        U_start_section = RESYSIZE;
        V_start_section = RESYSIZE + RESUSIZE;
      }
      else
      {
        /*把读取预处理图片Y/U/V分量的起始位置放置目标对应位置*/
        Y_start_section = PREWEIGHT;
        U_start_section = RESYSIZE + PREWEIGHT / 2;
        V_start_section = RESYSIZE + RESUSIZE + PREWEIGHT / 2;
      }
      /*分别读Y、U、V*/
      ReadYUV(ResBuf, PreBuf, Y_start_section, 0, RESWEIGHT, PREWEIGHT, PREWEIGHT, PREHEIGHT);
      ReadYUV(ResBuf, PreBuf, U_start_section, PREYSIZE, RESWEIGHT / 2, PREWEIGHT / 2, PREWEIGHT / 2, PREHEIGHT / 2);
      ReadYUV(ResBuf, PreBuf, V_start_section, PREYSIZE + PREUSIZE, RESWEIGHT / 2, PREWEIGHT / 2, PREWEIGHT / 2, PREHEIGHT / 2);
      j_combine++;
    }
    fwrite(ResBuf, 1, RESSIZE, FileResult);
    fflush(FileResult);
    i_combine++;
  }
  fclose(fp_combine[0]);
  fclose(fp_combine[1]);
  fclose(FileResult);
  return 0;
}