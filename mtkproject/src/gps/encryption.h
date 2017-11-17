#ifndef _ENCRYPTION_H
#define _ENCRYPTION_H

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "eat_interface.h"

#define DEVICELICENSE		"L9D7Y6E6N2F6K518887U9D2A"//注册码

int Generate_EncryptionID(unsigned char* DeviceSerialNum);
int Decode_EncryptionID(unsigned char *ExpirationTime);

#endif 

