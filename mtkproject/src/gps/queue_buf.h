/***********************************************************************
 * @ queue_raw.h : MTK Raw Data decode
 *
 * Copyright 2017-2025 R&D, ZHD HI-TARGET, ALL Rights Reserved.
 * http://www.zhdgps.com
 *
 * @author : YJCHEN @20170704
 *********************************************************************/

#ifndef QUEUE_BUFFER_H
#define QUEUE_BUFFER_H

#include <string.h>
#include "eat_interface.h"

/* debug utilities */
#if !defined(pr_dbgwarn)
#define pr_dbgwarn do{}while(0);
#endif

#if !defined(pr_dbgerr)
#define pr_dbgerr do{}while(0);
#endif

#if !defined(pr_dbgfn)
#define pr_dbgfn do{}while(0);
#endif

typedef struct {
	long front; /* this is an index, not the actual offset of data */
	long rear;  /* the same as above */
	long size;  /* the actual number of element in the queue */
	unsigned char *memp;
	long capacity;
	long elementsize;
	long elementnum;
}queue_buf_t;

extern queue_buf_t *queue_buf_New(int elementnum, int elementsize);
extern void queue_buf_Destroy(queue_buf_t *q);
extern void queue_buf_Init   (queue_buf_t *q,int elementnum, int elementsize);
extern void  queue_buf_Free   (queue_buf_t *q);
extern int  queue_buf_IsFull (queue_buf_t *q);
extern int  queue_buf_IsEmpty(queue_buf_t *q);
extern int  queue_buf_Peek   (queue_buf_t *q, int  index, void *data);
extern int  queue_buf_Output (queue_buf_t *q, void *buffer, int size);
extern int  queue_buf_Input  (queue_buf_t *q, void *buffer, int size);
extern int  queue_buf_Copy   (queue_buf_t *q, int start, int size, void *buffer);
extern int  queue_buf_Peek_rv(queue_buf_t *q, void *data, int index);


#endif /*¡¡QUEUE_BUFFER_H */




