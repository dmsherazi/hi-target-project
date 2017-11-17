#include "getrawdata.h"

#define BUFFLEN 1024
#define STARTCHAR	'$'
#define ENDCHAR1	'\r'
#define ENDCHAR2	'\n'


static u8 rawdata_buf[RAWDATA_BUF_LEN];
unsigned char msg[256];
int g_bdgsv_count = 0;
int g_gpgsv_count = 0;
int g_gpsave_count = 0;
int g_bdsave_count = 0;
int g_gpsatsum = 0;	
int g_bdsatsum = 0;
char BDS_satnum[MAXFIELD][5];	
char GPS_satnum[MAXFIELD][5];
char g_pre_gpsnum[MAXFIELD][5];
char g_pre_bdsnum[MAXFIELD][5];

char *val[MAXFIELD];
int StartLockFlag = 0;
int ReqBDSEphFlag = 0;
int ReqGPSEphFlag = 0;

int GetTimeFlag = 0;
char GPSTempFlag = 0;
char BDSTempFlag = 0;
int InitTime = 0;

void delay(u32 ncount)
{
	EatEvent_st event;
	unsigned int time = 0;
	unsigned int time_count = 0;
	time = eat_get_current_time();
	eat_timer_start(EAT_TIMER_1,ncount);
	while(1)
	{
		eat_get_event(&event);
		if(EAT_EVENT_TIMER == event.event){
			time_count = eat_get_duration_ms(time);
			eat_timer_stop(EAT_TIMER_1);
			//eat_trace("the time count is %d",time_count);
			ncount = 0;
			break;
		}
	}
}

extern int GetCheckSUM(char *buffer)
{
	int i,length = strlen(buffer);
	unsigned char checksum; 
	checksum = buffer[1];
	for (i = 2; buffer[i] != '*'; i++) {
		checksum ^= buffer[i];
	}
	return checksum;
}

int send_cmd_ephe(char *satellitenum,i_SysEnum_t systype)
{
	char trunk_cmd[40] = "AT+CGNSCMD=0,\"";
	char trunk_teminator[3] = "\"\r\n";
	char GPS_Cmd_Body[110] = "$PMTK668,";
	char BDS_Cmd_Body[120] = "$PMTK669,";
	int checksum = 0,buf_len = 0;
	unsigned char sat_buf[1024];
	if(satellitenum == NULL || (atoi(satellitenum) == 0)||(!strcmp(satellitenum,"")))
		return -1;
	switch(systype)
	{
		case i_SYS_GPS://cmdbody:$PMTK668,num
			strcat(GPS_Cmd_Body,satellitenum);
			strcat(GPS_Cmd_Body,"*");
			checksum = GetCheckSUM(GPS_Cmd_Body);
			sprintf(GPS_Cmd_Body,"%s%x",GPS_Cmd_Body,checksum);
			strcat(trunk_cmd,GPS_Cmd_Body);
			strcat(trunk_cmd,trunk_teminator);
			eat_modem_write(trunk_cmd,strlen(trunk_cmd));
			return 1;
		case i_SYS_BDS://cmdbody:$PMTK669,num
			strcat(BDS_Cmd_Body,satellitenum);	
			strcat(BDS_Cmd_Body,"*");
			checksum = GetCheckSUM(BDS_Cmd_Body);
			sprintf(BDS_Cmd_Body,"%s%x",BDS_Cmd_Body,checksum);
			strcat(trunk_cmd,BDS_Cmd_Body);
			strcat(trunk_cmd,trunk_teminator);
			eat_modem_write(trunk_cmd,strlen(trunk_cmd));
			return 1;
		default:
			break;
	}
	return 0;	
}

