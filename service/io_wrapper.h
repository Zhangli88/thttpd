/*
 * io_wrapper.h
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#ifndef IO_WRAPPER_H_
#define IO_WRAPPER_H_

#define SYSROUTINE_ERR -1
#define PEER_CLOSED -2

int write_buf(int fd, char *buf, unsigned int len);
int read_buf(int fd, char *buf, unsigned int len);

#endif /* IO_WRAPPER_H_ */
