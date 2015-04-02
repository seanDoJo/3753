#ifndef MULTI_L
#define MULTI_L

#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
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

void checkFunction(void* payloadp);

void doRequest(void* argPointer);

void doLookup(void* argPointer);

#endif
