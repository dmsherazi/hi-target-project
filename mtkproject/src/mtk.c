/***********************************************************************
 * @ MTK.c : MTK Raw Data decode
 *
 * Copyright 2017-2025 R&D, ZHD HI-TARGET, ALL Rights Reserved.
 * http://www.zhdgps.com
 *
 * @author : YJCHEN @20170704
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include "string.h"
//#include "rtklib.h"
#include "mtk.h"


#define PI          3.1415926535897932  /* pi */
#define D2R         (PI/180.0)          /* deg to rad */
#define R2D         (180.0/PI)          /* rad to deg */
#define CLIGHT      299792458.0         /* speed of light (m/s) */
#define RE_WGS84    6378137.0           /* earth semimajor axis (WGS84) (m) */
#define FE_WGS84    (1.0/298.257223563) /* earth flattening (WGS84) */

#define BUFFLEN    4096
#define MAXFIELD   64           /* max number of fields in a record */
#define MAXNMEA    256          /* max length of nmea sentence */

#define KNOT2M     0.514444444  /* m/knot */
#define P2_66      1.355252715606881E-20 /* 2^-66 for BeiDou ephemeris */

#define STARTCHAR '$'
#define ENDCHAR1  '\r'
#define ENDCHAR2  '\n'

#define ID_GNGGA "$GNGGA"
#define ID_GPGGA "$GPGGA"
#define ID_GPGSV "$GPGSV"
#define ID_BDGSV "$BDGSV"
#define ID_GLGSV "$GLGSV"
#define ID_GPGSA "$GPGSA"
#define ID_GLGSA "$GLGSA"
#define ID_BDGSA "&BDGSA"
#define ID_GPZDA "$GPZDA"
#define ID_GNZDA "$GNZDA"
#define ID_GPRMC "$GPRMC"
#define ID_GNRMC "&GNRMC"
#define ID_GPGST "$GPGST"
#define ID_GNGST "$GNGST"
#define ID_GPVTG "$GPVTG"
#define ID_GNVTG "$GNVTG"

#define TYPE_GNGGA 1
#define TYPE_GPGGA 2
#define TYPE_GPGSV 3
#define TYPE_BDGSV 4
#define TYPE_GLGSV 5
#define TYPE_GPGSA 6
#define TYPE_GLGSA 7
#define TYPE_BDGSA 8
#define TYPE_GPZDA 9
#define TYPE_GNZDA 10
#define TYPE_GPRMC 11
#define TYPE_GNRMC 12
#define TYPE_GPGST 13
#define TYPE_GNGST 14
#define TYPE_GPVTG 15
#define TYPE_GNVTG 16

#define PMTKmsg      "$PMTK"
#define ID_PMTKCHL   "$PMTKCHL"
#define ID_PMTKGRP   "$PMTKGRP"
#define ID_PMTKVNED  "$PMTKVNED"
#define ID_PMTK473   "$PMTK473"
#define ID_PMTK474   "$PMTK474"
#define ID_PMTK477   "$PMTK477"
#define ID_PMTK478   "$PMTK478"
#define ID_PMTK493   "$PMTK493"
#define ID_PMTK494   "$PMTK494"
#define ID_PMTK668   "$PMTK668"
#define ID_PMTK669   "$PMTK669"

#define TYPE_PMTKCHL   60
#define TYPE_PMTKGRP   61
#define TYPE_PMTKVNED  62
#define TYPE_PMTK473   63
#define TYPE_PMTK474   64
#define TYPE_PMTK477   65
#define TYPE_PMTK478   66
#define TYPE_PMTK493   67
#define TYPE_PMTK494   68
#define TYPE_PMTK668   69
#define TYPE_PMTK669   70

#define MINMTKTYPE  TYPE_PMTKCHL
#define MAXMTKTYPE  TYPE_PMTK669

/* transform geodetic to ecef position -----------------------------------------
* transform geodetic position to ecef position
* args   : double *pos      I   geodetic position {lat,lon,h} (rad,m)
*          double *r        O   ecef position {x,y,z} (m)
* return : none
* notes  : WGS84, ellipsoidal height
*-----------------------------------------------------------------------------*/
static void POS2ECEF(const double *pos, double *r)//将BLH坐标转化为XYZ坐标
{
	double sinp=sin(pos[0]),cosp=cos(pos[0]),sinl=sin(pos[1]),cosl=cos(pos[1]);
	double e2=FE_WGS84*(2.0-FE_WGS84),v=RE_WGS84/sqrt(1.0-e2*sinp*sinp);

	r[0]=(v+pos[2])*cosp*cosl;
	r[1]=(v+pos[2])*cosp*sinl;
	r[2]=(v*(1.0-e2)+pos[2])*sinp;
}


