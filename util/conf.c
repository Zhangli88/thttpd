/*
 * conf.c
 *
 *  Created on: 2013年9月7日
 *      Author: yangfan
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_LINE 128

static FILE *file;

void init_conf(char *name)
{
	if(name == NULL)
		name = "thttpd.conf";
	file = fopen(name, "r");
	if(file == NULL)
	{
		perror("error open conf file!");
		exit(1);
	}
}

int read_opt(char *opt, char *value)
{
	assert(file != NULL);
	char buf[MAX_LINE];
	char *begin,*end;
	value[0] = '\0';
	while(fgets(buf, sizeof(buf), file) != NULL)
	{
		if(strstr(buf, opt))
		{
			begin = strchr(buf, '=');
			end = strchr(buf, '\n');
			*end = '\0';
			if(begin == NULL || end == NULL || end <= begin + 1)
				return -1;
			strcpy(value, begin + 1);
			return 0;
		}
	}
	return -1;
}

void fini_conf()
{
	fclose(file);
}
