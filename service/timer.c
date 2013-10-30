/*
 * timer.c
 *
 *  Created on: 2013年9月12日
 *      Author: yangfan
 */

#include "timer.h"
#include "../util/queue.h"
#include "../util/wrapper.h"
#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>

static QUEUE *g_queue;
static unsigned int g_flag;
extern int timer_base;

void alarm_handler(int sig)
{
	g_flag = 1;
	printf("%s\n", "alarm beep");
}

Job_Node *add_job(void *content, int sec)
{
	Job_Node *node = malloc(sizeof(Job_Node));
	if(node == NULL)
		return NULL;
	node->content = content;
	node->tick = sec / timer_base;
	node->enabled = 1;
	QUEUE_In(g_queue, node);
	return node;
}

static int init_queue()
{
	if(g_queue != NULL)
		return 0;

	g_queue = QUEUE_Init();
	if(g_queue == NULL)
	{
		fprintf(stderr, "%s\n", "error creating queue");
		return -1;
	}
	if(signal_wrap(SIGALRM, alarm_handler) == -1)
	{
		fprintf(stderr, "%s\n", "error create timer");
		return -1;
	}
	return 0;
}

void timer_fini()
{
	if(g_queue != NULL)
		QUEUE_Destroy(g_queue, free);
}

int timer_init()
{
	struct itimerval val;

	if(init_queue() != 0)
		return -1;

	val.it_interval.tv_sec = timer_base;
	val.it_interval.tv_usec = 0;
	val.it_value = val.it_interval;
	if(setitimer(ITIMER_REAL, &val, NULL) < 0)
	{
		perror("error create timer\n");
		return -1;
	}
	return 0;
}
