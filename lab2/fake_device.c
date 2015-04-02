#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
static char DISK_BUFFER[1024];
static char INT_BUFFER[512];
static char DISK_STAT[1];

int main() {
	int fifIn;
	int fifOut;
	int eof = 0;
	char * diskFifo = "/tmp/virtual_dev_in";
	char * drivFifo = "/tmp/virtual_dev_out";
	mkfifo(diskFifo, 0666);
	mkfifo(drivFifo, 0666);
	for (int i = 0; i < sizeof(DISK_STAT); i++)
		DISK_STAT[i] = 0;
	
	fifIn = open(diskFifo, O_RDONLY | O_CREAT, 0666);
	fifOut = open(drivFifo, O_WRONLY | O_CREAT, 0666);
	
	while(1) {
		read(fifIn, INT_BUFFER, 1024);
		if(strcmp(INT_BUFFER, "WRITE_REQ") == 0 && DISK_STAT[0] == 0) {
			printf("WRITE REQUEST\n");
			DISK_STAT[0] = 1;
			write(fifOut, "WRITE_ACK", sizeof("WRITE_ACK"));
		}
		else if(DISK_STAT[0] == 1 && strcmp(INT_BUFFER, "WRITE_REQ") != 0) {
			int i;
			for(i = 0; i < sizeof(INT_BUFFER) && INT_BUFFER[i] != '\0'; i++) {
				DISK_BUFFER[eof] = INT_BUFFER[i];
				eof++;
			}
			DISK_STAT[0] = 0;
			printf("%s", DISK_BUFFER);
		}  
	}
	/*close(fifIn);
	close(fifOut);
	exit(0);*/
}
