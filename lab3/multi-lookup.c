#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#include "util.h"
#include "queue.h"

#define MINARGS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define SBUFSIZE 1025
#define INPUTFS "%1024s"
#define MAX_RESOLVER_THREADS 10
// need to dynamically allocate requester thread array
// try to dynamically allocate requester hostname array
// general error checking
queue qt;
struct request_args {
		char* input;
		char* output;
		int core_assign;
	};
struct resolve_args {
		char* output;
		int core_assign;
		int* qFlag;
	};
struct helper_args {
		char** payload;
		char* input;
		int* left;
		int* doneFlag;
		int core_assign;
		int* bufSize;
		pthread_mutex_t check;
};
pthread_mutex_t qlock;
pthread_mutex_t flock;
pthread_mutex_t rlock;
pthread_mutex_t readBlock;
pthread_cond_t allDone;
pthread_cond_t lookupCond;
int hostnameLeft;
int readCount;

void checkFunction(void* payloadp) {
	struct helper_args *args = payloadp;
	
	//local variables
	char** payload_in = args -> payload;
	char* fileloc = args -> input;
	int* continuer = args -> doneFlag;
	int* bufferSize = args -> bufSize;
	pthread_mutex_t mut = args -> check;
	int* sharedLeft = args -> left;
	char nameCheck[4096];
	int x, i, z;
	FILE* inputfp = NULL;
	
	//set core id to parent core id
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	int core_loc = args -> core_assign;
	CPU_SET(core_loc, &cpuset);
	pthread_t current = pthread_self();
	pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
	
	x = 1;
	int doneSize = (*bufferSize);
	int* done = malloc((*bufferSize) * sizeof(int));
	if(!done) {
		printf("ERROR, squire done malloc failed!\n");
		pthread_exit(NULL);
	}
	for (z = 0; z < (*bufferSize); z++) {
		done[z] = 1;
	}
	
	
	while(x && *continuer) {
		//sleep for 250 ms
		usleep(250 * 1000);
		
		//readers/writers solution to obtain file lock
		pthread_mutex_lock(&readBlock);
		pthread_mutex_lock(&rlock);
		if (readCount == 0) pthread_mutex_lock(&flock);
		readCount++;
		pthread_mutex_unlock(&rlock);
		pthread_mutex_unlock(&readBlock);
		inputfp = fopen(fileloc, "r");
		
		//loop through existing results
		while(fscanf(inputfp, INPUTFS, nameCheck) > 0) {
			pthread_mutex_lock(&mut);
			//compare to hostnames in the parent buffer
			for(i = 0; i < (*bufferSize) && strcmp(payload_in[i], ""); i++) {
				if(doneSize < (*bufferSize)) {
					done = (int*)realloc(done, (*bufferSize) * sizeof(int));
					if(!done) {
						printf("ERROR, squire done realloc failed!\n");
						pthread_exit(NULL);
					}
					for(z = doneSize; z < (*bufferSize); z++) {
						done[z] = 1;
					}
					doneSize = (*bufferSize);
				}
				pthread_mutex_unlock(&mut);
				int j;
				int k;
				//clear any previously written data frin name
				char name[1025];
				for(j = 0; nameCheck[j] != ',' && j < 1025; j++);
				for(k = 0; k < 1025; k++) {
					name[k] = 0;
				}
				//copy just the hostname from the output file
				strncpy(name, nameCheck, j);
				if(!strcmp(payload_in[i], name) && done[i]) {
					//if a previously undetected hostname is found, print it and mark it as detected
					printf("%s\n", nameCheck);
					pthread_mutex_lock(&mut);
					*sharedLeft -= 1;
					done[i] = 0;
					pthread_mutex_unlock(&mut);
				}
				pthread_mutex_lock(&mut);
			}
			pthread_mutex_unlock(&mut);
			//release file lock
		}
		fclose(inputfp);
		pthread_mutex_lock(&rlock);
		readCount--;
		if (readCount == 0) pthread_mutex_unlock(&flock);
		pthread_mutex_unlock(&rlock);
	}
	free(done);
}
		
	

