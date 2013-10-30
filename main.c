/*
 * main.c
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#include "util/conf.h"
#include "util/wrapper.h"
#include "service/thread.h"
#include "service/timer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

char const *docRoot;
char const *cgiRoot;
int ka_timeout;
int ka_max;
int timer_base;

static void sigsegv_handler(int sig)
{
	fprintf(stderr, "%s\n", "Received SIGSEGV");
	exit(1);
}

static int gcd(int a, int b)
{
	int min,max,t;
	if(a > b)
	{
		max = a;
		min = b;
	}
	else
	{
		max = b;
		min = a;
	}
	while(min != 0)
	{
		t = min;
		min = max % min;
		max = t;
	}
	return max;
}

int main(int argc, char *argv[])
{
	char buf[50];
	int port, err;
	struct sockaddr_in servaddr, cliaddr;
	socklen_t cliaddr_len;
	int listenfd, connfd;
	pthread_t tid;

	init_conf(NULL);
	if(read_opt("PORT", buf) == -1)
		port = 80;
	else
		port = atoi(buf);
	printf("Use port:%i\n", port);

	if(read_opt("DOCUMENTROOT", buf) == 0)
	{
		printf("DocumentRoot:%s\n", buf);
		docRoot = malloc(strlen(buf)+1);
		strcpy((char *)docRoot, buf);
	}

	if(read_opt("CGIROOT", buf) == 0)
	{
		printf("CGIRoot:%s\n", buf);
		cgiRoot = malloc(strlen(buf)+1);
		strcpy((char *)cgiRoot, buf);
	}

	if(read_opt("KeepAliveTimeout", buf) == 0)
	{
		printf("KeepAliveTimeout:%s\n", buf);
		ka_timeout = atoi(buf);
		if(ka_timeout < 0)
		{
			fprintf(stderr, "%s\n", "KeepAliveTimeout < 0!");
			exit(1);
		}
	}

	if(read_opt("KeepAliveMax", buf) == 0)
	{
		printf("KeepAliveMax:%s\n", buf);
		ka_max = atoi(buf);
		if(ka_max < ka_timeout)
		{
			fprintf(stderr, "%s\n", "KeepAliveMax < KeepAliveTimeout!");
			exit(1);
		}
	}

	timer_base = gcd(ka_timeout, ka_max);

	fini_conf();

	listenfd = socket_wrap();

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);

	if((bind_wrap(listenfd, &servaddr, sizeof(servaddr)) != 0)
		||(listen_wrap(listenfd, MAX_CONNS) != 0)
		||(signal_wrap(SIGPIPE, SIG_IGN) != 0)
		||(signal_wrap(SIGSEGV, sigsegv_handler) != 0)
		||(timer_init() != 0))
		goto exit;

	printf("start serving ...\n");
	while(1)
	{
		cliaddr_len = sizeof(cliaddr);
		connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
		printf("accept connection at %s,fd:%i\n", inet_ntoa(cliaddr.sin_addr), connfd);
		err = pthread_create(&tid, NULL, do_job, (void *)connfd);
		if(err != 0)
		{
			fprintf(stderr, "can't create thread: %s\n", strerror(err));
			goto exit;
		}
		pthread_detach(tid);
	}

exit:
	timer_fini();
	free((char *)docRoot);
	free((char *)cgiRoot);
	return 0;
}
