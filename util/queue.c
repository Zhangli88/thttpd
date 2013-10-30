/*
 * queue.c
 *
 *  Created on: 2013年7月19日
 *      Author: yangfan
 */

#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>

#define RETURN_ON_NULL(ptr, ret) \
if(ptr == NULL) return ret

struct tagQUEUE_NODE
{
	void *pContent;
	struct tagQUEUE_NODE *pNext;
};

struct tagQUEUE
{
	pthread_mutex_t mutex;
	QUEUE_NODE *pstHead;
	QUEUE_NODE *pstTail;
};

int QUEUE_IsEmpty(QUEUE *pstQueue)
{
	assert(pstQueue != NULL);
	int ret;
	pthread_mutex_lock(&pstQueue->mutex);
	if(pstQueue->pstTail == NULL && pstQueue->pstHead == NULL)
		ret = 0;
	else
		ret = 1;
	pthread_mutex_unlock(&pstQueue->mutex);
	return ret;
}

QUEUE *QUEUE_Init()
{
	QUEUE *pstQueue = malloc(sizeof(QUEUE));
	RETURN_ON_NULL(pstQueue, NULL);
	pthread_mutex_init(&pstQueue->mutex, NULL);
	pstQueue->pstHead = NULL;
	pstQueue->pstTail = NULL;
	return (QUEUE *)pstQueue;
}

void QUEUE_Destroy(QUEUE *pstQueue, Free_Fn func)
{
	assert(pstQueue != NULL);
	QUEUE_NODE *pstNode, *pstNext;

	pthread_mutex_lock(&pstQueue->mutex);
	pstNode = pstQueue->pstHead;
	while(pstNode != NULL) // right for a empty queue
	{
		pstNext = pstNode->pNext;
		func(pstNode->pContent);
		free(pstNode);
		pstNode = pstNext;
	}
	pthread_mutex_unlock(&pstQueue->mutex);
	pthread_mutex_destroy(&pstQueue->mutex);
	free(pstQueue);
	return;
}

int QUEUE_In(QUEUE *pstQueue, void *pNode)
{
	assert(pstQueue != NULL);
	QUEUE_NODE *pstQnode = malloc(sizeof(QUEUE_NODE));
	RETURN_ON_NULL(pstQnode, -1);
	pstQnode->pContent = pNode;
	pstQnode->pNext = NULL;

	pthread_mutex_lock(&pstQueue->mutex);
	if(QUEUE_IsEmpty(pstQueue) == 1) // this is the first node
	{
		pstQueue->pstHead = pstQnode;
		pstQueue->pstTail = pstQnode;
	}
	else
	{
		pstQueue->pstTail->pNext = pstQnode;
		pstQueue->pstTail = pstQnode;
	}
	pthread_mutex_unlock(&pstQueue->mutex);
	return 0;
}

void *QUEUE_Out(QUEUE *pstQueue)
{
	assert(pstQueue != NULL);
	QUEUE_NODE *pstNode = NULL;
	void *pstRet = NULL;

	pthread_mutex_lock(&pstQueue->mutex);
	pstNode = pstQueue->pstHead;
	if(pstNode != NULL) // do nothing if an empty queue
	{
		if(pstNode == pstQueue->pstTail) // this is the last node
		{
			pstQueue->pstTail = NULL;
		}
		pstRet = pstNode->pContent;
		pstQueue->pstHead = pstNode->pNext;
		free(pstNode);
	}
	pthread_mutex_unlock(&pstQueue->mutex);
	return pstRet;
}

void *QUEUE_Peek(QUEUE *pstQueue)
{
	assert(pstQueue != NULL);
	QUEUE_NODE *pstTemp = NULL;

	pthread_mutex_lock(&pstQueue->mutex);
	pstTemp = pstQueue->pstHead;
	pthread_mutex_unlock(&pstQueue->mutex);

	return pstTemp == NULL ? NULL : pstTemp->pContent;
}

void QUEUE_Walk(QUEUE *pstQueue, Walk_Fn func)
{
	assert(pstQueue != NULL);
	QUEUE_NODE *pstNow, *pstPre, *pstNext;

	if(QUEUE_IsEmpty(pstQueue) == 1)
		return;

	pthread_mutex_lock(&pstQueue->mutex);
	pstPre = pstQueue->pstHead;
	func(NULL, pstPre);
	pstNow = pstPre->pNext;
	while(pstNow != NULL)
	{
		pstNext = pstNow->pNext;
		if(func(pstPre, pstNow) == 0)
			pstPre = pstNow;
		pstNow = pstNext;
	}
	pthread_mutex_unlock(&pstQueue->mutex);
	return;
}
