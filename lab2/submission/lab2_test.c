#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
int main() {
	char line[256];
	char opt;
	int active = 1;
	printf("Simple Character Device Interface\nr:\tread from device\nw:\twrite to device\ne:\texit interface\n");

	while(active) {
		printf("Enter your command: ");
		fgets(line, sizeof(line), stdin);
		if(1 == sscanf(line, "%c", &opt)) {
			if (opt == 'r') {
				FILE * filed = fopen("/dev/simple_char_device", "r");
				int c;
				if(filed) {
					printf("Data Read From Device: ");
					while((c = getc(filed)) != EOF) {
						putchar(c);
					}
					fclose(filed);
				}
			}
			else if (opt == 'w') {
				int fd = open("/dev/simple_char_device", O_WRONLY);
				char inputData[128];
				printf("Enter data you want to write to the device: ");
				fgets(inputData, sizeof(inputData), stdin);
				int i;
				for(i = 0; inputData[i] != '\0' && i < sizeof(inputData); i++);
				write(fd, inputData, i);
				close(fd);
			}
			else if (opt == 'e') {
				active = 0;
				printf("Bye!\n");
			}
			else {
				printf("unrecognized!\n");
			}
		}
	}
}