static void saveFile(char *filename, char *buf, int size)//将数据保存文件
{
	FILE *fp = fopen(filename, "a");
	if (NULL != fp) {
		fwrite(buf, 1, size, fp);
		fclose(fp);
	}
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

static int decode_PMTKCHL(char **val, int n, mtkraw_t *mtkraw)
{
	int i,sys,sat,sysid,prn,slipc,fcn,ions,sync;
	long int iode;
	double range,phase,doppler,ionc;
	unsigned char snr;
	double rs[3];
	mtkd_t mtkd={0};

	trace(2,"decode_PMTKCHL: n=%d\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTKCHL (%4d):", mtkraw->len);
	}

	for (i=0;i<n;i++)
	{
		switch (i) 
		{
		case  0: sysid  =atoi(val[i]); break; /* system id */ 
		case  1: prn    =atoi(val[i]); break; /* satellite id */
		case  2: range  =atof(val[i]); break; /* pseudorange (unit:m ) */
		case  3: phase  =atof(val[i]); break; /* time sync carrier phase (unit:cycle) */
		case  4: doppler=atof(val[i]); break; /* doppler (unit:Hz) */
		case  5: slipc  =atoi(val[i]); break; /* cycle slip count(0-999) */
		case  6: snr    =atoi(val[i]); break; /* SNR (integer:00-99 unit:C/N0 ) */
		case  7: rs[0]  =atof(val[i]); break; /* satellite positions(X)(unit:m ) */
		case  8: rs[1]  =atof(val[i]); break; /* satellite positions(Y)(unit:m ) */
		case  9: rs[2]  =atof(val[i]); break; /* satellite positions(Z)(unit:m ) */
		case 10: fcn    =atoi(val[i]); break; /* GLONASS satellite frequency channel - 8 */
		case 11: iode   =strtoul(val[i],NULL,16); break; /* altitude in msl */
		case 12: ionc   =atof(val[i]); break; /* ionosphere correction (unit:m) */
		case 13: ions   =atoi(val[i]); break; /* ionosphere source(0:none 1:broadcast 2:sbas) */
		case 14: sync   =atoi(val[i]); break; /* sync status(0:none 1:bit sync 2:subframe sync 3:exact sync) */
		}
	}
	if(sysid==0) sys=SYS_GPS;
	else if(sysid==1) sys=SYS_GLO;
	else if(sysid==2) sys=SYS_CMP;
	else return -1;

	sat=satno(sys,prn);
	if(sat<=0) return -1;



	mtkd.sat=sat;
	mtkd.P  =range;
	mtkd.L =phase;
	mtkd.D=(float)doppler;
	mtkd.SNR=(unsigned char)(snr*4);	
	mtkd.code=(sys==SYS_CMP?CODE_L1I:CODE_L1C);
	for(i=0;i<3;i++) mtkd.rs[i]=rs[i];

	if(mtkraw->slipc[sat-1]!=slipc) 
	{
		mtkraw->slipc[sat-1]=slipc;
		mtkd.LLI|=1;
	}
	if(mtkraw->mtkobs.n>=mtkraw->mtkobs.nmax)
	{
		mtkraw->sync=1;
		mtkraw->mtkobs.n=0;
	}
	mtkraw->mtkobs.data[mtkraw->mtkobs.n]=mtkd;
	mtkraw->mtkobs.n++;
	
	trace(2,"decode_PMTKCHL: sat=%03d P=%13.3lf L=%13.3lf D=%8.3lf SNR=%d.\n",
		sat,range,phase,doppler,snr);

	return 1;
}

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