static int decode_PMTK668(char **val, int n)
{
	int i,prn,weeknum,urai,idot,iode,toc,af2,af1,af0,iodc,crs,dn,anomtime,cuc,ecce,cus;
	int sqrta,toe,cic,omega,cis,incline,crc,argperig,omegadot,tgd,svhealth;
	for(i = 0; i< n; i++)
	{
		switch(i)
		{
			case 0://SVID of satellite
				prn = atoi(val[i]);eat_trace("dmh:decode MTK668 prn is:%d,%s",prn,val[i]);break;
			case 1://referrnce week number[weeks]
				weeknum = atoi(val[i]);eat_trace("dmh:decode MTK668 weeknum is:%d,%s",weeknum,val[i]);break;
			case 2://figure of merit-defines URA
				urai = atoi(val[i]);eat_trace("dmh:decode MTK668 urai is:%d,%s",urai,val[i]);break;
			case 3://rate of inclination angle[rad/s]
				idot = atoi(val[i]);eat_trace("dmh:decode MTK668 idot is:%d,%s",idot,val[i]);break;
			case 4://issue of data counter
				iode = atoi(val[i]);eat_trace("dmh:decode MTK668 iode is:%d,%s",iode,val[i]);break;
			case 5://reference time of week[s] 
				toc = atoi(val[i]);eat_trace("dmh:decode MTK668 toc is:%d,%s",toc,val[i]);break;
			case 6://SV clock correction polynomial coefficient[s/s/s]
				af2 = atoi(val[i]);eat_trace("dmh:decode MTK668 af2 is:%d,%s",af2,val[i]);break;
			case 7://SV clock correction polynomial coefficient[s/s]
				af1 = atoi(val[i]);eat_trace("dmh:decode MTK668 af1 is:%d,%s",af1,val[i]);break;
			case 8://SV clock correction polynomial coefficient[s]
				af0 = atoi(val[i]);eat_trace("dmh:decode MTK668 af0 is:%d,%s",af0,val[i]);break;
			case 9://issue of data counter
				iodc = atoi(val[i]);eat_trace("dmh:decode MTK668 iodc is:%d,%s",iodc,val[i]);break;
			case 10://issue of data counter
				crs = atoi(val[i]);eat_trace("dmh:decode MTK668 crs is:%d,%s",crs,val[i]);break;
			case 11://delta n mean motion diff from computed value[rad/s]
				dn = atoi(val[i]);eat_trace("dmh:decode MTK668 dn is:%d,%s",dn,val[i]);break;
			case 12://mean anomaly at reference time[rad]
				anomtime = atoi(val[i]);eat_trace("dmh:decode MTK668 anomtime is:%d,%s",anomtime,val[i]);break;
			case 13://amplitude of cos harm corr term arg of latitude[rad]
				cuc = atoi(val[i]);eat_trace("dmh:decode MTK668 cuc is:%d,%s",cuc,val[i]);break;
			case 14://eccemtricity
				ecce = atoi(val[i]);eat_trace("dmh:decode MTK668 ecce is:%d,%s",ecce,val[i]);break;
			case 15://amplitude og sim harm corr term arg of latitude[rad]
				cus = atoi(val[i]);eat_trace("dmh:decode MTK668 cus is:%d,%s",cus,val[i]);break;
			case 16://square root of the semi-major axis
				sqrta = atoi(val[i]);eat_trace("dmh:decode MTK668 sqrta is:%d,%s",sqrta,val[i]);break;
			case 17://reference time of week[ephemeris terms][s]
				toe = atoi(val[i]);eat_trace("dmh:decode MTK668 toe is:%d,%s",toe,val[i]);break;
			case 18://amplitude of cos harm corr term ange of inclination[rad]
				cic = atoi(val[i]);eat_trace("dmh:decode MTK668 cic is:%d,%s",cic,val[i]);break;
			case 19://longitude of ascending node of orbit plane[rad]
				omega = atoi(val[i]);eat_trace("dmh:decode MTK668 omega is:%d,%s",omega,val[i]);break;
			case 20://anplitude of sin harm corr term ang of inclination[rad]
				cis = atoi(val[i]);eat_trace("dmh:decode MTK668 cis is:%d,%s",cis,val[i]);break;
			case 21://inclinarion angle at reference time[rad]
				incline = atoi(val[i]);eat_trace("dmh:decode MTK668 incline is:%d,%s",incline,val[i]);break;
			case 22://amplitude of cos harm corr term orbit radius[rad]
				crc = atoi(val[i]);eat_trace("dmh:decode MTK668 crc is:%d,%s",crc,val[i]);break;
			case 23://argument of perigee[rad]
				argperig = atoi(val[i]);eat_trace("dmh:decode MTK668 argperig is:%d,%s",argperig,val[i]);break;
			case 24://rate of right ascention[rad/s]
				omegadot = atoi(val[i]);eat_trace("dmh:decode MTK668 omegadot is:%d,%s",omegadot,val[i]);break;
			case 25://group delay[s]
				tgd = atoi(val[i]);eat_trace("dmh:decode MTK668 tgd is:%d,%s",tgd,val[i]);break;
			case 26://the 5 LSBs of the NAV data`s health status from the ephemeris
				svhealth = atoi(val[i]);eat_trace("dmh:decode MTK668 svhealth is:%d,%s",svhealth,val[i]);break;
		}
	}
	return 1;
}

