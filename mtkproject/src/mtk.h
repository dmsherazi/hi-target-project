/***********************************************************************
 * @ mtk.h 
 *
 * Copyright 2017-2025 R&D, ZHD HI-TARGET, ALL Rights Reserved.
 * http://www.zhdgps.com
 *
 * @author : YJCHEN @20170704
 *********************************************************************/

#ifndef HI_MTK_H
#define HI_MTK_H

#include " zhd_sfrtk_interface.h"
#include "queue_buf.h"

#if 0
typedef struc{
	gtime_t time;       /* receiver sampling time (GPST) */
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

typedef struct {        /* mtk observation data */
	int n,nmax;         /* number of obervation data/allocated */
	mtkd_t *data;       /* observation data records */
}mtkobs_t;

#endif


#define MAX_SAT_GPS 32
#define MAX_SAT_BDS 35 
typedef gpstime_t i_gpstime_t;
typedef sat_id_t i_sat_id_t;
typedef mtkobs_t i_epoch_obs_t;
typedef brdc_eph_t i_brdc_eph_t;


typedef struct {          /* receiver raw data control type */
	int clctime;          /* local receiver time(unit: ms) */
	gpstime_t time;       /* message time */
	gpstime_t tobs;       /* observation data time */
	mtkobs_t mtkobs;       /* mtk observation data  */
	brdc_eph_t bds_eph[MAX_SAT_BDS]      /* satellite ephemerides */
	brdc_eph_t gps_eph[MAX_SAT_GPS]      /* satellite ephemerides */
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

/* extract one message from the data queue --------------------
* args   : queue_t       *q     IO  data queue
*          unsigned char *buff  IO  msg buffer 
*          unsigned int  size   IO  the max size of buff 
*          prcopt_t *opt    I   processing options
* return : msglen
*--------------------------------------------------------------*/
extern int GetOneMsg(queue_buf_t *q, unsigned char *buff,  int size);

/* decode MTK RAW data */
extern int decode_mtk(mtkraw_t *mtkraw);

/* initialize mtk raw data control ----------------------------------------
* initialize mtk raw data control struct and reallocate obsevation and
* epheris buffer
* args   : mtkraw_t  *mtkraw   IO     mtk raw data control struct
* return : status (1:ok,0:memory allocation error)
*-----------------------------------------------------------------------------*/
extern int init_mtkraw(mtkraw_t *mtkraw);
/* free receiver raw data control ----------------------------------------------
* free observation and ephemeris buffer in receiver raw data control struct
* args   : mtkraw_t  *mtkraw   IO     receiver raw data control struct
* return : none
*-----------------------------------------------------------------------------*/
extern void free_mtkraw(mtkraw_t *mtkraw);

extern int input_mtkrawf(mtkraw_t *mtkraw, FILE *fp);

extern int CheckSUM(char *buffer);
extern int GetCheckSUM(char *buffer);

#endif  /* HI_MTK_H */

