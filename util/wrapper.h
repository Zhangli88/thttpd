/*
 * sock_wrapper.h
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#ifndef SOCK_WRAPPER_H_
#define SOCK_WRAPPER_H_

typedef void (*sighandler_t)(int);

int socket_wrap();
int bind_wrap(int fd, void *addr, unsigned int len);
int listen_wrap(int fd, int num);
int signal_wrap(int sig, sighandler_t handler);

#endif /* SOCK_WRAPPER_H_ */