static int decode_PMTK473(char **val, int n, mtkraw_t *mtkraw)
{
	trace(3,"decode_PMTK473: n=%d.\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK473 (%4d):", mtkraw->len);
	}
	return 0;
}
static int decode_PMTK474(char **val, int n, mtkraw_t *mtkraw)
{
	trace(3,"decode_PMTK474: n=%d.\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK474 (%4d):", mtkraw->len);
	}
	return 0;
}
static int decode_PMTK477(char **val, int n, mtkraw_t *mtkraw)
{
	trace(3,"decode_PMTK477: n=%d.\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK477 (%4d):", mtkraw->len);
	}
	return 0;
}
static int decode_PMTK478(char **val, int n, mtkraw_t *mtkraw)
{
	trace(3,"decode_PMTK478: n=%d.\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK478 (%4d):", mtkraw->len);
	}
	return 0;
}
static int decode_PMTK493(char **val, int n, mtkraw_t *mtkraw)
{
	trace(3,"decode_PMTK493: n=%d.\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK493 (%4d):", mtkraw->len);
	}
	return 0;
}
static int decode_PMTK494(char **val, int n, mtkraw_t *mtkraw)
{
	trace(3,"decode_PMTK494: n=%d.\n",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK494 (%4d):", mtkraw->len);
	}
	return 0;
}

/* PMTK668-Ephemeris information of GPS system */
static int decode_PMTK668toEph(char **val, int n, eph_t *eph)
{
	int i,prn,week,tgd;
	double toc=0.0,sqrtA=0.0;

	trace(3,"decode_PMTK468toEph :\n ");

	if(n<5) return 0;

	for (i=0;i<n;i++)
	{
		switch (i) 
		{
		case  0: prn      =atoi(val[i]); break; /* satellite id */ 	
		case  1: eph->week=atoi(val[i]); break; /* GPS week number(range:0-1023 interger) */
		case  2: eph->sva =atoi(val[i]); break; /* SV accuracy */
		case  3: eph->idot=atof(val[i])*P2_43*SC2RAD; break;
		case  4: eph->iode=atoi(val[i]); break;
		case  5: toc      =atof(val[i])*16.0; break;
		case  6: eph->f2  =atof(val[i])*P2_55; break;
		case  7: eph->f1  =atof(val[i])*P2_43; break;
		case  8: eph->f0  =atof(val[i])*P2_31; break;
		case  9: eph->iodc=atoi(val[i]); break;
		case 10: eph->crs =atof(val[i])*P2_5 ; break;
		case 11: eph->deln=atof(val[i])*P2_43*SC2RAD; break; 
		case 12: eph->M0  =atof(val[i])*P2_31*SC2RAD; break; 		
		case 13: eph->cuc =atof(val[i])*P2_29; break; 
		case 14: eph->e   =atof(val[i])*P2_33; break;
		case 15: eph->cus =atof(val[i])*P2_29; break;
		case 16: sqrtA    =atof(val[i])*P2_19; break; /* the interger value of val[16] is very large(more than 4 bity ) */
		case 17: eph->toes=atof(val[i])*16.0 ; break; 
		case 18: eph->cic =atof(val[i])*P2_29; break; 
		case 19: eph->OMG0=atof(val[i])*P2_31*SC2RAD; break; 
		case 20: eph->cis =atof(val[i])*P2_29; break; 
		case 21: eph->i0  =atof(val[i])*P2_31*SC2RAD; break; 
		case 22: eph->crc =atof(val[i])*P2_5 ; break; 
		case 23: eph->omg =atof(val[i])*P2_31*SC2RAD; break;
		case 24: eph->OMGd=atof(val[i])*P2_43*SC2RAD; break; 
		case 25: tgd      =atoi(val[i]); break; 
		case 26: eph->svh =atoi(val[i]); break;;
		}
	}
	eph->sat=satno(SYS_GPS,prn);
 	eph->week=adjgpsweek(eph->week);        /* week of tow */
	eph->toc=gpst2time(eph->week,toc);
	eph->toe=gpst2time(eph->week,eph->toes);
	eph->A=sqrtA*sqrtA;
	eph->tgd[0]=tgd==-128?0.0:tgd*P2_31; 

	return 1;
}
static int decode_PMTK668(char **val, int n, mtkraw_t *mtkraw)
{
	eph_t eph={0};

	trace(3,"decode_PMTK468 : n=%d.\n ",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK468 (%4d):", mtkraw->len);
	}
	if(n<5) return 0;
	decode_PMTK668toEph(val,n,&eph);

	mtkraw->ephsat=eph.sat;
	mtkraw->nav.eph[eph.sat-1]=eph;

	return 2;
}

