/*
 * sock_wrapper.c
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#include "wrapper.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

int socket_wrap()
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd == -1)
	{
		perror("error create socket");
		return -1;
	}
	return fd;
}

int bind_wrap(int fd, void *addr, unsigned int len)
{
	int opt = 1;
	if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
	{
		perror("error set socket option");
		return -1;
	}
	if(bind(fd, (struct sockaddr *)addr, len) != 0)
	{
		perror("error bind socket");
		return -1;
	}
	return 0;
}

int listen_wrap(int fd, int num)
{
	if(listen(fd, num) != 0)
	{
		perror("error listen socket");
		return -1;
	}
	return 0;
}

int signal_wrap(int sig, sighandler_t handler)
{
	if(signal(sig, handler) == SIG_ERR)
	{
		perror("error set signal handler");
		return -1;
	}
	return 0;
}
