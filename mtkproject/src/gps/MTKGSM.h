#ifndef _MTKGSM_H_
#define _MTKGSM_H_

#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_uart.h"

#define EAT_UART_RX_BUF_LEN_MAX 2048
#define NMEA_BUFF_SIZE			1024

#define  CMD_AT						"AT\r\n"
#define  CMD_ATCPIN					"AT+CPIN?\r\n"
#define  CMD_ATCSQ					"AT+CSQ\r\n"
#define  CMD_ATCOPS					"AT+COPS?\r\n"
#define  CMD_CREGCHECK				"AT+CREG?\r\n"
#define  CMD_CREG_ON				"AT+CREG=1\r\n"
#define  CMD_IPSHUT					"AT+CIPSHUT\r\n"//关闭无线链路
#define  CMD_CIPMODE_ON				"AT+CIPMODE=1\r\n"//开启透传模式
#define	 CMD_CSTT_UNICOM			"AT+CSTT=\"3GNET\"\r\n"
#define  CMD_CSTT_MOBILE			"AT+CSTT=\"CMNET\"\r\n"
#define  CMD_CSTT_CHNCT				"AT+CSTT=\"CTNET\"\r\n"
#define  CMD_CIICR      			"AT+CIICR\r\n"//激活移动场景
#define  CMD_CIFSR					"AT+CIFSR\r\n"//获取本地IP
#define  CMD_CIPSTART				"AT+CIPSTART=\"TCP\",\"202.96.185.34\",\"2101\"\r\n"//建立TCP连接
#define  CMD_DOWNLOAD				"GET /0020008013 HTTP/1.0\r\nAuthorization: Basic ZHVtaW5oYW86emhkZ3Bz\r\n"//从挂载点下载差分数据

typedef enum{
	GSMBegin,						//启动GSM				
	GSMCheckSIM,					//检测SIM卡
	GSMCheckNet,					//检测网络注册情况
	GSMCheckSigQua,					//检测网络信号
	GSMCheckOperator,				//检测网络运营商
	GSMWireShut,					//关闭无线链路
	GSMEnTrans,						//开启透传模式
	GSMAttach,						//附着网络						
	GSMActiveMobile,				//激活移动场景					
	GSMGetLocalIP,					//获取本地IP						
	GSMConnectIP,					//建立TCP连接
	GSMLogServer,					//登录服务器			
	GSMGetMountPoint,				//获取挂载点
	GSMRecvDiffData,				//读取差分数据
	GSMSendDiffData,				//发送差分数据						
}GSMStateEnum_t;

typedef struct								
{
	char	 Mode[10];							//收发模式
	char	 IPAddress[20];						//IP地址
	char     ComNumber[10];						//端口号
	char	 MountPoint[20];					//ID值，即挂载点
	char   	 UserName[20];						//用户名
	char   	 PassWord[20];						//密码
	char   	 ConnectType[10];					//连接方式，TCP/UDP
}Network_tParam_t;

typedef enum{
	CHNCT,
	MOBILE,
	UNICOM,
}GSMOperatorEnum_t;
//static u8 rx_buf[15+1] = {0};
//static u8 tx_buf[EAT_UART_RX_BUF_LEN_MAX ] = {0};
int GSMCtrl();
int GSMWrite(unsigned char *write_buf, int write_len, long int time_out_ms);
int GSMRead(unsigned char *read_buf, int read_len, long int time_out_ms);
int GSMClose();

#endif