void doRequest(void* argPointer) {
	struct request_args *args = argPointer;
	//local variables
	char* fileloc = args -> input;
	char** payload_in;
	int core_loc = args -> core_assign;
	int i;
	int numHostsInDat = 0;
	int helperDone = 1;
	int curr_size = SBUFSIZE;
	//set core affinity to whatever id was passed
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core_loc, &cpuset);
	pthread_t current = pthread_self();
	pthread_t comp_checker;
	pthread_mutex_t ownChecker;
	if(pthread_mutex_init(&ownChecker, NULL)) {
		printf("ERROR; failure from pthread_mutex_init");
		pthread_exit(NULL);
	}
	
	pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
	payload_in = malloc(SBUFSIZE * sizeof(char*));
	if(!payload_in) {
		printf("ERROR, payload_in malloc failed!\n");
		pthread_exit(NULL);
	}
	
	for(i=0; i<SBUFSIZE; i++){
		payload_in[i] = calloc(SBUFSIZE, sizeof(char));
	}
	//spawn a helper thread to scan the output file for hostnames we've pushed on to the stack
	struct helper_args *vargs = malloc(sizeof(struct helper_args));
	if(!vargs) {
		printf("ERROR, helper_args malloc failed!\n");
		pthread_exit(NULL);
	}
	vargs -> payload = payload_in;
	vargs -> input = args -> output;
	vargs -> left = &numHostsInDat;
	vargs -> check = ownChecker;
	vargs -> doneFlag = &helperDone;
	vargs -> core_assign = core_loc;
	vargs -> bufSize = &curr_size;
	pthread_create(&comp_checker, NULL, (void*)checkFunction, (void*)(vargs));
	FILE* inputfp = NULL;
	inputfp = fopen(fileloc, "r");
	
	if(!inputfp){
		fprintf(stderr, "Error Opening Input File: %s\n", fileloc);
		pthread_exit(NULL);
	}
	//loop through hostnames in the file
	while(fscanf(inputfp, INPUTFS, payload_in[numHostsInDat]) > 0){
		//lock the queue
		pthread_mutex_lock(&qlock);
		while(queue_is_full(&qt)) {
			//if the queue is full, sleep for a random amount of time between 0 and 100 microseconds
			srand(time(NULL));
			pthread_mutex_unlock(&qlock);
			usleep(rand() % 101);
			pthread_mutex_lock(&qlock);
		}
		queue_push(&qt, (void*)(payload_in[numHostsInDat]));
		//obtain queue lock
		hostnameLeft += 1;
		pthread_mutex_lock(&ownChecker);
		numHostsInDat += 1;
		if (numHostsInDat >= curr_size) {
			payload_in = (char**)realloc(payload_in, curr_size * 2 * sizeof(char*));
			if(!payload_in) {
				pthread_mutex_unlock(&ownChecker);
				helperDone = 0;
				pthread_join(comp_checker, NULL);
				fprintf(stderr, "ERROR requester realloc failed!\n");
				pthread_exit(NULL);
			}
			int j;
			for(j = curr_size; j < (curr_size * 2); j++) {
				payload_in[j] = calloc(SBUFSIZE, sizeof(char));
			}
			curr_size *= 2;
		}
		pthread_mutex_unlock(&ownChecker);
		
		//release queue lock
		pthread_cond_broadcast(&lookupCond);
		pthread_mutex_unlock(&qlock);
		//release queue lock
	}
	pthread_mutex_lock(&ownChecker);
	//prevent termination until helper thread has discovered all hostnames we've pushed in the output file
	while(numHostsInDat) {
		pthread_mutex_unlock(&ownChecker);
		pthread_mutex_lock(&ownChecker);
	}
	//kill the helper thread
	helperDone = 0;
	pthread_join(comp_checker, NULL);
	free(vargs);
	pthread_mutex_destroy(&ownChecker);
	for(i=0; i<SBUFSIZE; i++){
		free(payload_in[i]);
	}
	fclose(inputfp);
	free(payload_in);
	free(args);
	
}

