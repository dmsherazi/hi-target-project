#ifndef _ENCODEDECODE_H
#define _ENCODEDECODE_H
#include <stdio.h>   
#include <stdlib.h>   
#include <string.h> 

//#define U16 unsigned short
//#define U8 unsigned char
//typedef unsigned char byte; 
// 计算给定长度数据的16 位CRC。
//unsigned short GetCrc16(char* pData, int nLength);

//void GetPosition (char c, int *x, int *y);  

//char GetKey (int x, int y,char type);  

//void insert_c(char *s, char x, int location);

//void del_positionchar(char *str, int n);

//void del_char(char *str, char c);

//void Encrypt (char *inputdata,char *time, char *output);

//void byteToHexStr(char byte_arr,char hex_str[]);  

void compress(char *HardID,char *hardid,char *platform);

void SoftwareID_encode(char *HardID,char softwareIdentification,char softwaremodule,char platform,char *code);

void Decrypt (char *input,char *output_decode); 
 
 #endif