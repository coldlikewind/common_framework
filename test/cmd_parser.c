#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int parse_and_exec(const char *xcmd);

int
main(int argc, char *argv[])
{
	parse_and_exec("kdcsd1d test set ag");
	parse_and_exec("kdcsd1d test get");
}

static int parse_and_exec(const char *xcmd)
{
	char *uuid = NULL;
	char *cmd =NULL;
	char *arg = NULL;
	
	uuid = strdup(xcmd);
	if ((arg = strchr(uuid, '\r')) != 0 || (arg = strchr(uuid, '\n')) != 0) {
		*arg = '\0';
		arg = NULL;
	}
	
	if ((cmd = strchr(uuid, ' ')) != 0) {
		*cmd++ = '\0';
	}

	if ((arg = strchr(cmd, ' ')) != 0) {
		*arg++ = '\0';
	}	
	
	printf("uuid=%s, cmd=%s, arg=%s \n", uuid, cmd, arg);
	free(uuid);
}
