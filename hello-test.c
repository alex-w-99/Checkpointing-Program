#include <stdio.h>
#include <unistd.h>

int main() {
	int i;
	for (i=0; i<10; i++ ) {
		printf("Hello world!\n");
		sleep(1);
	}
	printf("Goodbye world!\n");
	
	return 0;
}
