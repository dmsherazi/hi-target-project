/********************************************************************
 *                Copyright Simcom(shanghai)co. Ltd.                   *
 *---------------------------------------------------------------------
 * FileName      :   app_demo_uart.c
 * version       :   0.10
 * Description   :   
 * Authors       :   Maobin
 * Notes         :
 *---------------------------------------------------------------------
 *
 *    HISTORY OF CHANGES
 *---------------------------------------------------------------------
 *0.10  2012-09-24, Maobin, create originally.
 *
 *--------------------------------------------------------------------
 *  Description:
 *   UART1: app channel. 
 *   UART2: Debug port
 *   UART3: AT command channel
 *          In this example,APP gets AT cmd data from UART3,and sends it to Modem,
 *          then gets response message from Modem and sends to UART3.
 *  Data flow graph:
 *         "AT" 
 *   UART1 --->APP ---> Modem AT
 *     |             "OK" |
 *      <---- APP <-------
 *
 *            "AT"
 *   UART3 <=======> Modem AT
 *            "OK"
 **************************************************************************/

/******************************************************************************
 *  Include Files
 ******************************************************************************/
#include "eat_modem.h"
#include "eat_interface.h"
#include "eat_periphery.h"
#include "eat_uart.h"
#include "eat_clib_define.h" //only in main.c
#include "eat_gps.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
//#include "MTKGSM.h"
//#include "ENCodeDecode.h"
//#include "getrawdata.h"
//#include "encryption.h"

//#include "app_demo_gps.h"
/********************************************************************
 * Macros
 ********************************************************************/
#define EAT_AT_TEST
#define EAT_DEBUG_STRING_INFO_MODE  1 //output debug string info to debug port
#define EAT_DEBUG_STRING_INFO_MODE
#define EAT_UART_RX_BUF_LEN_MAX  2048
#define NMEA_BUFF_SIZE 100

#define  CMD_ATCMD_PWRON 		"AT+Cgnspwr=1\r\n"
#define  CMD_ATCMD_STST			"AT+CGNSTST=1\r\n"
#define  CMD_ATCMD_EPH			"AT+CGNSCMD=0,\"$PMTK669,3*24\"\r\n"
#define  CMD_AT					"AT\r\n"
/********************************************************************
 * Types
 ********************************************************************/
typedef void (*app_user_func)(void*);

/********************************************************************
 * Extern Variables (Extern /Global)
 ********************************************************************/
 
/********************************************************************
 * Local Variables:  STATIC
 ********************************************************************/
static const EatUart_enum eat_uart_app= EAT_UART_NULL;
static const EatUart_enum eat_uart_debug = EAT_UART_USB;
static const EatUart_enum eat_uart_at = EAT_UART_1;
static u16 wr_uart_offset = 0;
static u16 wr_uart_len = 0;
static char gps_info_buf[NMEA_BUFF_SIZE]="";
//u8 rx_buf[2048];
//extern unsigned char msg[256];
unsigned char uart_rcv[2048];
unsigned char eph_rcv[2048];

/********************************************************************
 * External Functions declaration
 ********************************************************************/
extern void APP_InitRegions(void);

/********************************************************************
 * Local Function declaration
 ********************************************************************/
void app_main(void *data);
void app_user1(void *data);
void app_user2(void *data);
void app_func_ext1(void *data);

/********************************************************************
 * Local Function
 ********************************************************************/
#pragma arm section rodata = "APP_CFG"
APP_ENTRY_FLAG 
#pragma arm section rodata


#pragma arm section rodata="APPENTRY"
	const EatEntry_st AppEntry = 
	{
		app_main,
		app_func_ext1,
		(app_user_func)EAT_NULL,//app_user1,
		(app_user_func)EAT_NULL,//app_user2,
		(app_user_func)EAT_NULL,//app_user3,
		(app_user_func)EAT_NULL,//app_user4,
		(app_user_func)EAT_NULL,//app_user5,
		(app_user_func)EAT_NULL,//app_user6,
		(app_user_func)EAT_NULL,//app_user7,
		(app_user_func)EAT_NULL,//app_user8,
		EAT_NULL,
		EAT_NULL,
		EAT_NULL,
		EAT_NULL,
		EAT_NULL,
		EAT_NULL
	};
#pragma arm section rodata

/***************************************************************************
 * Local Functions
 ***************************************************************************/
