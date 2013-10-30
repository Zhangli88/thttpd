/*
 * thread.h
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#ifndef THREAD_H_
#define THREAD_H_

#define MAX_CONNS 5

#define INTERNAL_ERROR -1
#define UNSUPPORT_METHOD -2
#define UNSUPPORT_VERSION -3
#define NO_SUCH_FILE -4
#define NO_RIGHT -5
#define URL_ERROR -6
#define HEAD_TOOLONG -7

void *do_job(void *arg);

#endif /* THREAD_H_ */
