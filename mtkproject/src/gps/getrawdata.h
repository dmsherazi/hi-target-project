#ifndef _GETRAWDATA_H
#define _GETRAWDATA_H
/********************include file*************************/
#include "eat_interface.h"
//#include "mtk.h"
#include "queue_buf.h"
#include "eat_fs_errcode.h" 
#include "eat_fs_type.h" 
#include "eat_fs.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

/*********************************************************/
typedef signed   char      zhd_bool;
typedef unsigned char      RTK_UINT8;
typedef signed   short int RTK_INT16;
typedef unsigned int       RTK_UINT32;
typedef signed   int       RTK_INT32;
typedef double             RTK_FLOAT64;

/***********************宏定义****************8***********/
#define  CMD_ATCGNSPWR_ON           "AT+Cgnspwr=1\r\n"
#define  CMD_ATCGNSPWR_OFF          "AT+CGPIO=0,57,1,0\r\n"
#define  CMD_ATCGNSTST_ON           "AT+CGNSTST=1\r\n"
#define  CMD_ATCGNSTST_OFF          "AT+CGNSTST=0\r\n"
#define  CMD_OUTRAWDATA             "AT+CGNSCMD=0,\"$PMTK876,0*27\"\r\n"
#define  CMD_TURNONDEBUG			"AT+CGNSCMD=0,\"$PMTK299,1*2d\"\r\n"
#define  CMD_EPHEMERIS				"AT+CGNSCMD=0,\"$PMTK668,1*27\"\r\n"
#define  CMD_ATEPHDATA 				"AT+CGNSCMD=0,\"$PMTK668,11*16\"\r\n"

#define 	RAWDATA_BUF_LEN		2048
#define 	MAXFIELD   64           /* max number of fields in a record */
#define 	MAXNMEA    256          /* max length of nmea sentence */

typedef enum
{
	i_SYS_NUL,
	i_SYS_GPS,
	i_SYS_GLO,
	i_SYS_BDS,

}i_SysEnum_t;

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

#if 1
#define  MAX_SAT_GPS 			32
#define  MAX_SAT_BDS 			35 
#define  OBS_NUM         		25
#define  MAXSAT					100
#define  SYS_NUL				0
#define  SYS_GPS				1
#define  SYS_GLO				2
#define  SYS_BDS				3
#endif
/********************************************************/


typedef struct	/* time struct */
{
	RTK_INT16	gps_week;		/* GPS week */
	RTK_FLOAT64 gps_second;		/* GPS week seconds */
} i_gpstime_t;

typedef struct{
	i_gpstime_t time;       /* receiver sampling time (GPST) */
	unsigned char sat ; /* satellite/receiver number */
	unsigned char SNR ; /* signal strength (0.25 dBHz) */
	unsigned char LLI ; /* loss of lock indicator */
	unsigned char code; /* code indicator (CODE_???) */
	double L;           /* observation data carrier-phase (cycle) */
	double P;           /* observation data pseudorange (m) */
	float  D;           /* observation data doppler frequency (Hz) */
	float  ionc;        /* ionosphere correction (unit:m) */
	char   ions;        /* ionosphere source(0:none 1:broadcast 2:sbas) */
	double rs[3];       /* satellite position(m) */
}mtkd_t;

typedef struct
{
	RTK_FLOAT64 x; /* coordinate x [m] */
	RTK_FLOAT64 y; /* coordinate y [m] */
	RTK_FLOAT64 z; /* coordinate z [m] */
}i_xyz_t;

typedef struct
{
	RTK_UINT8 prn;			/* satellite/receiver number */
	RTK_UINT8 sys;			/* system index */
	RTK_UINT8 channel;		/* glonass channel number */
}i_sat_id_t;

typedef struct  
{
	RTK_FLOAT64 L; /* carrier phase measurement in cycle */
	RTK_FLOAT64 P; /* code measurement in meter */
	RTK_FLOAT64 D; /* dopper measurement in Hz (1/s) */
	RTK_UINT8 snr; /* signal to noise ratio (SNR) */
	zhd_bool isL;  /* continuous phase measurement */
	RTK_UINT8 lli; /* loss of lock indicator - 0:OK or not known; 1:Cycle slip possible; 2:Half-cycle ambiguity/slip possible; */
#ifdef _USE_CS_IN_FILTER_
	RTK_FLOAT64 CS;
#endif
}i_obs_channel_t;

typedef struct
{
	i_obs_channel_t l1;
}i_obs_data_t;

typedef struct
{
	i_sat_id_t id;
	i_obs_data_t data;
	i_xyz_t pos;			/* satellite position XYZ */
}i_sat_obs_t;