static void app_func_ext1(void *data)
{
#ifdef EAT_DEBUG_STRING_INFO_MODE
    EatUartConfig_st cfg =
    {
        EAT_UART_BAUD_115200,
        EAT_UART_DATA_BITS_8,
        EAT_UART_STOP_BITS_1,
        EAT_UART_PARITY_NONE
    };
#endif
#if 1
	eat_uart_set_debug(EAT_UART_USB);
    eat_uart_set_at_port(EAT_UART_1);   
	//eat_uart_set_config(EAT_UART_1,&cfg);
	//eat_uart_set_debug_config(EAT_UART_DEBUG_MODE_UART, NULL);
#else
 	eat_uart_set_at_port(EAT_UART_NULL); 
	eat_uart_set_debug(EAT_UART_1);
#endif
//  set debug string info to debug port
/*#ifdef EAT_DEBUG_STRING_INFO_MODE
    if( EAT_UART_USB == eat_uart_debug)
    {
		eat_uart_set_debug_config(EAT_UART_DEBUG_MODE_UART, &cfg);
    }else
    {
        eat_uart_set_debug_config(EAT_UART_DEBUG_MODE_UART, &cfg);
    }
#endif*/

}

static void app_main(void *data)
{	
	EatEvent_st event;
//	queue_buf_t rawdata_q;
    u16 satnum = 0;
	int res = 0;
	
//	mtkraw_t rawdata_t;
	unsigned char EncryptedTime[10];
	
	u32 rcv_len = 0,temp = 0,buf_len = 0;
	char flag = 0;
	APP_InitRegions();//Init app RAM, first step
    APP_init_clib(); //C library initialize, second step	
	//queue_buf_Init(&rawdata_q,2048,sizeof(char));
	eat_trace("#########################");
  	while(1)
    {	
		switch(flag)
		{
			case 0:
				eat_modem_write(CMD_AT,strlen(CMD_AT));
				memset(uart_rcv,0,sizeof(uart_rcv));
				rcv_len = eat_modem_read(uart_rcv,1024);
				
				if(rcv_len > 0){
					if(strstr(uart_rcv,"OK") != NULL){
						eat_trace("the module work normal");
						flag = 1;
					}
				}
				eat_trace("### ## case0 uart_rcv is     %s    ",uart_rcv);
			break;
			case 1:
				eat_modem_write(CMD_ATCMD_PWRON,strlen(CMD_ATCMD_PWRON));
				memset(uart_rcv,0,sizeof(uart_rcv));
				rcv_len = eat_modem_read(uart_rcv,1024);
				eat_trace("### ## case1 uart_rcv is     %s    ",uart_rcv);
				if(rcv_len > 0){
					if(strstr(uart_rcv,"OK") != NULL){
						eat_trace("the module GPS power normal");
						flag = 2;
					}
				}
				
			break;
			case 2:
				eat_modem_write(CMD_ATCMD_STST,strlen(CMD_ATCMD_STST));
				memset(uart_rcv,0,sizeof(uart_rcv));
				rcv_len = eat_modem_read(uart_rcv,1024);
				
				if(rcv_len > 0){
					if(strstr(uart_rcv,"OK") != NULL){
						eat_trace("the module send NMEA data to uart1 normal");
						flag = 3;
					}
				}
				eat_trace("### ## case2 uart_rcv is     %s    ",uart_rcv);
				eat_modem_write(CMD_ATCMD_EPH,strlen(CMD_ATCMD_EPH));
			break;
			case 3:
				
				memset(uart_rcv,0,sizeof(uart_rcv));
				rcv_len = eat_modem_read(uart_rcv,1024);
				//buf_len = eat_uart_read(EAT_UART_1,eph_rcv,1024);
				//buf_len = eat_modem_read(eph_rcv,1024);
				eat_trace("### ## case3 the eph data is     %s    ",uart_rcv);
				/*
				if(rcv_len > 0){
					if(strstr(uart_rcv,"OK") != NULL){
						eat_trace("the module get eph data normal");
						eat_trace("### ##the eph data is %s",eph_rcv);
					}
				}*/
			break;
				
		}
		eat_sleep(1000);
	#if 0
		eat_modem_write(CMD_ATCMD_PWRON,strlen(CMD_ATCMD_PWRON));
		memset(rx_buf,0,sizeof(rx_buf));
		eat_sleep(4000);
		buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
		rx_buf[buf_len] = '\0';
		eat_trace("### case0 eat_modem_rea=  %s ",rx_buf);
		if(strstr(rx_buf,"OK") != NULL){
						eat_trace("GNSS power on ok len is %d\r\n",buf_len);
						
						eat_sleep(1000);

						
				        eat_modem_write(CMD_ATCMD_STST,strlen(CMD_ATCMD_STST));
						memset(rx_buf,0,sizeof(rx_buf));
						buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
					rx_buf[buf_len] = '\0';
					//eat_uart_write(EAT_UART_1,rx_buf,buf_len);
					eat_trace("### case2 eat_modem_rea=  %s ",rx_buf);
					if(strstr(rx_buf,"OK") != NULL){
						eat_trace("GNSS out raw data on ok len is %d\r\n",buf_len);
						//flag = 1;
						eat_sleep(1000);


						while(1)
							{
						eat_modem_write(CMD_ATCMD_EPH,strlen(CMD_ATCMD_EPH));
						memset(rx_buf,0,sizeof(rx_buf));
						buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
						//buf_len = eat_uart_read(EAT_UART_1,rx_buf,EAT_UART_RX_BUF_LEN_MAX);
						rx_buf[buf_len] = '\0';
						//eat_uart_write(EAT_UART_1,rx_buf,buf_len);
						eat_trace("### case1 eat_modem_rea=  %s ",rx_buf);
						if(strstr(rx_buf,"OK") != NULL){
							eat_trace("GNSS stst ok len is %d \r\n",buf_len);
							//flag = 1;
							eat_sleep(2000);
						}

						
					}
	                }
		}
		eat_sleep(1000);
		#endif
    #if 0
    	switch(flag)
    	{
			case 0:
				temp = MTK_GetRawData();
				if(temp > 0) 
					flag = 2;
				break;
			case 1:
				temp = send_cmd_ephe("06",SYS_BDS);
			//	temp = eat_modem_write(CMD_ATEPHDATA,strlen(CMD_ATEPHDATA));
				if(temp > 0){
					eat_trace("!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
					flag =2;
				eat_sleep(200);
				}
				break;
			case 2:
				memset(uart_rcv,0,sizeof(uart_rcv));
				do{
				//	temp = eat_modem_write(CMD_ATEPHDATA,strlen(CMD_ATEPHDATA));
					temp = send_cmd_ephe("06",SYS_BDS);
					rcv_len = eat_uart_read(EAT_UART_2,uart_rcv,1024);
					if(rcv_len > 0){
						eat_uart_write(EAT_UART_1,uart_rcv,rcv_len);
					//	eat_sleep(200);
					}
					#if 0
						g_bdgsv_count = 0;
						g_gpgsv_count = 0;
						queue_buf_Output(&rawdata_q,NULL,rawdata_q.size);//初始化队列内部数组
						queue_buf_Input(&rawdata_q,uart_rcv,rcv_len);//将rawdata_buf数据存入队列
						while(rawdata_q.size > 0 && rcv_len > 0)
						{
							res = GetOneMsg(&rawdata_q,msg,rcv_len);//将队列内数组解析为一条语句
							if(res < 0){
								eat_trace("dmh:error:the msg is not complete.");
								break; 
							}
							res = DecodeMTKmsg(msg,res, &rawdata_t);
							eat_sleep(100);
						}	
						if(StartLockFlag > 0){
								res = MTK_GetEphedata(BDS_satnum[satnum]);
								if(res > 0){
									eat_trace("dmh:send cmd to request the satnum[%d]:%s,eph data.",satnum,BDS_satnum[satnum]);
									satnum ++;
									if(satnum > sizeof(*BDS_satnum))
										satnum = 0;
								}
								else
									eat_trace("dmh:send cmd to request the eph data fail####");
							}	
						#endif
				}while(rcv_len == RAWDATA_BUF_LEN);
					//	flag = 1;
				break;
		}
		#if 0//加密程序
		if(!Decode_EncryptionID(EncryptedTime)){
			eat_trace("dmh:the time is %s",EncryptedTime);
			return;
		}	
		eat_trace("################################");
		return;
		#endif
		#endif 
    }
	//eat_sleep(100);
}

void app_user1(void *data)
{
	unsigned char user1_buf[1024];
	unsigned char user1_buflen = 0;
//	eat_trace("here is the user1 app ^^^^^^^^^^^^^^^^^^^^^^^^");
	while(1)
	{

	}
}

