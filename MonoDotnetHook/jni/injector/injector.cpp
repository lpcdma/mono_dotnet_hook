#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include "android-injector.h"

#define WAIT_TIME 500000

int main(int argc, char* argv[])
{
	if (argc<3)
	{
		printf("useage: %s process\n\n", argv[0]);
		return 0;
	}

	char *proc_name = argv[1];
	const char *so_path = argv[2];

	if (access(so_path, F_OK) == -1)
	{
		printf("err %s not exists!\n\n", so_path);
		return 0;
	}

	ANDROID_INJECTOR injector;
	pid_t target_pid = -1;
	do
	{
		target_pid = injector.find_pid_of(proc_name);
		usleep(WAIT_TIME);
		printf("wait for %s...\n", proc_name);
	} while (target_pid == -1);

	int ret = injector.inject_remote_process(target_pid, so_path, NULL, NULL, 0, 0);
	if (ret==-1)
	{
		printf("inject failed!\n\n");
		return 0;
	}
	
	printf("inject success!\n\n");
	return 1;
}
