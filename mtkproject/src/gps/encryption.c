#include "encryption.h"

int Generate_EncryptionID(unsigned char* DeviceSerialNum)//生成序列号
{
	unsigned char ChipIDBuf[16];
	unsigned char SequenceNum[20];
	u16 buf_len = 0;
	buf_len = eat_get_chipid(ChipIDBuf,sizeof(ChipIDBuf));//获取芯片ID
    if(buf_len > sizeof(ChipIDBuf))
		return -1;
	compress(ChipIDBuf,SequenceNum,"M");//压缩芯片ID为7位
	eat_trace("dmh: the Sequence Num is %x,or %s",SequenceNum,SequenceNum);
	SoftwareID_encode(SequenceNum,48,0,'M',DeviceSerialNum);//生成序列号
	eat_trace("dmh:the last sequence num is %s",DeviceSerialNum);
	return 0;
}

int Decode_EncryptionID(unsigned char *ExpirationTime)//根据注册码解码7位ID
{
	char *DeviceBuf;
	char *DeviceInfo[10];
	unsigned char Sequence_temp[10];
	unsigned char ChipIDBuf[16];
	unsigned char SequenceNum[10];
	unsigned char SerialNum[20];
	u16 buf_len = 0;
	u16 i = 0;
	buf_len = eat_get_chipid(ChipIDBuf,sizeof(ChipIDBuf));//获取芯片ID
    if(buf_len > sizeof(ChipIDBuf))
		return;
	compress(ChipIDBuf,SequenceNum,"M");//压缩芯片ID为7位
	strcpy(Sequence_temp,SequenceNum);
	Decrypt(DEVICELICENSE,SerialNum);//由注册码解析出压缩ID
	DeviceBuf = strtok(SerialNum,"$");
	while(DeviceBuf){
		DeviceInfo[i] = DeviceBuf;
		DeviceBuf = strtok(NULL,"$");
		i++;
	}
	if(!strcmp(DeviceInfo[0],Sequence_temp)){
		strcpy(ExpirationTime,DeviceInfo[3]);
		return 0;
	}
	return 1;
}

