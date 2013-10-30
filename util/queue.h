/*
 * queue.h
 *
 *  Created on: 2013年7月19日
 *      Author: yangfan
 */

#ifndef QUEUE_H_
#define QUEUE_H_

typedef struct tagQUEUE_NODE QUEUE_NODE;

typedef struct tagQUEUE QUEUE;

//返回1表示将pstNow从队列中删除，返回0表示没有删除
typedef int (*Walk_Fn)(const void *pstPre, const void *pstNow);

typedef void (*Free_Fn)(void *);

int QUEUE_IsEmpty(QUEUE *pstQueue);
QUEUE *QUEUE_Init();
void QUEUE_Destroy(QUEUE *pstQueue, Free_Fn func);
int QUEUE_In(QUEUE *pstQueue, void *pNode);
void *QUEUE_Out(QUEUE *pstQueue);
void *QUEUE_Peek(QUEUE *pstQueue);

#endif /* QUEUE_H_ */