/* PMTK669-Ephemeris information of BDS system */
static int decode_PMTK669toEph( char **val, int n, eph_t *eph)
{
	int i,prn;
	double toc=0.0,sqrtA=0.0;

	trace(3,"decode_PMTK669toEph :\n ");

	if(n<5) return 0;

	for (i=0;i<n;i++)
	{
		switch (i) 
		{
		case  0: prn      =atoi(val[i]); break; /* satellite id */ 	
		case  1: eph->week=atoi(val[i]); break; /* GPS week number(range:0-1023 interger) */
		case  2: eph->sva =atoi(val[i]); break; /* SV accuracy */
		case  3: eph->idot=atof(val[i])*P2_43*SC2RAD; break;
		case  4: eph->iode=atoi(val[i]); break;
		case  5: toc      =atof(val[i])*8.0  ; break;
		case  6: eph->f2  =atof(val[i])*P2_66; break;
		case  7: eph->f1  =atof(val[i])*P2_50; break;
		case  8: eph->f0  =atof(val[i])*P2_33; break;
		case  9: eph->iodc=atoi(val[i]); break;
		case 10: eph->crs =atof(val[i])*P2_6 ; break;
		case 11: eph->deln=atof(val[i])*P2_43*SC2RAD; break; 
		case 12: eph->M0  =atof(val[i])*P2_31*SC2RAD; break; 		
		case 13: eph->cuc =atof(val[i])*P2_31; break; 
		case 14: eph->e   =atof(val[i])*P2_33; break;
		case 15: eph->cus =atof(val[i])*P2_31; break;
		case 16: sqrtA    =atof(val[i])*P2_19; break;    /* the interger value of val[16] is very large(more than 4 bity ) */
		case 17: eph->toes=atof(val[i])*8.0  ; break; 
		case 18: eph->cic =atof(val[i])*P2_31; break; 
		case 19: eph->OMG0=atof(val[i])*P2_31*SC2RAD; break; 
		case 20: eph->cis =atof(val[i])*P2_31; break; 
		case 21: eph->i0  =atof(val[i])*P2_31*SC2RAD; break; 
		case 22: eph->crc =atof(val[i])*P2_6 ; break; 
		case 23: eph->omg =atof(val[i])*P2_31*SC2RAD; break;
		case 24: eph->OMGd=atof(val[i])*P2_43*SC2RAD; break; 
		case 25: eph->tgd[0]=atof(val[i])*1.0E-10; break; 
		case 26: eph->svh =atoi(val[i]); break;
		}
	}
	eph->sat=satno(SYS_CMP,prn);
	eph->week=adjgpsweek(eph->week);        /* week of tow */
	eph->toc=gpst2time(eph->week,toc);
	eph->toe=gpst2time(eph->week,eph->toes);
	eph->toe=gpst2bdt(eph->toe);
	time2bdt(eph->toc,&eph->week);  
	eph->toe=bdt2gpst(eph->toe);
	eph->A=sqrtA*sqrtA;

	return 1;
}
static int decode_PMTK669(char **val, int n, mtkraw_t *mtkraw)
{
	eph_t eph={0};

	trace(3,"decode_PMTK669 : n=%d.\n ",n);

	if (mtkraw->outtype) {
		sprintf(mtkraw->msgtype, "PMTK469 (%4d):", mtkraw->len);
	}
	if(n<5) return 0;
 
	decode_PMTK669toEph(val,n,&eph);

	mtkraw->ephsat=eph.sat;
	mtkraw->nav.eph[eph.sat-1]=eph;
	return 2;
}

static int update_obs(mtkraw_t *mtkraw)
{
	int i,j;
	mtkobs_t *mtkobs=&mtkraw->mtkobs;
	obs_t *obs=&mtkraw->obs;

	trace(4,"update_obs :\n");
	
	obs->n=0;
	for(i=0,j=0;i<mtkobs->n;i++)
	{
		obs->data[j].sat=mtkobs->data[i].sat;
		obs->data[j].time=mtkobs->data[i].time;
		obs->data[j].code[0]=mtkobs->data[i].code;
		obs->data[j].P[0]=mtkobs->data[i].P;
		obs->data[j].L[0]=mtkobs->data[i].L;
		obs->data[j].D[0]=mtkobs->data[i].D;
		obs->data[j].SNR[0]=mtkobs->data[i].SNR;
		obs->data[j++].LLI[0]=mtkobs->data[i].LLI;		
	}
	obs->n=j;
}

