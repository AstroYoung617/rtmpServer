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
    fseek(fp, 0, SEEK_END);/*���ļ�ָ���Ƶ�YUV�ļ���ĩβ*/
    size = ftell(fp);/*�����ļ����ܴ�С*/
  }
  return (size / PRESIZE);
}
void ReadYUV(char* ResBuf, char* PreBuf, int resstart, int prestart, int resoffset, int preoffset, int size, int height)
{
  int k;
  for (k = 0; k < height; k++)
  {
    memmove(ResBuf + resstart + k * (resoffset), PreBuf + prestart + k * (preoffset), size);//ע��������memmov����strncpy  
  }
}
int main(int argc, char* argv[])
{
  const char* FileName[] = { "e:\BMX_L_1920x1080_240frms.yuv","e:\BMX_R_1920x1080_240frms.yuv" };/*��·YUV�ļ���*/
  FILE* FileResult;/*����ļ���*/
  FILE** fp_combine = (FILE**)malloc(sizeof(FILE*) * 3);/*�����ļ�ָ��*/
  int* FileFrameNum = (int*)malloc(sizeof(int) * 3);/*ÿ��YUV��֡��*/
  char* PreBuf = (char*)malloc(sizeof(char) * (PRESIZE + 1));/*����ǰÿһ֡ͼ��Ĵ�С*/
  char* ResBuf = (char*)malloc(sizeof(char) * (RESSIZE + 1));/*�����ÿһ֡ͼ��Ĵ�С*/
  int Y_start_section = 0;/*Ԥ����ͼƬY��������Ŀ���������ʼ����*/
  int U_start_section = 0;/*Ԥ����ͼƬU��������Ŀ���������ʼ����*/
  int V_start_section = 0;/*Ԥ����ͼƬV��������Ŀ���������ʼ����*/
  int File_offset = 0;/*Ԥ�����ļ�ƫ��ֵ*/
  int i_combine = 0, j_combine = 0, k_combine = 0;/*����ѭ��*/
  /*�ж������ڴ��Ƿ�ɹ�*/
  if (!((fp_combine) && (FileFrameNum) && (PreBuf) && (ResBuf)))
  {
    printf("Allocate memeroy Faile !");
    exit(1);
  }
  /*��ʼ������ռ�*/
  memset(fp_combine, 0, sizeof(FILE*) * 2);
  memset(FileFrameNum, 0, sizeof(int) * 2);
  memset(PreBuf, 0, sizeof(char) * PRESIZE);
  memset(ResBuf, 0, sizeof(char) * RESSIZE);
  if (!(FileResult = fopen("hua_result.YUV", "wb")))/*��������ļ�*/
  {
    printf("Creat File faile !");
    exit(1);
  }
  for (i_combine = 0; i_combine < 2; i_combine++)
  {
    if (!(fp_combine[i_combine] = fopen(FileName[i_combine], "rb")))/*�������ļ�*/
    {
      printf("Open File %s Faile !", FileName[i_combine]);
      exit(1);
    }
    else
    {
      FileFrameNum[i_combine] = GetFrameNum(FileName[i_combine]);/*�洢ÿһ����Ƶ��֡��*/
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
      fseek(fp_combine[j_combine], File_offset, SEEK_SET);/*�ƶ��ļ�ָ������Ҫ��������ݵ�λ��*/
      fread(PreBuf, 1, PRESIZE, fp_combine[j_combine]);/*��ȡһ��ͼ��*/
      if (j_combine == 0)
      {
        /*�Ѷ�ȡԤ����ͼƬY/U/V��������ʼλ�÷���Ŀ���Ӧλ��*/
        Y_start_section = 0;
        U_start_section = RESYSIZE;
        V_start_section = RESYSIZE + RESUSIZE;
      }
      else
      {
        /*�Ѷ�ȡԤ����ͼƬY/U/V��������ʼλ�÷���Ŀ���Ӧλ��*/
        Y_start_section = PREWEIGHT;
        U_start_section = RESYSIZE + PREWEIGHT / 2;
        V_start_section = RESYSIZE + RESUSIZE + PREWEIGHT / 2;
      }
      /*�ֱ��Y��U��V*/
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