static int decode_PMTK669(char **val, int n)
{
	int i = 0,prn,weeknum,urai,idot,iode,toc,af2,af1,af0,iodc,crs,dn,anomtime,cuc,ecce,cus;
	int sqrta,toe,cic,omega,cis,incline,crc,argperig,omegadot,tgd,svhealth;
	for(i = 0; i< n; i++)
	{
		switch(i)
		{
			case 0://SVID of satellite
				prn = atoi(val[i]);eat_trace("dmh:decode MTK669 prn is:%d,%s",prn,val[i]);break;
			case 1://referrnce week number[weeks]
				weeknum = atoi(val[i]);eat_trace("dmh:decode MTK669 weeknum is:%d,%s",weeknum,val[i]);break;
			case 2://figure of merit-defines URA
				urai = atoi(val[i]);eat_trace("dmh:decode MTK669 urai is:%d,%s",urai,val[i]);break;
			case 3://rate of inclination angle[rad/s]
				idot = atoi(val[i]);eat_trace("dmh:decode MTK669 idot is:%d,%s",idot,val[i]);break;
			case 4://issue of data counter
				iode = atoi(val[i]);eat_trace("dmh:decode MTK669 iode is:%d,%s",iode,val[i]);break;
			case 5://reference time of week[s] 
				toc = atoi(val[i]);eat_trace("dmh:decode MTK669 toc is:%d,%s",toc,val[i]);break;
			case 6://SV clock correction polynomial coefficient[s/s/s]
				af2 = atoi(val[i]);eat_trace("dmh:decode MTK669 af2 is:%d,%s",af2,val[i]);break;
			case 7://SV clock correction polynomial coefficient[s/s]
				af1 = atoi(val[i]);eat_trace("dmh:decode MTK669 af1 is:%d,%s",af1,val[i]);break;
			case 8://SV clock correction polynomial coefficient[s]
				af0 = atoi(val[i]);eat_trace("dmh:decode MTK669 af0 is:%d,%s",af0,val[i]);break;
			case 9://issue of data counter
				iodc = atoi(val[i]);eat_trace("dmh:decode MTK669 iodc is:%d,%s",iodc,val[i]);break;
			case 10://issue of data counter
				crs = atoi(val[i]);eat_trace("dmh:decode MTK669 crs is:%d,%s",crs,val[i]);break;
			case 11://delta n mean motion diff from computed value[rad/s]
				dn = atoi(val[i]);eat_trace("dmh:decode MTK669 dn is:%d,%s",dn,val[i]);break;
			case 12://mean anomaly at reference time[rad]
				anomtime = atoi(val[i]);eat_trace("dmh:decode MTK669 anomtime is:%d,%s",anomtime,val[i]);break;
			case 13://amplitude of cos harm corr term arg of latitude[rad]
				cuc = atoi(val[i]);eat_trace("dmh:decode MTK669 cuc is:%d,%s",cuc,val[i]);break;
			case 14://eccemtricity
				ecce = atoi(val[i]);eat_trace("dmh:decode MTK669 ecce is:%d,%s",ecce,val[i]);break;
			case 15://amplitude og sim harm corr term arg of latitude[rad]
				cus = atoi(val[i]);eat_trace("dmh:decode MTK669 cus is:%d,%s",cus,val[i]);break;
			case 16://square root of the semi-major axis
				sqrta = atoi(val[i]);eat_trace("dmh:decode MTK669sqrta is:%d,%s",sqrta,val[i]);break;
			case 17://reference time of week[ephemeris terms][s]
				toe = atoi(val[i]);eat_trace("dmh:decode MTK669 toe is:%d,%s",toe,val[i]);break;
			case 18://amplitude of cos harm corr term ange of inclination[rad]
				cic = atoi(val[i]);eat_trace("dmh:decode MTK669 cic is:%d,%s",cic,val[i]);break;
			case 19://longitude of ascending node of orbit plane[rad]
				omega = atoi(val[i]);eat_trace("dmh:decode MTK669 omega is:%d,%s",omega,val[i]);break;
			case 20://anplitude of sin harm corr term ang of inclination[rad]
				cis = atoi(val[i]);eat_trace("dmh:decode MTK669 cis is:%d,%s",cis,val[i]);break;
			case 21://inclinarion angle at reference time[rad]
				incline = atoi(val[i]);eat_trace("dmh:decode MTK669 incline is:%d,%s",incline,val[i]);break;
			case 22://amplitude of cos harm corr term orbit radius[rad]
				crc = atoi(val[i]);eat_trace("dmh:decode MTK669 crc is:%d,%s",crc,val[i]);break;
			case 23://argument of perigee[rad]
				argperig = atoi(val[i]);eat_trace("dmh:decode MTK669 argperig is:%d,%s",argperig,val[i]);break;
			case 24://rate of right ascention[rad/s]
				omegadot = atoi(val[i]);eat_trace("dmh:decode MTK669 omegadot is:%d,%s",omegadot,val[i]);break;
			case 25://group delay[s]
				tgd = atoi(val[i]);eat_trace("dmh:decode MTK669 tgd is:%d,%s",tgd,val[i]);break;
			case 26://the 5 LSBs of the NAV data`s health status from the ephemeris
				svhealth = atoi(val[i]);eat_trace("dmh:decode MTK669 svhealth is:%d,%s",svhealth,val[i]);break;
		}
	}
	return 1;
}