extern int DecodeMTKmsg(void *msg, int nbyte, mtkraw_t *mtkraw)
{
	int res=0,n=0,type=-1;
	char *buff=(char *)msg;
    char *p,*q,*val[MAXFIELD];
	

	trace(4,"DecodeMTKmsg: buff=%s\n",buff);

	if(nbyte<5) return -1; 

	/* parse fields */
	for (p=buff;*p&&n<MAXFIELD;p=q+1) {
		if ((q=strchr(p,','))||(q=strchr(p,'*'))) {
			val[n++]=p; *q='\0';
		}
		else break;
	}
	/* get massege type */
	type=GetMsgType(val[0]);

	switch(type)
	{
	case TYPE_PMTKCHL :res=decode_PMTKCHL (val+1,n-1,mtkraw);break;
	case TYPE_PMTKGRP :res=decode_PMTKGRP (val+1,n-1,mtkraw);break;
	case TYPE_PMTKVNED:res=decode_PMTKVNED(val+1,n-1,mtkraw);break;
	case TYPE_PMTK473 :res=decode_PMTK473 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK474 :res=decode_PMTK474 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK477 :res=decode_PMTK477 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK478 :res=decode_PMTK478 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK493 :res=decode_PMTK493 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK494 :res=decode_PMTK494 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK668 :res=decode_PMTK668 (val+1,n-1,mtkraw);break;
	case TYPE_PMTK669 :res=decode_PMTK669 (val+1,n-1,mtkraw);break;
	}
	return res;
}

/* extract one message from the data queue --------------------
* args   : queue_t       *q     IO  data queue
*          unsigned char *buff  IO  msg buffer 
*          unsigned int  size   IO  the max size of buff 
*          prcopt_t *opt    I   processing options
* return : msglen
*--------------------------------------------------------------*/
extern int GetOneMsg(queue_buf_t *q, unsigned char *buff,  int size)
{
	int res = 0;
	unsigned char msg[BUFFLEN];
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
		if (endchar1!= ENDCHAR1/*||endchar2!= ENDCHAR2*/) 
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
			trace(1, "GetOneMsg.checksum error: msg=%s,len=%d\n", msg, msglen);
			start++;
			continue;
		}
		trace(1,"GetOneMsg.msg=%s",msg);
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

/* decode MTK RAW data */
extern int decode_mtk(mtkraw_t *mtkraw)
{
	int res = 0;
	unsigned char msg[BUFFLEN];
	int msglen=0;
	queue_buf_t *q=&mtkraw->queue;

	while(q->size>0&&msglen>=0)
	{
		msglen=GetOneMsg(q,msg,BUFFLEN);
		if(msglen<=0) {res=-1;break;}

		mtkraw->len=msglen;
		res=DecodeMTKmsg(msg,msglen,mtkraw);
		
		if(res<=0) continue;
		else if(res==1)
		{
			res=0;
			if(update_obstime(mtkraw))
			{
				mtkraw->time=mtkraw->tobs;
				update_obs(mtkraw);
				res=1;
				break;
			}
		}
		else if(res==2) break;
	}	
	return res;
}