void doLookup(void* argPointer) {
	//local variables
	struct resolve_args *args = argPointer;
	char* outputf = args -> output;
	int core_loc = args -> core_assign;
	int* quit = args -> qFlag;
	FILE* outputfp = NULL;
	char hostname[1025];
	char ipstring[4096];
	int x = 1;
	//set core affinity
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(core_loc, &cpuset);
	pthread_t current = pthread_self();
	pthread_setaffinity_np(current, sizeof(cpu_set_t), &cpuset);
	//only run while parent thread says it's ok
	while(x && *quit) {
		//obtain queue lock
		pthread_mutex_lock(&qlock);
		while(queue_is_empty(&qt) && (*quit)) {
			pthread_cond_wait(&lookupCond, &qlock);
		}
		if(*quit == 0) {
			pthread_mutex_unlock(&qlock);
			break;
		}
		//grab a hostname off the queue and get its address info
		strncpy(hostname,((char*)(queue_pop(&qt))),1025);
		if(myDnsLookup(hostname, ipstring, sizeof(ipstring)) == UTIL_FAILURE){
			fprintf(stderr, "dnslookup error: %s\n", hostname);
			strncpy(ipstring, "", sizeof(ipstring));
		}
		
		//release queue lock using readers/writers solution
		pthread_mutex_unlock(&qlock);
		
		//lock the output file
		pthread_mutex_lock(&readBlock);
		pthread_mutex_lock(&flock);
		outputfp = fopen(outputf, "a");
		if(!outputfp){
			perror("Error Opening Output File");
			exit(1);
		}
		fprintf(outputfp, "%s,%s\n", hostname, ipstring);
		fclose(outputfp);
		
		pthread_mutex_unlock(&flock);
		pthread_mutex_unlock(&readBlock);
		
		pthread_mutex_lock(&qlock);
		hostnameLeft -= 1;
		pthread_mutex_unlock(&qlock);
		pthread_cond_signal(&allDone);
		//release output file
	}
	
	free(args);
}