static int decode_GNGGA(char **val,int n)
{
	int i = 0, ReceTime = 0, IntervalTime = 0,temp = 0,hour=0,min=0,sec=0,gettime = 0;
	for(i =0; i < n; i++)
	{
		switch(i)
		{
			case 1:
				if(StartLockFlag > 0){
					if(GetTimeFlag == 0){
						gettime = atoi(val[i]);
						hour = gettime /10000;
						min = gettime%10000/100;
						sec = gettime%10000%100;
						InitTime = hour*3600 + min*60 +sec;
						GetTimeFlag = 1;
					}
					else{
						gettime = atoi(val[i]);
						hour = gettime /10000;
						min = gettime%10000/100;
						sec = gettime%10000%100;
						ReceTime = hour*3600 + min*60 +sec;
						IntervalTime = ReceTime - InitTime;
						if(IntervalTime >= (15*60)){//每十五分钟刷新星历一次
							for(temp = 0;temp < MAXFIELD; temp ++)
							{
								strcpy(*(g_pre_gpsnum +temp),"");
								strcpy(*(g_pre_bdsnum +temp),"");
							}
							g_gpsatsum = 0;
							g_bdsatsum = 0;
							InitTime = ReceTime;
						}
					}
				}
				break;
			case 3:
				if((!strcmp(val[i],"N"))||(!strcmp(val[i],"S")))
				{
					StartLockFlag = 1;
				}
				else
				{
					StartLockFlag = -1;
				}
				break;
			case 5:
				if((!strcmp(val[i],"E"))||(!strcmp(val[i],"W")))
				{
					StartLockFlag = 2;
				}
				else 
				{
					StartLockFlag = -2;
				}
				break;
			default:
			break;
		}
	}
	return 0;
}

static int decode_GPGSV(char **val,int n)
{
	int i = 0, j = 0;
	char rec_gpsnum[MAXFIELD][5];
	for(i = 0; i < MAXFIELD; i++){
		strcpy(*(rec_gpsnum + i),"");
	}
	if(StartLockFlag > 0 ){
		for(i = 0; i < n; i++)//get the satellite num  
		{
			if(i == 4 || i ==8 || i == 12 || i == 16){//保存卫星编号
				strcpy(rec_gpsnum[g_gpgsv_count],val[i]);
				//eat_trace("dmh:rec num[%d]is:%s",g_gpgsv_count,rec_gpsnum[g_gpgsv_count]);
				g_gpgsv_count ++;
			}
		}	
		for(i = 0; i < MAXFIELD; i++)//对比卫星编号，找出新出现的卫星
		{
			for(j = 0; j < MAXFIELD; j++){
				if(!strcmp(rec_gpsnum[i],g_pre_gpsnum[j]) || !strcmp(rec_gpsnum[i],"")){
					break;
				}
			}
			if(j == MAXFIELD){//遍历一次数组后未发现重复卫星编号
				strcpy(GPS_satnum[g_gpsave_count],rec_gpsnum[i]);
				//eat_trace("dmh:save num[%d]is:%s",g_gpsave_count,GPS_satnum[g_gpsave_count]);
				g_gpsave_count++;
			}
		}
		if(g_gpsave_count > 0 )//若有新出现的卫星
		{
			for(i = 0 ; i < g_gpsave_count; i++){
				strcpy(g_pre_gpsnum[g_gpsatsum],GPS_satnum[i]);
				//eat_trace("!!!!!!!!!!!dmh:the gps pre num[%d]is:%s,save num is %d",g_gpsatsum,g_pre_gpsnum[g_gpsatsum],g_gpsave_count);
				g_gpsatsum ++;
			}
		}
		return 1;
	}
	return 0;
}

