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
#include "MTKGSM.h"
#include "ENCodeDecode.h"
#include "getrawdata.h"
#include "encryption.h"

//#include "app_demo_gps.h"
/********************************************************************
 * Macros
 ********************************************************************/
#define EAT_AT_TEST
#define EAT_DEBUG_STRING_INFO_MODE  1 //output debug string info to debug port
#define EAT_DEBUG_STRING_INFO_MODE
#define EAT_UART_RX_BUF_LEN_MAX  (1024*4)
#define NMEA_BUFF_SIZE 100

#define  CMD_ATCGNSPWR_ON2 		"AT+CGPIO=0,57,1,1\r\n"
#define  CMD_ATCMD				"AT+CGNSAID=31,1,1\r\n"
#define  CMD_EPHEMERIS2			"AT+CGNSCMD=0,\"$PMTK669,06*11\"\r\n"

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
static const EatUart_enum eat_uart_app= EAT_UART_2;
static const EatUart_enum eat_uart_debug = EAT_UART_USB;
static const EatUart_enum eat_uart_at = EAT_UART_1;
static u16 wr_uart_offset = 0;
static u16 wr_uart_len = 0;
static char gps_info_buf[NMEA_BUFF_SIZE]="";
u8 rx_buf[2048];
//extern unsigned char msg[256];
unsigned char uart_rcv[EAT_UART_RX_BUF_LEN_MAX];

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
#if 0
    eat_uart_set_at_port(EAT_UART_1);    
	eat_uart_set_debug(eat_uart_debug);
#else
 	eat_uart_set_at_port(EAT_UART_1); 
	eat_uart_set_debug(EAT_UART_USB);
#endif
// set debug string info to debug port
#ifdef EAT_DEBUG_STRING_INFO_MODE
    if( EAT_UART_USB == eat_uart_debug)
    {
		eat_uart_set_debug_config(EAT_UART_DEBUG_MODE_TRACE, NULL);
    }else
    {
        eat_uart_set_debug_config(EAT_UART_DEBUG_MODE_UART, &cfg);
    }
#endif
    eat_pin_set_mode(EAT_PIN57_GPIO1, EAT_PIN_MODE_GPIO);

}

static void app_main(void *data)
{	
	EatEvent_st event;
	queue_buf_t rawdata_q;
    u16 gp_satnum = 0,bd_satnum = 0;
	int res = 0;
	
	mtkraw_t rawdata_t;
	unsigned char EncryptedTime[10];
	
	u32 rcv_len = 0;
	u16 flag = 0;
	APP_InitRegions();//Init app RAM, first step
    APP_init_clib(); //C library initialize, second step	
	queue_buf_Init(&rawdata_q,EAT_UART_RX_BUF_LEN_MAX,sizeof(char));
	MTK_GetRawData();
	delay(3000);
  	while(1)
    {	
    #if 1
		if(StartLockFlag > 0){	
			#if 1
			switch(flag)
			{
				case 0:
					ReqGPSEphFlag = 0;
					if(g_gpsave_count > 0){
						eat_trace("######the GPS num[%d] is:%s,savecount is %d",gp_satnum,GPS_satnum[gp_satnum],g_gpsave_count);
						send_cmd_ephe(GPS_satnum[gp_satnum],i_SYS_GPS);
						gp_satnum ++;
						flag = 1;
					}
					
					if(gp_satnum >= g_gpsave_count){
						gp_satnum = 0;
						ReqGPSEphFlag = 1;
						ReqBDSEphFlag = 0;
						g_gpsave_count = 0;
						
					}
					break;
				case 1:
					ReqBDSEphFlag = 0;
					if(g_bdsave_count > 0){
						eat_trace("######the BDS num[%d] is:%s,savecount is %d",bd_satnum,BDS_satnum[bd_satnum],g_bdsave_count);
						send_cmd_ephe(BDS_satnum[bd_satnum],i_SYS_BDS);
						bd_satnum ++;
						flag = 0;
					}
					
					if(bd_satnum >= g_bdsave_count){
						bd_satnum = 0;
						ReqBDSEphFlag = 1;
						ReqGPSEphFlag = 0;
						g_bdsave_count = 0;
						
					}
					break;
			}
			#else
			if(flag == 0){//请求GPS星历数据
				ReqGPSEphFlag = 0;
				if(g_gpsave_count > 0){
					eat_trace("######the GPS num[%d] is:%s,savecount is %d",gp_satnum,GPS_satnum[gp_satnum],g_gpsave_count);
					send_cmd_ephe(GPS_satnum[gp_satnum],i_SYS_GPS);
					gp_satnum ++;
				}
				if(gp_satnum >= g_gpsave_count){
					gp_satnum = 0;
					ReqGPSEphFlag = 1;
					ReqBDSEphFlag = 0;
					g_gpsave_count = 0;
					flag = 1;
				}
			}
			else if(flag ==1){//请求BDS星历数据
				ReqBDSEphFlag = 0;
				if(g_bdsave_count > 0){
					eat_trace("######the BDS num[%d] is:%s,savecount is %d",bd_satnum,BDS_satnum[bd_satnum],g_bdsave_count);
					send_cmd_ephe(BDS_satnum[bd_satnum],i_SYS_BDS);
					bd_satnum ++;
				}
				if(bd_satnum >= g_bdsave_count){
					bd_satnum = 0;
					ReqBDSEphFlag = 1;
					ReqGPSEphFlag = 0;
					g_bdsave_count = 0;
					flag = 0;
				}
			}
			#endif
		}
		memset(uart_rcv,0,sizeof(uart_rcv));
		rcv_len = eat_modem_read(uart_rcv,EAT_UART_RX_BUF_LEN_MAX);
		if(rcv_len > 0){
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
				rcv_len -= res;
				res = DecodeMTKmsg(msg,res, &rawdata_t);//解析语句
			}
			
		}
		delay(1000);
  	}
	#else //加密程序
		if(!Decode_EncryptionID(EncryptedTime)){//获取过期时间函数
			eat_trace("dmh:the time is %s",EncryptedTime);
			return;
		}	
		eat_trace("dmh:your application date is over due.Please contact with the engineer or load the web:\"http://www.zhdgps.com/\"");
		return;
	}
	#endif
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

