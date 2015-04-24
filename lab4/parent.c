
//iterations
//process type
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sched.h>
#include <math.h>
#include <sys/wait.h>

extern char** environ;
FILE* inputfp = NULL;

void doCpu(char* schedPol, int numChildren, char* iterations) {
	struct sched_param param;
	int policy;
	int i;
	char* newArgv[3];
	newArgv[0] = "pi\0";
	newArgv[1] = iterations;
	newArgv[2] = NULL;

	if(!strcmp("SCHED_FIFO", schedPol)) {
		policy = SCHED_FIFO;
	}
	else if (!strcmp("SCHED_RR", schedPol)) {
		policy = SCHED_RR;
	}
	else {
		policy = SCHED_OTHER;
	}
	param.sched_priority = sched_get_priority_max(policy);
	sched_setscheduler(0, policy, &param);
	
	for(i = 0; i < numChildren; i++) {
		if(fork() == 0) {
			if(execve("pi", newArgv, environ) < 0) {
				printf("execve failed!\n");
				exit(1);
			}
		}
	}
	while(waitpid(-1, NULL, 0) > 0) {
	}
}

void doIo(char* schedPol, int numChildren, char* bytesToCopy, char* blockSize) {
	struct sched_param param;
	int policy;
	int i;
	char* newArgv[4];
	newArgv[0] = "rw\0";
	newArgv[1] = bytesToCopy;
	newArgv[2] = blockSize;
	newArgv[3] = NULL;

	if(!strcmp("SCHED_FIFO", schedPol)) {
		policy = SCHED_FIFO;
	}
	else if (!strcmp("SCHED_RR", schedPol)) {
		policy = SCHED_RR;
	}
	else {
		policy = SCHED_OTHER;
	}
	param.sched_priority = sched_get_priority_max(policy);
	sched_setscheduler(0, policy, &param);
	
	for(i = 0; i < numChildren; i++) {
		if(fork() == 0) {
			if(execve("rw", newArgv, environ) < 0) {
				printf("execve failed!\n");
				exit(1);
			}
		}
	}
	while(waitpid(-1, NULL, 0) > 0) {
	}
}

void doMixed(char* schedPol, int numChildren, char* bytesToCopy, char* blockSize, char* iterations) {
	struct sched_param param;
	int policy;
	int i;
	char* newArgv[5];
	newArgv[0] = "rw\0";
	newArgv[1] = bytesToCopy;
	newArgv[2] = blockSize;
	newArgv[3] = iterations;
	newArgv[4] = NULL;

	if(!strcmp("SCHED_FIFO", schedPol)) {
		policy = SCHED_FIFO;
	}
	else if (!strcmp("SCHED_RR", schedPol)) {
		policy = SCHED_RR;
	}
	else {
		policy = SCHED_OTHER;
	}
	param.sched_priority = sched_get_priority_max(policy);
	sched_setscheduler(0, policy, &param);
		
	for(i = 0; i < numChildren; i++) {
		if(fork() == 0) {
			if(execve("mixed", newArgv, environ) < 0) {
				printf("execve failed!\n");
				exit(1);
			}
		}
	}
	while(waitpid(-1, NULL, 0) > 0) {
	}
}

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("You have failed\n");
		exit(0);
	}
	char* P_TYPE = argv[1];
	
	
	if(!strcmp("PROC_CPU", P_TYPE)){
		doCpu(argv[4], atoi(argv[3]), argv[2]);	
	}
	
	else if(!strcmp("PROC_IO", P_TYPE)){
		doIo(argv[5], atoi(argv[2]), argv[3], argv[4]);
	}
	
	else {
		doMixed(argv[6], atoi(argv[2]), argv[3], argv[4], argv[5]);
	}
		
	return 0;
}
			