static int decode_BDGSV(char **val,int n)
{
	int i = 0, j = 0;
	char rec_bdsnum[MAXFIELD][5];
	for(i = 0; i < MAXFIELD; i++){
		strcpy(*(rec_bdsnum + i),"");
	}
	if(StartLockFlag > 0 ){
		for(i = 0; i < n; i++)//保存卫星编号
		{
			if(i == 4 || i ==8 || i == 12 || i == 16){
				strcpy(rec_bdsnum[g_bdgsv_count],val[i]);//
				//eat_trace("dmh:rec num[%d]is:%s",g_bdgsv_count,rec_bdsnum[g_bdgsv_count]);
				g_bdgsv_count ++;
			}
		}
		for(i = 0; i < MAXFIELD; i++)//对比卫星编号，找出新出现的卫星
		{
			for(j = 0; j < MAXFIELD; j++){
				if(!strcmp(rec_bdsnum[i],g_pre_bdsnum[j]) || !strcmp(rec_bdsnum[i],"")){
					break;
				}
			}
			if(j == MAXFIELD){//遍历一次数组后未发现重复卫星编号
				strcpy(BDS_satnum[g_bdsave_count],rec_bdsnum[i]);
				//eat_trace("dmh:save num[%d]is:%s",g_bdsave_count,BDS_satnum[g_bdsave_count]);
				g_bdsave_count++;
			}
		}
		if(g_bdsave_count > 0 )//若有新出现的卫星
		{
			for(i = 0 ; i < g_bdsave_count; i++){
				strcpy(g_pre_bdsnum[g_bdsatsum],BDS_satnum[i]);
				//eat_trace("dmh:~~~~~~~~pre num[%d]is:%s,the save num is %d",g_bdsatsum,g_pre_bdsnum[g_bdsatsum],g_bdsave_count);
				g_bdsatsum ++;
			}
		}
		return 1;
	}
	return 0;
}

static int decode_PMTKCHL(char **val, int n, mtkraw_t *mtkraw)
{
	int i,sys,sat,sysid,prn,slipc,fcn,ions,sync;
	long int iode;
	double range,phase,doppler,ionc;
	unsigned char snr;
	double rs[3];
	mtkd_t mtkd={0};
	eat_trace("decode_PMTKCHL: n=%d\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTKCHL (%4d):", mtkraw->len);
	}

	for (i=0;i<n;i++)
	{
		switch (i) 
		{
		case  0: sysid  =atoi(val[i]) +1;eat_trace("sysid :%d",sysid); break; /* system id */ 
		case  1: prn    =atoi(val[i]); eat_trace("prn :%d",prn); break; /* satellite id */
		case  2: range  =atof(val[i]); eat_trace("range :%d",range); break; /* pseudorange (unit:m ) */
		case  3: phase  =atof(val[i]); eat_trace("phase :%d",phase); break; /* time sync carrier phase (unit:cycle) */
		case  4: doppler=atof(val[i]); eat_trace("doppler :%d",doppler); break; /* doppler (unit:Hz) */
		case  5: slipc  =atoi(val[i]); eat_trace("slipc :%d",slipc); break; /* cycle slip count(0-999) */
		case  6: snr    =atoi(val[i]); eat_trace("snr :%d",snr); break; /* SNR (integer:00-99 unit:C/N0 ) */
		case  7: rs[0]  =atof(val[i]); eat_trace("rs[0] :%d",rs[0]); break; /* satellite positions(X)(unit:m ) */
		case  8: rs[1]  =atof(val[i]); eat_trace("rs[1]  :%d",rs[1]); break; /* satellite positions(Y)(unit:m ) */
		case  9: rs[2]  =atof(val[i]); eat_trace("rs[2] :%d",rs[2]); break; /* satellite positions(Z)(unit:m ) */
		case 10: fcn    =atoi(val[i]); eat_trace("fcn :%d",fcn); break; /* GLONASS satellite frequency channel - 8 */
		case 11: iode   =strtoul(val[i],NULL,16); eat_trace("iode :%d",iode); break; /* altitude in msl */
		case 12: ionc   =atof(val[i]); eat_trace("ionc :%d",ionc); break; /* ionosphere correction (unit:m) */
		case 13: ions   =atoi(val[i]); eat_trace("ions :%d",ions); break; /* ionosphere source(0:none 1:broadcast 2:sbas) */
		case 14: sync   =atoi(val[i]); eat_trace("sync :%d",sync); break; /* sync status(0:none 1:bit sync 2:subframe sync 3:exact sync) */
		}
	}
	if(sysid == 1) sys=SYS_GPS;
	else if(sysid == 2) sys=SYS_GLO;
	else if(sysid == 3) sys=SYS_BDS;
	else return -1;
	return 1;
}

