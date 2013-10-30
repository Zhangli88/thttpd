/*
 * thread.c
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#include "thread.h"
#include "io_wrapper.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include <signal.h>

#define MAX_LINE 256
#define HEAD_END_BACKWARD(p) \
		((p)[-3] == '\r' && (p)[-2] == '\n' && (p)[-1] == '\r' && (p)[0] == '\n')
#define HEAD_END_FORWARD(p) \
		((p)[0] == '\r' && (p)[1] == '\n' && (p)[2] == '\r' && (p)[3] == '\n')

extern char **environ;
extern char const *docRoot;
extern char const *cgiRoot;
extern int ka_timeout;
extern int ka_max;

static unsigned short g_count;

static const char const *mime_table[2][9] =
{
	{"html,htm", "css",     "js",
	 "txt",      "xml",     "xhtml",
	 "png",      "gif",     "jpeg,jpg"},

	{"text/html", "text/css", "application/x-javascript",
	 "text/plain","text/xml", "application/xhtml+xml",
	 "image/png", "image/gif","image/jpeg"}
};

typedef struct tag_request
{
	char method_name[8];
	char url[256];
	char version[16];
	char host[256];
	unsigned int contentLength;
	short isKeepAlive;
	short isPost;
}Request;

static int parse_request(char *buf, Request *p)
{
	int i;
	char *begin = buf, *end, *c;

	memset(p, 0, sizeof(Request));

	end = strchr(begin, ' ');
	memcpy(p->method_name, begin, end-begin);
	for(i = 0, c = p->method_name; c[i] != '\0'; c[i] = toupper(c[i]), i++);
	if(strcmp(p->method_name, "POST") == 0)
		p->isPost = 1;

	begin = end + 1;//skip space
	end = strchr(begin, ' ');
	memcpy(p->url, begin, end-begin);

	begin = end + 1;//skip space
	end = strchr(begin, '\r');
	memcpy(p->version, begin, end-begin);
	for(i = 0, c = p->version; c[i] != '\0'; c[i] = toupper(c[i]), i++);

	while(!HEAD_END_FORWARD(end))
	{
		begin = end + 2;//skip \r\n 指向下一行的开头
		if((end = strstr(begin, "Host: ")) != NULL)
		{
			begin = end + strlen("Host: ");
			end = strchr(begin, '\r');
			memcpy(p->host, begin, end-begin);
			continue;
		}

		if((end = strstr(begin, "Connection: ")) != NULL)
		{
			begin = end + strlen("Connection: ");
			end = strchr(begin, '\r');
			p->isKeepAlive = 1;
			continue;
		}

		if((end = strstr(begin, "Content-Length: ")) != NULL)
		{
			begin = end + strlen("Content-Length: ");
			end = strchr(begin, '\r');
			p->contentLength = atoi(begin);
			continue;
		}
		end = strchr(begin, '\r');//end指向\r，判断是否读到请求头结束(\r\n\r\n)
	}
	return 0;
}

static int read_request(int fd, Request *p)
{
	int err,i = 0,len;
	char head[1024], t;

	len = sizeof(head);
	head[0] = '\0';
	while(i < len && (err = read_buf(fd, &t, 1)) > 0)
	{
		if(err < 0) return INTERNAL_ERROR;
		head[i++] = t;
		if(HEAD_END_BACKWARD(&head[i-1]))
			break;
	}

	if(err < 0) return INTERNAL_ERROR;
	if(i >= len) return HEAD_TOOLONG;
	head[i] = '\0';

	err = parse_request(head, p);
	if(err != 0) return err;
	return 0;
}

static int verify_request(Request *p)
{
	if(strcmp(p->version, "HTTP/1.1") != 0)
		return UNSUPPORT_VERSION;

	if(strcmp(p->method_name, "GET") != 0 && strcmp(p->method_name, "POST") != 0)
		return UNSUPPORT_METHOD;

	return 0;
}

static void error_page(int fd, int errcode, char *msg)
{
	assert(strlen(msg) < 400);
	char head[256],body[512];
	head[0] = '\0';
	body[0] = '\0';
	int bodyLen,headLen;

	strcat(body, "<html><title>Error</title>");
	strcat(body, "<body bgcolor='ffffff'>\r\n");
	sprintf(body, "%s%s%s%s", body, "<h1 color='red'>", msg, "</h1>");
	strcat(body, "</body></html>");
	bodyLen = strlen(body);

	sprintf(head, "%s %u\r\n", "HTTP/1.1", errcode);
	sprintf(head, "%s%s\r\n", head, "Content-type: text/html");
	sprintf(head, "%s%s%u\r\n", head, "Content-length: ", bodyLen);
	strcat(head, "\r\n");

	headLen = strlen(head);
	write_buf(fd, head, headLen);
	write_buf(fd, body, bodyLen);
	return;
}

static void handle_error(int fd, int err)
{
	switch(err)
	{
		case INTERNAL_ERROR:
			error_page(fd, 500, "Internal Error");
			break;
		case UNSUPPORT_VERSION:
			error_page(fd, 500, "Unsupport HTTP Version");
			break;
		case UNSUPPORT_METHOD:
			error_page(fd, 500, "Unsupport HTTP Method");
			break;
		case NO_SUCH_FILE:
			error_page(fd, 404, "No Such File");
			break;
		case NO_RIGHT:
			error_page(fd, 401, "No Enough Right");
			break;
		case URL_ERROR:
			error_page(fd, 500, "URL Error");
			break;
		case HEAD_TOOLONG:
			error_page(fd, 500, "Request Head Too Long");
			break;
		default:
			assert(0);
			break;
	}
}

static int parse_url_static(char *url, char *path)
{
	char name[512];
	path[0] = '\0';
	name[0] = '\0';
	int i;
	char *defaults[] = {"index.html", "index.htm", "default,html", "default,htm"};

	if(url[strlen(url) - 1] == '/')//如果以/结尾，认为是文件夹，则以默认文件名找文件
	{
		for(i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++)
		{
			strcat(name, docRoot);
			strcat(name, url);
			strcat(name, defaults[i]);
			if(access(name, F_OK) == 0)
			{
				strcpy(path, name);
				return 0;
			}
			name[0] = '\0';
		}
	}
	else//如果不是/结尾，则按文件名直接查找
	{
		strcat(name, docRoot);
		strcat(name, url);
		if(access(name, F_OK) == 0)
		{
			strcpy(path, name);
			return 0;
		}
	}
	printf("parse url:%s FAIL!\n", name);
	perror("parse url");
	return NO_SUCH_FILE;
}

static int parse_url_dynamic(char *url, char *path, char *query)
{
	char name[512];
	path[0] = '\0';
	query[0] = '\0';
	name[0] = '\0';
	char *begin, *end;
	int len;

	begin = strstr(url, "cgi-bin");
	if(begin == NULL) return URL_ERROR;

	begin += strlen("cgi-bin");
	end = strchr(begin, '?');
	if(end == NULL)
		end = strchr(begin, '\0');

	if(end == NULL || end <= begin + 1) return URL_ERROR;

	len = strlen(cgiRoot);
	strcat(name, cgiRoot);
	memcpy(name + len, begin, end - begin);
	len += (end - begin);
	name[len] = '\0';

	if(access(name, F_OK) == 0)
	{
		strcpy(path, name);
		strcpy(query, end);
		return 0;
	}
	return NO_SUCH_FILE;
}

//判断依据：url中是否有cgi-bin子串
static int static_request(char *url)
{
	if(strstr(url, "cgi-bin") != NULL)
		return 0;
	return 1;
}

static void parse_mime(char *path, char *mime)
{
	char suffix[16];
	char *c = strrchr(path, '.');
	int i,len;
	if(c != NULL && *(c+1) != '\0')
	{
		strcpy(suffix, c+1);
		len = sizeof(mime_table[0]) / sizeof(mime_table[0][0]);
		for(i = 0; i < len; i++)
		{
			if(strstr((mime_table[0][i]), suffix) != NULL)
				break;
		}
		strcpy(mime, mime_table[1][i]);
		return;
	}
	strcpy(mime, "text/plain");
	return;
}

static int response_static(int fd, char *path)
{
	unsigned long filesize = -1;
	char head[256],mime[32],body[1024];
	struct stat statbuf;
	FILE *file;
	int read;

	if(stat(path, &statbuf) < 0 || ((statbuf.st_mode & S_IRUSR) == 0))
	{
		return NO_RIGHT;
	}
	filesize = statbuf.st_size;
	parse_mime(path, mime);

	sprintf(head, "%s %i\r\n", "HTTP/1.1", 200);
	sprintf(head, "%s%s%s\r\n", head, "Content-type: ", mime);
	sprintf(head, "%s%s%lu\r\n", head, "Content-length: ", filesize);
	strcat(head, "\r\n");
	write_buf(fd, head, strlen(head));

	file = fopen(path, "r");
	assert(file != NULL);
	while((read = fread(body, sizeof(body[0]), sizeof(body)/sizeof(body[0]), file)) > 0)
	{
		write_buf(fd, body, read);
	}
	fclose(file);
	return 0;
}

static int response_dynamic(int fd, char *path, char *query, Request *p)
{
	struct stat statbuf;
	char lenstr[12];
	char *argv[] = {NULL};
	pid_t pid;

	sprintf(lenstr, "%u", p->contentLength);
	if(stat(path, &statbuf) < 0 || ((statbuf.st_mode & S_IRUSR) == 0)
	   || ((statbuf.st_mode & S_IXUSR) == 0))
	{
		return NO_RIGHT;
	}

	pid = fork();
	if(pid == 0)
	{
		setenv("REQUEST_METHOD", p->method_name, 1);
		setenv("QUERY_STRING", query, 1);
		setenv("REMOTE_HOST", p->host, 1);
		if(p->isPost == 1)
			setenv("CONTENT_LENGTH", lenstr, 1);
		printf("exec cgi:%s\n", path);
		dup2(fd, STDOUT_FILENO);
		dup2(fd, STDIN_FILENO);
		execve(path, argv, environ);
	}
	else if(pid > 0)
	{
		int ret;
		waitpid(pid, &ret, 0);
		if (WIFEXITED(ret))
			printf("cgi:%s finished!\n", path);
	}
	else return INTERNAL_ERROR;
	return 0;
}

void *do_job(void *arg)
{
	printf("No.%u connections\n", g_count++);
	int fd = (int)arg;
	char path[MAX_LINE],query[MAX_LINE];
	Request req;
	int err;

begin:
	err = read_request(fd, &req);
	if(err != 0) goto error;

	err = verify_request(&req);
	if(err != 0) goto error;

	printf("request:%s\n", req.url);
	if(static_request(req.url) == 1)//请求静态文件
	{
		err = parse_url_static(req.url, path);
		if(err != 0) goto error;

		err = response_static(fd, path);
		if(err != 0) goto error;
	}
	else//执行cgi脚本
	{
		err = parse_url_dynamic(req.url, path, query);
		if(err != 0) goto error;

		err = response_dynamic(fd, path, query, &req);
		if(err != 0) goto error;
	}

	if(req.isKeepAlive == 1)
	{
		goto begin;
	}

error:
	handle_error(fd, err);
	printf("close socket %i\n", fd);
	close(fd);
	return NULL;
}