int main(int argc, char* argv[]){
	/* Initialize Queue */
	if(queue_init(&qt, -1) < 0){
		fprintf(stderr, "error: queue_init failed!\n");
		return EXIT_FAILURE;
    }

    /* Local Vars */
    int i;
    int v;
    char* outputFile;
    int started_requests;
    int resolver_thread_count;
    int rc;
    int avail_cores;
    int core_assignment;
    int* resolve_q;
	pthread_t* request_t;
	pthread_t* resolve_t;
	clock_t begin, end;
	double time_spent;
    
    /* Check Arguments */
    if(argc < MINARGS){
		fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
		fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
		return EXIT_FAILURE;
    }
    
    /* Initialize vars */
    
    
    outputFile = argv[argc - 1];
    FILE* temp = fopen(outputFile, "w");
    if(!temp) {
		fprintf(stderr, "Output file can't be opened!\n");
		return EXIT_FAILURE;
	}
    fclose(temp);
    started_requests = 0;
    hostnameLeft = 0;
    core_assignment = 0;
    readCount = 0;
    //initialize mutexes and condition variables
    rc = pthread_mutex_init(&qlock, NULL);
    if (rc){
		printf("ERROR; return code from pthread_mutex_init is %d\n", rc);
		exit(EXIT_FAILURE);
	}
    rc = pthread_mutex_init(&flock, NULL);
    if (rc){
		printf("ERROR; return code from pthread_mutex_init is %d\n", rc);
		exit(EXIT_FAILURE);
	}
    rc = pthread_mutex_init(&readBlock, NULL);
    if (rc){
		printf("ERROR; return code from pthread_mutex_init is %d\n", rc);
		exit(EXIT_FAILURE);
	}
    rc = pthread_mutex_init(&rlock, NULL);
    if (rc){
		printf("ERROR; return code from pthread_mutex_init is %d\n", rc);
		exit(EXIT_FAILURE);
	}
    rc = pthread_cond_init(&allDone, NULL);
    if (rc){
		printf("ERROR; return code from pthread_cond_init is %d\n", rc);
		exit(EXIT_FAILURE);
	}
    rc = pthread_cond_init(&lookupCond, NULL);
    if (rc){
		printf("ERROR; return code from pthread_mutex_init is %d\n", rc);
		exit(EXIT_FAILURE);
	}
    rc = avail_cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (rc < 0){
		printf("ERROR; return code from sysconf is %d\n", rc);
		exit(EXIT_FAILURE);
	}
	//set the number of resolver threads based on the core count
    resolver_thread_count = 4 * avail_cores;
    resolve_t = malloc(resolver_thread_count * sizeof(pthread_t));
    if(!resolve_t) {
		printf("ERROR, resolver thread malloc failed!\n");
		exit(EXIT_FAILURE);
	}
	//create an array of resolver thread continue flags
    resolve_q = malloc(resolver_thread_count * sizeof(int));
    if(!resolve_q) {
		printf("ERROR, resolver thread malloc failed!\n");
		exit(EXIT_FAILURE);
	}
	//initialize continue flags to 1
    for(v = 0; v < resolver_thread_count; v++) {
		resolve_q[v] = 1;
	}
    request_t = malloc((argc - 1) * sizeof(pthread_t));
    if(!request_t) {
		printf("ERROR, resolver thread malloc failed!\n");
		exit(EXIT_FAILURE);
	}
    
    // spawn all resolver threads -- these will be killed after all requesters have been finished and all
    // hostname processing has completed
    begin = clock();
	for(v = 0; v < resolver_thread_count; v++) {
		
		struct resolve_args *args = malloc(sizeof(struct resolve_args)); // need to error check
		if(!args) {
			printf("ERROR, resolver arguments malloc failed!\n");
			exit(EXIT_FAILURE);
		}
		args -> core_assign = core_assignment % avail_cores;
		args -> output = outputFile;
		args -> qFlag = &resolve_q[v];
		
		rc = pthread_create(&resolve_t[v], NULL, (void*)doLookup, (void*)(args));
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
		
		core_assignment += 1;
	}
	
    /* Loop Through Input Files */
    //spawn a request thread for every input file
    for(i=1; i<(argc-1); i++){
		struct request_args *args = malloc(sizeof(struct request_args)); // need to error check
		if(!args) {
			printf("ERROR, requester arguments malloc failed!\n");
			exit(EXIT_FAILURE);
		}
		args -> input = argv[i];
		args -> output = outputFile;
		args -> core_assign = core_assignment % avail_cores;
			
		rc = pthread_create(&(request_t[i-1]), NULL, (void*)doRequest, (void*)(args));
		if (rc){
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(EXIT_FAILURE);
		}
		
		started_requests += 1;
		core_assignment += 1;
    }
    
    //join on request threads
    for (i = 0; i < started_requests; i++) {
		pthread_join(request_t[i],NULL);
	}
	//wait until all resolver threads report their work has completed
	pthread_mutex_lock(&qlock);
	while(hostnameLeft) {
		pthread_cond_wait(&allDone, &qlock);
	}
	pthread_mutex_unlock(&qlock);
    //tell the resolver threads to finish
    for (i=0; i < resolver_thread_count; i++) {
		resolve_q[i] = 0;
		pthread_mutex_lock(&qlock);
		pthread_cond_broadcast(&lookupCond);
		pthread_mutex_unlock(&qlock);
		pthread_join(resolve_t[i],NULL);
	}
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("\n");
	printf("time to complete: %f\n", time_spent);
	//cleanup
	queue_cleanup(&qt);
    pthread_mutex_destroy(&qlock);
    pthread_mutex_destroy(&flock);
    pthread_mutex_destroy(&rlock);
    pthread_mutex_destroy(&readBlock);
    pthread_cond_destroy(&allDone);
    pthread_cond_destroy(&lookupCond);
    free(resolve_t);
    free(resolve_q);
    free(request_t);
    
    return EXIT_SUCCESS;
}