#if 0

static int decode_PMTKGRP(char **val, int n, mtkraw_t *mtkraw)
{
	int i,clctime,weeksec,week,clcstat,leaps,clcBias,clcoffA,clcoffB;
	gtime_t time;

	trace(2,"decode_PMTKCHL: n=%d\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTKCHL (%4d):", mtkraw->len);
	}

	for (i=0;i<n;i++)
	{
		switch (i) 
		{
		case  0: clctime=atoi(val[i]); break; /* local receiver time(range:2^32-1 interger unit:ms) */ 
		case  1: weeksec=atoi(val[i]); break; /* GPS time of week(range:0-604800000 interger unit:s) */
		case  2: week   =atoi(val[i]); break; /* GPS week number(range:0-9999 interger) */
		case  3: clcstat=atoi(val[i]); break; /* clock status(0:no clock 1:RTC 2:synced to gps 3:from GPS fix)*/
		case  4: leaps  =atoi(val[i]); break; /* the difference between GPS and UTC(integer unit:second) */
		case  5: clcBias=atoi(val[i]); break; /* clock bias(interger unit:meter)*/
		case  6: clcoffA=atoi(val[i]); break; /* the clock offset between GPS clock and GLONASS clock(integer unit:meter)*/
		case  7: clcoffB=atoi(val[i]); break; /* the clock offset between GPS clock and GLONASS clock(integer unit:meter)*/
		}
	}
	mtkraw->nav.leaps=leaps;
	time=gpst2time(week,weeksec);
	mtkraw->tobs=time;

	if(clctime!=mtkraw->clctime) 
	{
		mtkraw->clctime=clctime;
		mtkraw->mtkobs.n=0;
		mtkraw->sync=1;
	}
	else mtkraw->sync=0;

	return 1;
}

static int decode_PMTKVNED(char **val, int n, mtkraw_t *mtkraw)
{
	int i,clctime,velN,velE,velU,speedH,speed;

	trace(2,"decode_PMTKVNED: n=%d\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTKVNED (%4d):", mtkraw->len);
	}
	for (i=0;i<n;i++)
	{
		switch (i) 
		{
		case  0: clctime=atoi(val[i]); break; /* local receiver time(range:2^32-1 interger unit:ms) */ 
		case  1: velN   =atoi(val[i]); break; /* north velocity(integer unit:m/s) */
		case  2: velE   =atoi(val[i]); break; /* east  velocity(integer unit:m/s) */
		case  3: velU   =atoi(val[i]); break; /* up    velocity(integer unit:m/s) */
		case  4: speedH =atoi(val[i]); break; /* horizontal ground speed(integer unit:m/s)*/
		case  5: speed  =atoi(val[i]); break; /* object spee, include horizontal and vertiacal speed(integer unit:m/s)*/
		}
	}
	if(clctime!=mtkraw->clctime) 
	{
	   	mtkraw->clctime=clctime;
		mtkraw->mtkobs.n=0;
		mtkraw->sync=1;
	}
	else mtkraw->sync=0;

	return 1;
}

static int update_obstime(mtkraw_t *mtkraw)
{
	int i;
	mtkobs_t *obs=&mtkraw->mtkobs;

	if(mtkraw->sync) return 0;

	for(i=0;i<obs->n;i++)
	{
		obs->data[i].time=mtkraw->tobs;
	}
	return 1;
}
#endif

