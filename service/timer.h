/*
 * timer.h
 *
 *  Created on: 2013年9月12日
 *      Author: yangfan
 */

#ifndef TIMER_H_
#define TIMER_H_

struct tag_job
{
	void *content;
	unsigned int tick;
	unsigned short enabled;
};
typedef struct tag_job Job_Node;

int timer_init();
void timer_fini();
Job_Node *add_job(void *content, int sec);
void alarm_handler(int sig);
#endif /* TIMER_H_ */
