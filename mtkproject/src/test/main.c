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
//#include "app_demo_gps.h"
/********************************************************************
 * Macros
 ********************************************************************/
#define EAT_AT_TEST
#define EAT_DEBUG_STRING_INFO_MODE  1 //output debug string info to debug port

#define  CMD_ATCGNSPWR_ON           "AT+CGPIO=0,57,1,1\r\n"
#define  CMD_ATCGNSPWR_OFF          "AT+CGPIO=0,57,1,0\r\n"
#define  CMD_ATCGNSTST_ON           "AT+CGNSTST=1\r\n"
#define  CMD_ATCGNSTST_OFF          "AT+CGNSTST=0\r\n"
#define  CMD_OUTRAWDATA             "AT+CGNSCMD=0,\"$PMTK876,0*27\"\r\n"
#define  CMD_RAWDATA				"$PMTK876,0*27\r\n"
#define  CMD_TURNONDEBUG			"AT+CGNSCMD=0,\"$PMTK299,1*2d\"\r\n"
#define  CMD_URCDATA				"AT+CGNSURC=1\r\n"
#define EAT_DEBUG_STRING_INFO_MODE
#define EAT_UART_RX_BUF_LEN_MAX  2048
#define NMEA_BUFF_SIZE 100


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
static const EatUart_enum eat_uart_debug = EAT_UART_1;
static const EatUart_enum eat_uart_at = EAT_UART_NULL;
static u16 wr_uart_offset = 0;
static u16 wr_uart_len = 0;
static char gps_info_buf[NMEA_BUFF_SIZE]="";
u8 rx_buf[2048];




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

static void mdm_rx_proc(const EatEvent_st* event);
static void uart_rx_proc(const EatEvent_st* event);
static void uart_send_complete_proc(const EatEvent_st* event);
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
 	eat_uart_set_at_port(EAT_UART_NULL); 
	eat_uart_set_debug(EAT_UART_1);
#endif
//  set debug string info to debug port
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
	char flag = 0;
	//char flag = 2;
	int count = 0;
	u16 buf_len = 0,i;
	u16 rcv_len = 0;
	unsigned char sum;
	EatEvent_st event;
	EatUartConfig_st cfg =
    {
        EAT_UART_BAUD_115200,
        EAT_UART_DATA_BITS_8,
        EAT_UART_STOP_BITS_1,
        EAT_UART_PARITY_NONE
    };
	
	APP_InitRegions();//Init app RAM, first step
    APP_init_clib(); //C library initialize, second step
    //eat_gps_init();
	#if 1
    eat_uart_open(EAT_UART_2);
	eat_uart_set_config(EAT_UART_2,&cfg);	
	#endif
  	while(1)
    {	
	#if 1
		switch(flag)
		{
			case 0:
				eat_modem_write(CMD_ATCGNSPWR_ON,strlen(CMD_ATCGNSPWR_ON));
			//	eat_uart_write(EAT_UART_2,CMD_ATCGNSPWR_ON,strlen(CMD_ATCGNSPWR_ON));
				memset(rx_buf,0,sizeof(rx_buf));
				do{
					buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
				//	buf_len = eat_uart_read(EAT_UART_2,rx_buf,EAT_UART_RX_BUF_LEN_MAX);
					rx_buf[buf_len] = '\0';
					eat_uart_write(EAT_UART_1,rx_buf,buf_len);
					if(strstr(rx_buf,"OK") != NULL){
						eat_trace("GNSS power on ok len is %d\r\n",buf_len);
						flag = 2;
						//delay(2000);
					}
				}while(buf_len == EAT_UART_RX_BUF_LEN_MAX);
				break;
			case 1:
				eat_modem_write(CMD_TURNONDEBUG,strlen(CMD_TURNONDEBUG));
				memset(rx_buf,0,sizeof(rx_buf));
				do{
					buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
					rx_buf[buf_len] = '\0';
					eat_uart_write(EAT_UART_1,rx_buf,buf_len);
					if(strstr(rx_buf,"OK") != NULL){
						eat_trace("GNSS stst ok len is %d \r\n",buf_len);
						flag = 2;
						//delay(2000);
					}
				}while(buf_len == EAT_UART_RX_BUF_LEN_MAX);
				break;
			case 2:	
				eat_modem_write(CMD_OUTRAWDATA,strlen(CMD_OUTRAWDATA));
				memset(rx_buf,0,sizeof(rx_buf));
				do{
					buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
					rx_buf[buf_len] = '\0';
					eat_uart_write(EAT_UART_1,rx_buf,buf_len);
					if(strstr(rx_buf,"OK") != NULL){
						eat_trace("GNSS out raw data on ok len is %d\r\n",buf_len);
						flag = 3;
						//delay(2000);
					}
				}while(buf_len == EAT_UART_RX_BUF_LEN_MAX);
				break;
			case 3:
				memset(rx_buf,0,sizeof(rx_buf));
				do{
					buf_len = eat_uart_read(EAT_UART_2,rx_buf,EAT_UART_RX_BUF_LEN_MAX);
					//buf_len = eat_modem_read(rx_buf,EAT_UART_RX_BUF_LEN_MAX);
					if(buf_len > 0){
						eat_uart_write(EAT_UART_1,rx_buf,buf_len);
						//eat_trace("%s",rx_buf);
					}
					//delay(1000);
				}while(buf_len == EAT_UART_RX_BUF_LEN_MAX);
				break;
		}
		
	#endif
    }
}