static int GetMsgType(char *msgId)
{
	if     (!strcmp(msgId,ID_PMTKCHL))  return TYPE_PMTKCHL ;
	else if(!strcmp(msgId,ID_PMTKGRP))  return TYPE_PMTKGRP ;
	else if(!strcmp(msgId,ID_PMTKVNED)) return TYPE_PMTKVNED;
	else if(!strcmp(msgId,ID_PMTK473))  return TYPE_PMTK473 ;
	else if(!strcmp(msgId,ID_PMTK474))  return TYPE_PMTK474 ;
	else if(!strcmp(msgId,ID_PMTK477))  return TYPE_PMTK477 ;
	else if(!strcmp(msgId,ID_PMTK478))  return TYPE_PMTK478 ;
	else if(!strcmp(msgId,ID_PMTK493))  return TYPE_PMTK493 ;
	else if(!strcmp(msgId,ID_PMTK494))  return TYPE_PMTK494 ;
	else if(!strcmp(msgId,ID_PMTK668))  return TYPE_PMTK668 ;
	else if(!strcmp(msgId,ID_PMTK669))  return TYPE_PMTK669 ;
	else if(!strcmp(msgId,ID_GNGGA)  )  return TYPE_GNGGA;
	else if(!strcmp(msgId,ID_GPGGA)  )  return TYPE_GPGGA;
	else if(!strcmp(msgId,ID_GPGSV)  )  return TYPE_GPGSV;
	else if(!strcmp(msgId,ID_BDGSV)  )  return TYPE_BDGSV;
	else if(!strcmp(msgId,ID_GLGSV)  )  return TYPE_GLGSV;
	else if(!strcmp(msgId,ID_GPGSA)  )  return TYPE_GPGSA;
	else if(!strcmp(msgId,ID_GLGSA)  )  return TYPE_GLGSA;
	else if(!strcmp(msgId,ID_BDGSA)  )  return TYPE_BDGSA;
	else if(!strcmp(msgId,ID_GPZDA)  )  return TYPE_GPZDA;
	else if(!strcmp(msgId,ID_GNZDA)  )  return TYPE_GNZDA;
	else if(!strcmp(msgId,ID_GPRMC)  )  return TYPE_GPRMC;
	else if(!strcmp(msgId,ID_GNRMC)  )  return TYPE_GNRMC;
	else if(!strcmp(msgId,ID_GPGST)  )  return TYPE_GPGST;
	else if(!strcmp(msgId,ID_GNGST)  )  return TYPE_GNGST;
	else if(!strcmp(msgId,ID_GPVTG)  )  return TYPE_GPVTG;
	else if(!strcmp(msgId,ID_GNVTG)  )  return TYPE_GNVTG;
	else return -1;		  
}

int GetOneMsg(queue_buf_t *q, unsigned char *buff,  int size)
{
	int res = 0;
	int start=0,end,minimumlength=1,msglen=0,type=-1;
	int sflg=0,eflg=0;
	unsigned char startchar,endchar1,endchar2;
	while(1)   /* find the head */
	{
		if(queue_buf_Peek(q, start, &startchar)) {sflg=0;break;};
		if (startchar != STARTCHAR) 
		{
			start++;
			continue;
		}
		else 
		{
			sflg=1;
			break;
		}
	}
	if(!sflg)   
	{
		queue_buf_Output(q, NULL, start);
		return -2;   /* not find the head */
	}
	end=start;
	while(1)   /* find the terminator */
	{
		if(queue_buf_Peek(q, end+0, &endchar1)||queue_buf_Peek(q, end+1, &endchar2)) 
		{
			eflg=0;
			break;
		}
		if (endchar1!= ENDCHAR1||endchar2!= ENDCHAR2) 
		{
			end++;
			continue;
		}
		else 
		{
			eflg=1;
			break;
		}
	}
	if(!eflg) 
	{
		queue_buf_Output(q,NULL,start);
		return -1;  /* not find the terminator */
	}

	for (res=0; start + minimumlength <= end+2;) 
	{
		queue_buf_Peek(q, start, &startchar);
		if (startchar != STARTCHAR) 
		{
			start++;
			continue;
		}
		msglen=end+2-start;
		if(msglen>BUFFLEN) {start++;continue;}

		queue_buf_Copy(q, start, msglen, msg);
		msg[msglen]='\0';
		if (!CheckSUM(msg))
		{
			start++;
			continue;
		}
		res=1;
		start += msglen;
		break;
	}
	if(res==1&&msglen<=size)
	{
		memcpy(buff,msg,msglen*sizeof(char));
	}
	else
	{
		msglen=0; /* invalid message */   
	}
	queue_buf_Output(q, NULL, start);

	return msglen;
}


