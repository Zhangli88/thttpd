/*
 * io_wrapper.c
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#include "io_wrapper.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>

int write_buf(int fd, char *buf, unsigned int len)
{
	unsigned int i = 0,total = 0;
	while(1)
	{
		i = write(fd, buf+total, len-total);
		if(i > 0)
		{
			total += i;
			if(total == len)
				break;
		}
		else
		{
			if(i == 0)
				return PEER_CLOSED;
			else
			{
				if(errno == EAGAIN || errno == EWOULDBLOCK)
					continue;
				else if(errno == EINTR)
					pause();
				else
					return SYSROUTINE_ERR;
			}
		}
	}
	return 0;
}

int read_buf(int fd, char *buf, unsigned int len)
{
	int err;
	while((err = read(fd, buf, len)))
	{
		if(err > 0)
			break;
		else if(err == 0)
			return PEER_CLOSED;
		else
		{
			if(errno == EAGAIN || errno == EWOULDBLOCK)
				continue;
			else
				return SYSROUTINE_ERR;
		}
	}
	return err;
}
