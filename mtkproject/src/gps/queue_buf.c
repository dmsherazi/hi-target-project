/* Note: queue_buf.h cannot be applied to loading stucture data of 
  the variable capacity,like mtkobs_t in mtk.h #YJCHEN@20170707  */

#include "queue_buf.h"
#include <stdlib.h>

extern queue_buf_t *queue_buf_New(int elementnum, int elementsize)
{
    queue_buf_t *q = (queue_buf_t *)malloc(sizeof (queue_buf_t));
    q->capacity = elementnum * elementsize;
    q->elementsize = elementsize;
    q->elementnum = elementnum;
    q->memp = (unsigned char *)malloc(q->capacity);
    memset(q->memp, 0, elementnum * elementsize);
    q->front = 0;
    q->rear = 0;
    q->size = 0;
    return q;
}

extern void queue_buf_Destroy(queue_buf_t *q)
{
    if (q == NULL) {
        return;
    }
    free(q->memp);
    free(q);
    q = NULL;
}

extern void  queue_buf_Init   (queue_buf_t *q,int elementnum, int elementsize)
{
	q->capacity = elementnum * elementsize;
	q->elementsize = elementsize;
	q->elementnum = elementnum;
	q->memp = (unsigned char *)malloc(q->capacity);
	memset(q->memp, 0, elementnum * elementsize);
	q->front = 0;
	q->rear = 0;
	q->size = 0;
//	eat_trace("queue_init: size=%d.\n",q->size);
}

extern void  queue_buf_Free   (queue_buf_t *q)
{
	if(q->memp) {free(q->memp);q->memp=NULL;}
	q->front = 0;
	q->rear = 0;
	q->size = 0;
}

extern int  queue_buf_IsFull (queue_buf_t *q)
{
    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
        return -2;
    }
    if (q->size + 1 > q->elementnum) {
        return 1;
    }
    return 0;
}

extern int  queue_buf_IsEmpty(queue_buf_t *q)
{
    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
        return -2;
    }
    if (q->size == 0) {
        return 1;
    }
    return 0;
}

extern int  queue_buf_Peek   (queue_buf_t *q, int  index, void *data)//取出队列中一个消息
{
	int i;
	unsigned char *tmp = (unsigned char *) data;

    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
		eat_trace("handler is null!\n");
        return -2;
    }
    if (q->size <= index) {
        pr_dbgerr("index out of bound, current size(%d), index(%d)\n",
                q->size, index);
		//eat_trace("index out of bound, current size(%d), index(%d)\n",
        //        q->size, index);
        return -1;
    }
    for (i = 0; i < q->elementsize; i++) {
        tmp[i] = q->memp[(q->rear + index) % q->elementnum * q->elementsize + i];
    }
    return 0;
}

extern int  queue_buf_Output (queue_buf_t *q, void *buffer, int size)
{
	unsigned char *tmp = (unsigned char *) buffer;
	int i, j;

//	eat_trace("queue_buf_Output(1): size=%d.\n",q->size);

    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
        return -2;
    }
    if (q->size - size < 0) {
        pr_dbgerr("no enough data, current size(%d), read size(%d)\n",
                q->size, size);       
        return -1;
    }
    if (buffer == NULL && size != 0) {
        pr_dbgfn("dump %d bytes.\n", size);
        q->rear = (q->rear + size) % q->elementnum;
        q->size -= size;
        return 0;
    }
    for (i = 0; i < size; i++) {
        for (j = 0; j < q->elementsize; j++) {
            tmp[j + i * q->elementsize] = q->memp[(q->rear) * q->elementsize + j];
        }
        q->rear = (q->rear + 1) % q->elementnum;
    }
    q->size -= size;

//	eat_trace("queue_buf_Output(2): size=%d.\n",q->size);

    return 0;
}

/* Note: once the size of the new buffer is more than the queue residual capacity , 
  delete some old data  to make room for loading the new data, so it cannot be applied 
  to loading stucture data structure of the variable capacity,like mtkobs_t in mtk.h */
extern int  queue_buf_Input  (queue_buf_t *q, void *buffer, int size)
{
	unsigned char *tmp = (unsigned char *) buffer;
	int i, j;
    int flag=0;
    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
		eat_trace("null");
        return -2;
    }
    if (q->size + size > q->elementnum) {
        pr_dbgerr("too much data, current size(%d), input size(%d)\n",
                q->size / q->elementsize, size);
		eat_trace("too much data, current size(%d), input size(%d)\n");
        flag=1;                                    
        q->rear=(q->front+size+1)%q->elementnum; 
    }
    for (i = 0; i < size; i++) {
        for (j = 0; j < q->elementsize; j++) {
            q->memp[(q->front) * q->elementsize + j] = tmp[j + i * q->elementsize];
			//eat_trace("front:%ld,memp[%d]:%s",q->front,j,q->memp[j]);
        }
        q->front = (q->front + 1) % q->elementnum;
		
    }
    q->size += size;
    if(flag) q->size=q->elementnum;

//	eat_trace("queue_buf_Input: size=%d.\n",q->size);

    return flag;
}

extern int  queue_buf_Copy   (queue_buf_t *q, int start, int size, void *buffer)
{
    int i, j;
	unsigned char *tmp = (unsigned char *) buffer;

    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
        return -2;
    }
    if (q->size < size + start) {
        pr_dbgwarn("request more data(%d) than available(%d at %d).\n",
                size, q->size, start);
        return -1;
    }  
    for (i = 0; i < size; i++) {
        for (j = 0; j < q->elementsize; j++) {
            tmp[j + i * q->elementsize] = q->memp[(q->rear + start + i) % q->elementnum * q->elementsize + j];
        }
    }
    return 0;
}

extern int  queue_buf_Peek_rv(queue_buf_t *q, void *data, int index)
{
	unsigned char *tmp = (unsigned char *) data;
    if (q == NULL) {
        pr_dbgerr("handler is null!\n");
        return -2;
    }
    if (q->size <= index) {
        pr_dbgerr("index out of bound, current size(%d), index(%d)\n",
                q->size, index);
        return -1;
    }  
    queue_buf_Peek(q, q->size - (index + 1), tmp);

    return 0;
}