int DecodeMTKmsg(void *msg, int nbyte, mtkraw_t *mtkraw)
{
	int res=0,n=0,type=-1,i = 0;
	char *buff=(char *)msg;
    char *p,*q;
	
	if(nbyte<5) return -1; 

	 /*parse fields*/ 
	q = strtok(buff,"*");
	p = strtok(q,",");
	 while(p)
	{
		val[n] = p;
		p = strtok(NULL,",");
		n++;
	}
	/* get massege type */
	type=GetMsgType(val[0]);
	switch(type)
	{
	case TYPE_GNGGA:
		eat_trace("the msg is TYPE_GNGGA,decoding");
		res = decode_GNGGA(val,n);
		break;
	case TYPE_BDGSV:
		if(!BDSTempFlag){
			eat_trace("the msg is TYPE_BDGSV,decoding");
			res = decode_BDGSV(val,n);
			BDSTempFlag = 1;
		}
		if(ReqBDSEphFlag){
			eat_trace("the msg is TYPE_BDGSV,decoding");
			res = decode_BDGSV(val,n);
		}
		break;
	case TYPE_GPGSV:
		if(!GPSTempFlag){
			eat_trace("the msg is TYPE_GPGSV,decoding");
			res = decode_GPGSV(val,n);
			GPSTempFlag = 1;
		}
		if(ReqGPSEphFlag){
			eat_trace("the msg is TYPE_GPGSV,decoding");
			res = decode_GPGSV(val,n);
		}
		break;
	case TYPE_PMTKCHL :
		//eat_trace("the msg is TYPE_PMTKCHL,decoding");
		//res=decode_PMTKCHL (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTKGRP :
		//eat_trace("the msg is TYPE_PMTKGRP,decoding");
		//res=decode_PMTKGRP (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTKVNED:
		//eat_trace("the msg is TYPE_PMTKVNED,decoding");//res=decode_PMTKVNED(val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK473 :
		//eat_trace("the msg is TYPE_PMTK473,decoding");//res=decode_PMTK473 (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK474 :
		//eat_trace("the msg is TYPE_PMTK474,decoding");//res=decode_PMTK474 (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK477 :
		//eat_trace("the msg is TYPE_PMTK477,decoding");//res=decode_PMTK477 (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK478 :
		//eat_trace("the msg is TYPE_PMTK478,decoding");//decode_PMTK478 (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK493 :
		//eat_trace("the msg is TYPE_PMTK493,decoding");//decode_PMTK493 (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK494 :
		//eat_trace("the msg is TYPE_PMTK494,decoding");//decode_PMTK494 (val+1,n-1,mtkraw);
		break;
	case TYPE_PMTK668 :
		eat_trace("the msg is TYPE_PMTK668,decoding");
	//	res = decode_PMTK668 (val+1,n-1);
		break;
	case TYPE_PMTK669 :
		eat_trace("the msg is TYPE_PMTK669,decoding");
	//	res = decode_PMTK669 (val+1,n-1);
		break;		
	}

	return res;
}

extern int CheckSUM(char *buffer)
{
	int i,len = strlen(buffer);
    char hexstr[3] = "";
    char crc,sum; 
	hexstr[0] = buffer[len - 4];
	hexstr[1] = buffer[len - 3];
	hexstr[2] = '\0';
	crc = strtoul(hexstr, NULL, 16);  
	sum = buffer[1];
	for (i = 2; buffer[i] != '*'; i++) {
		sum ^= buffer[i];
	}
	return (sum == crc);
}

int MTK_GetRawData(void)
{
	char flag = 0;
	int temp = 0;
	s16 rawdata_buflen = 0;
	while(1)
	{
		switch(flag)
		{
			case 0://拉高GPS电源引脚，使能GPS
				eat_modem_write(CMD_ATCGNSPWR_ON,strlen(CMD_ATCGNSPWR_ON));
				memset(rawdata_buf,0,sizeof(rawdata_buf));
				do{
					rawdata_buflen = eat_modem_read(rawdata_buf,RAWDATA_BUF_LEN);
					rawdata_buf[rawdata_buflen] = '\0';
					if(strstr(rawdata_buf,"OK") != NULL){
						//eat_trace("dmh:send cmd:%s,GNSS power on ok\r\n",CMD_ATCGNSPWR_ON);
						flag = 1;
					}
				}while(rawdata_buflen == RAWDATA_BUF_LEN);
				break;
			case 1:	//使MTK吐出静态数据
				eat_modem_write(CMD_ATCGNSTST_ON,strlen(CMD_ATCGNSTST_ON));
				memset(rawdata_buf,0,sizeof(rawdata_buf));
				do{
					rawdata_buflen = eat_modem_read(rawdata_buf,RAWDATA_BUF_LEN);
					rawdata_buf[rawdata_buflen] = '\0';
					if(strstr(rawdata_buf,"OK") != NULL){
						//eat_trace("dmh:send cmd:%s,GNSS out raw data on ok\r\n",CMD_ATCGNSTST_ON);
						//flag = 2;
						return 1;
					}
				}while(rawdata_buflen == RAWDATA_BUF_LEN);
				break;
		}
		delay(100);
	}
}