/* initialize mtk raw data control ----------------------------------------
* initialize mtk raw data control struct and reallocate obsevation and
* epheris buffer
* args   : mtkraw_t  *mtkraw   IO     mtk raw data control struct
* return : status (1:ok,0:memory allocation error)
*-----------------------------------------------------------------------------*/
extern int init_mtkraw(mtkraw_t *mtkraw)
{
	const double lam_glo[NFREQ]={CLIGHT/FREQ1_GLO,CLIGHT/FREQ2_GLO};
	gtime_t time0={0};
	mtkd_t mtkd0={0};
	obsd_t data0={{0}};
	eph_t  eph0 ={0,-1,-1};
	alm_t  alm0 ={0,-1};
	geph_t geph0={0,-1};
	int i,j,sys;

	trace(3,"init_mtkraw:\n");

	mtkraw->clctime=0;
	mtkraw->time=mtkraw->tobs=time0;
	mtkraw->ephsat=0;
	mtkraw->sync=0;
	mtkraw->msgtype[0]='\0';
	for (i=0;i<MAXSAT;i++) 
	{
		for (j=0;j<380  ;j++) mtkraw->subfrm[i][j]=0;
		 mtkraw->lockt[i]=0.0;
		 mtkraw->halfc[i]=0;
		 mtkraw->slipc[i]=0;
	}

	mtkraw->len=0;
	mtkraw->outtype=0;
	mtkraw->obs.data =NULL;
	mtkraw->nav.eph  =NULL;
	mtkraw->nav.alm  =NULL;
	mtkraw->nav.geph =NULL;
	mtkraw->nav.seph =NULL;
	mtkraw->queue.memp=NULL;

	if ((!(mtkraw->mtkobs.data =(mtkd_t *)malloc(sizeof(mtkd_t)*MAXOBS))||
		!(mtkraw->obs.data =(obsd_t *)malloc(sizeof(obsd_t)*MAXOBS))||
		!(mtkraw->nav.eph  =(eph_t  *)malloc(sizeof(eph_t )*MAXSAT))||
		!(mtkraw->nav.alm  =(alm_t  *)malloc(sizeof(alm_t )*MAXSAT))||
		!(mtkraw->nav.geph =(geph_t *)malloc(sizeof(geph_t)*NSATGLO)))) {
			free_mtkraw(mtkraw);
			return 0;
	}
	mtkraw->mtkobs.n=0;
	mtkraw->mtkobs.nmax=MAXOBS;
	mtkraw->obs.n =0;
	mtkraw->obs.nmax=MAXOBS;
	mtkraw->nav.n =MAXSAT;
	mtkraw->nav.ng=NSATGLO;

	for (i=0;i<MAXOBS   ;i++) mtkraw->obs.data [i]=data0;
	for (i=0;i<MAXOBS   ;i++) mtkraw->mtkobs.data[i]=mtkd0;
	for (i=0;i<MAXSAT   ;i++) mtkraw->nav.eph  [i]=eph0;
	for (i=0;i<MAXSAT   ;i++) mtkraw->nav.alm  [i]=alm0;
	for (i=0;i<NSATGLO  ;i++) mtkraw->nav.geph [i]=geph0;

	for(i=0;i<4;i++) mtkraw->nav.utc_cmp[i]=0.0;

	for (i=0;i<MAXSAT;i++) for (j=0;j<NFREQ;j++) {
		if (!(sys=satsys(i+1,NULL))) continue;
		mtkraw->nav.lam[i][j]=sys==SYS_GLO?lam_glo[j]:lam_carr[j];
	}
	queue_buf_Init(&mtkraw->queue,MAXRAWLEN,sizeof(char));
	
	return 1;
}
/* free receiver raw data control ----------------------------------------------
* free observation and ephemeris buffer in receiver raw data control struct
* args   : mtkraw_t  *mtkraw   IO     receiver raw data control struct
* return : none
*-----------------------------------------------------------------------------*/
extern void free_mtkraw(mtkraw_t *mtkraw)
{
	trace(3,"free_mtkraw:\n");

	free(mtkraw->mtkobs.data ); mtkraw->mtkobs.data =NULL; mtkraw->mtkobs.n =0;
	free(mtkraw->obs.data ); mtkraw->obs.data =NULL; mtkraw->obs.n =0;
	free(mtkraw->nav.eph  ); mtkraw->nav.eph  =NULL; mtkraw->nav.n =0;
	free(mtkraw->nav.alm  ); mtkraw->nav.alm  =NULL; mtkraw->nav.na=0;
	free(mtkraw->nav.geph ); mtkraw->nav.geph =NULL; mtkraw->nav.ng=0;
	queue_buf_Free(&mtkraw->queue);
}


/* input mtk raw data from file ----------------------------------------------
* fetch next receiver raw data and input a message from file
* args   : mtkraw_t  *mtkraw   IO     receiver raw data control struct
*          FILE      *fp       I      file pointer
* return : status(-2: end of file/format error, -1...31: same as above)
*-----------------------------------------------------------------------------*/
extern int input_mtkrawf(mtkraw_t *mtkraw, FILE *fp)
{
	int i, data,nbyte=0,end=0,res=0;
	unsigned char buff[1024];

	trace(4,"input_mtkrawf: \n");

	for(i=0;i<1024;i++)
	{
		if((data=fgetc(fp))==EOF) 
		{
			if(mtkraw->queue.size>0) {end=1;break;}
			else return -2;
		}
		buff[nbyte++]=(unsigned char)data;
	}
	queue_buf_Input(&mtkraw->queue,buff,nbyte);

	/* decode MTK raw data */
	res=decode_mtk(mtkraw);

	if(end)
	{
		if(res>0) return res;
		else return -2;
	}
	else return res;
}
