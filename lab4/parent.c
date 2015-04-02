
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

extern char** environ;
FILE* inputfp = NULL;

void doCpu(char* schedPol, int numChildren, char* iterations) {
	struct sched_param param;
	int policy;
	struct rusage pidStats;
	long INV_SW = 0;
	long VOL_SW = 0;
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
		

	INV_SW = 0;
	VOL_SW = 0;
	for(i = 0; i < numChildren; i++) {
		if(fork() == 0) {
			if(execve("pi", newArgv, environ) < 0) {
				printf("execve failed!\n");
				exit(1);
			}
		}
	}
	while(wait4(0, NULL, 0, &pidStats) > 0) {
		INV_SW += pidStats.ru_nivcsw;
		VOL_SW += pidStats.ru_nvcsw;
	}
	INV_SW = INV_SW / numChildren;
	VOL_SW = VOL_SW / numChildren;
	fprintf(inputfp, "%ld, %ld\n", INV_SW, VOL_SW);
}

void doIo(char* schedPol, int numChildren, char* bytesToCopy, char* blockSize) {
	struct sched_param param;
	int policy;
	struct rusage pidStats;
	long INV_SW = 0;
	long VOL_SW = 0;
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
		

	INV_SW = 0;
	VOL_SW = 0;
	for(i = 0; i < numChildren; i++) {
		if(fork() == 0) {
			if(execve("rw", newArgv, environ) < 0) {
				printf("execve failed!\n");
				exit(1);
			}
		}
	}
	while(wait4(0, NULL, 0, &pidStats) > 0) {
		INV_SW += pidStats.ru_nivcsw;
		VOL_SW += pidStats.ru_nvcsw;
	}
	INV_SW = INV_SW / numChildren;
	VOL_SW = VOL_SW / numChildren;
	fprintf(inputfp, "%ld, %ld\n", INV_SW, VOL_SW);
}

void doMixed(char* schedPol, int numChildren, char* bytesToCopy, char* blockSize, char* iterations) {
	struct sched_param param;
	int policy;
	struct rusage pidStats;
	long INV_SW = 0;
	long VOL_SW = 0;
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
		

	INV_SW = 0;
	VOL_SW = 0;
	for(i = 0; i < numChildren; i++) {
		if(fork() == 0) {
			if(execve("mixed", newArgv, environ) < 0) {
				printf("execve failed!\n");
				exit(1);
			}
		}
	}
	while(wait4(0, NULL, 0, &pidStats) > 0) {
		INV_SW += pidStats.ru_nivcsw;
		VOL_SW += pidStats.ru_nvcsw;
	}
	INV_SW = INV_SW / numChildren;
	VOL_SW = VOL_SW / numChildren;
	fprintf(inputfp, "%ld, %ld\n", INV_SW, VOL_SW);
}

int main(int argc, char** argv) {
	if(argc < 2) {
		printf("You have failed\n");
		exit(0);
	}
	
	struct sched_param param;
	char* P_TYPE = argv[1];
	int policy;
	struct rusage pidStats;
	
	
	if(!strcmp("PROC_CPU", P_TYPE)){
		int numChildren = 0;
		int i;
		inputfp = fopen("cpubound.csv", "w+");
		
		numChildren = 10 + (rand() % 91);
		doCpu("SCHED_OTHER", numChildren, argv[2]);
		numChildren = 100 + (rand() % 901);
		doCpu("SCHED_OTHER", numChildren, argv[2]);
		numChildren = 1000; //+ (rand() % 9001);
		doCpu("SCHED_OTHER", numChildren, argv[2]);
		
		fprintf(inputfp, "\n\n");
		
		numChildren = 10 + (rand() % 91);
		doCpu("SCHED_FIFO", numChildren, argv[2]);
		numChildren = 100 + (rand() % 901);
		doCpu("SCHED_FIFO", numChildren, argv[2]);
		numChildren = 1000; //+ (rand() % 9001);
		doCpu("SCHED_FIFO", numChildren, argv[2]);
		
		fprintf(inputfp, "\n\n");
		
		numChildren = 10 + (rand() % 91);
		doCpu("SCHED_RR", numChildren, argv[2]);
		numChildren = 100 + (rand() % 901);
		doCpu("SCHED_RR", numChildren, argv[2]);
		numChildren = 1000; //+ (rand() % 9001);
		doCpu("SCHED_RR", numChildren, argv[2]);
		
		fclose(inputfp);
	}
	else if(!strcmp("PROC_IO", P_TYPE)){
		int numChildren = 0;
		int i;
		inputfp = fopen("iobound.csv", "w+");
		
		numChildren = 10 + (rand() % 91);
		doIo("SCHED_OTHER", numChildren, argv[2], argv[3]);
		numChildren = 100 + (rand() % 901);
		doIo("SCHED_OTHER", numChildren, argv[2], argv[3]);
		numChildren = 1000; //+ (rand() % 9001);
		doIo("SCHED_OTHER", numChildren, argv[2], argv[3]);
		
		fprintf(inputfp, "\n\n");
		
		numChildren = 10 + (rand() % 91);
		doIo("SCHED_FIFO", numChildren, argv[2], argv[3]);
		numChildren = 100 + (rand() % 901);
		doIo("SCHED_FIFO", numChildren, argv[2], argv[3]);
		numChildren = 1000; //+ (rand() % 9001);
		doIo("SCHED_FIFO", numChildren, argv[2], argv[3]);
		
		fprintf(inputfp, "\n\n");
		
		numChildren = 10 + (rand() % 91);
		doIo("SCHED_RR", numChildren, argv[2], argv[3]);
		numChildren = 100 + (rand() % 901);
		doIo("SCHED_RR", numChildren, argv[2], argv[3]);
		numChildren = 1000; //+ (rand() % 9001);
		doIo("SCHED_RR", numChildren, argv[2], argv[3]);
		
		fclose(inputfp);
	}
	
	else {
		int numChildren = 0;
		int i;
		inputfp = fopen("mixedbound.csv", "w+");
		
		numChildren = 10 + (rand() % 91);
		doMixed("SCHED_OTHER", numChildren, argv[2], argv[3], argv[4]);
		numChildren = 100 + (rand() % 901);
		doMixed("SCHED_OTHER", numChildren, argv[2], argv[3], argv[4]);
		numChildren = 1000; //+ (rand() % 9001);
		doMixed("SCHED_OTHER", numChildren, argv[2], argv[3], argv[4]);
		
		fprintf(inputfp, "\n\n");
		
		numChildren = 10 + (rand() % 91);
		doMixed("SCHED_FIFO", numChildren, argv[2], argv[3], argv[4]);
		numChildren = 100 + (rand() % 901);
		doMixed("SCHED_FIFO", numChildren, argv[2], argv[3], argv[4]);
		numChildren = 1000; //+ (rand() % 9001);
		doMixed("SCHED_FIFO", numChildren, argv[2], argv[3], argv[4]);
		
		fprintf(inputfp, "\n\n");
		
		numChildren = 10 + (rand() % 91);
		doMixed("SCHED_RR", numChildren, argv[2], argv[3], argv[4]);
		numChildren = 100 + (rand() % 901);
		doMixed("SCHED_RR", numChildren, argv[2], argv[3], argv[4]);
		numChildren = 1000; //+ (rand() % 9001);
		doMixed("SCHED_RR", numChildren, argv[2], argv[3], argv[4]);
		
		fclose(inputfp);
	}
		
	return 0;
}
			