typedef struct
{
	RTK_UINT8 num_of_sat;	/* number of satellite */
	i_gpstime_t time;			/* receiver sampling time (GPST) */
	i_sat_obs_t obs[OBS_NUM]; /* satellite raw observation in one epoch*/
	i_xyz_t pos;				/* observer position XYZ */
}i_epoch_obs_t;

typedef struct
{
	RTK_UINT8       sys;   /* i_SYS_??? */
	RTK_UINT8		prn;
	RTK_UINT8		uari;
	RTK_FLOAT64		idot;
	RTK_UINT32	    iode;
	i_gpstime_t	    toc;
	RTK_FLOAT64		af0;
	RTK_FLOAT64		af1;
	RTK_FLOAT64		af2;
	RTK_UINT8		iodc;
	RTK_FLOAT64		crs;
	RTK_FLOAT64		delta_n;
	RTK_FLOAT64		m0;
	RTK_FLOAT64		cuc;
	RTK_FLOAT64		e;
	RTK_FLOAT64		cus;
	RTK_FLOAT64		sqrt_a;
	i_gpstime_t	    toe;
	RTK_FLOAT64		cic;
	RTK_FLOAT64		OMEGA0;
	RTK_FLOAT64		cis;
	RTK_FLOAT64		i0;
	RTK_FLOAT64		crc;
	RTK_FLOAT64		omega;
	RTK_FLOAT64		OMEGA_DOT;
	RTK_FLOAT64		tgd;
	RTK_UINT8	    health;
}i_brdc_eph_t;

typedef struct {          /* receiver raw data control type */
	int clctime;          /* local receiver time(unit: ms) */
	i_gpstime_t time;       /* message time */
	i_gpstime_t tobs;       /* observation data time */
	i_epoch_obs_t mtkobs;       /* mtk observation data  */
	i_brdc_eph_t bds_eph[MAX_SAT_BDS];      /* satellite ephemerides */
	i_brdc_eph_t gps_eph[MAX_SAT_GPS];     /* satellite ephemerides */
	int sync;           /* time of obs data valid flag (1:ok,0:not get) */
	int ephsat;         /* sat number of update ephemeris (0:no satellite) */
	char outtype;
	char msgtype[256];   /* last message type */
	double lockt[MAXSAT]; /* lock time (s)   */
	int    slipc[MAXSAT]; /* slip count: 0-999 */
	unsigned char halfc[MAXSAT]; /* half-cycle add flag */
	int len;            /* message length (bytes) */
	queue_buf_t queue;  /* message buffer queue */
}mtkraw_t;

#if 0
/***********************结构体***************************/

#define SOL_TYPE_NUL  0    /* gnss positioning solution type, same to gga */
#define SOL_TYPE_SPP  1  
#define SOL_TYPE_SBS  9
#define SOL_TYPE_RTD  2
#define SOL_TYPE_FLO  5
#define SOL_TYPE_FIX  4

typedef struct
{
	i_gpstime_t time;    /*��epoch time (gpst)   */
	i_xyz_t pos;         /*  position (ecef: m)  */
	i_xyz_t vel;         /*  velocity (ecef: m/s)*/
	RTK_UINT8  soltype;  /*  solution type (SOL_TYPE_???)*/
}i_rtksol_t;



typedef struct{
	i_gpstime_t time;       /* receiver sampling time (GPST) */
	unsigned char sat ; /* satellite/receiver number */
	unsigned char SNR ; /* signal strength (0.25 dBHz) */
	unsigned char LLI ; /* loss of lock indicator */
	unsigned char code; /* code indicator (CODE_???) */
	double L;           /* observation data carrier-phase (cycle) */
	double P;           /* observation data pseudorange (m) */
	float  D;           /* observation data doppler frequency (Hz) */
	float  ionc;        /* ionosphere correction (unit:m) */
	char   ions;        /* ionosphere source(0:none 1:broadcast 2:sbas) */
	double rs[3];       /* satellite position(m) */
}mtkd_t;
#endif
/*********************************************************/
extern int g_bdgsv_count;
extern int g_gpgsv_count;

extern int g_gpsave_count;
extern int g_bdsave_count;


extern unsigned char msg[256];
extern int StartLockFlag;
extern int ReqGPSEphFlag;
extern int ReqBDSEphFlag;
extern char BDS_satnum[MAXFIELD][5];	
extern char GPS_satnum[MAXFIELD][5];
extern char g_pre_gpsnum[MAXFIELD][5];
extern char g_pre_bdsnum[MAXFIELD][5];

/***********************外部函数**************************/
int MTK_GetRawData(void);
int send_cmd_ephe(char *satellitenum,i_SysEnum_t systype);

void delay(u32 ncount);

/*********************************************************/
#endif
