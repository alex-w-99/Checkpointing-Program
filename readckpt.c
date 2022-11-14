#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <fcntl.h>
#include <ucontext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "ckpt_sgmt.h"


/**
 * A function which reads the context of myckpt.dat (corresponding to fd), and
 *     places that information into an array of ckpt_sgmt structs (metadata[])
 *     before printing that information to stdout.
 * @param fd a file descriptor corresponding to "myckpt.dat".
 * @param metadata[] an array of empty ckpt_sgmt structs where individual process
 *        header/metadata information will be placed.
 */
void readckpt(int fd, struct ckpt_sgmt metadata[]) {
	// 1. reading header (metadata) of context variable:
	int rc = read(fd, &metadata[0], sizeof(metadata[0]));
	if (rc == -1) {
		perror("read");
		exit(1);
	}
	while (rc < sizeof(metadata[0])) {
		rc += read(fd, &metadata[0] + rc, sizeof(metadata[0]) - rc);
	}
	assert(rc == sizeof(metadata[0]));

	// 2. reading headers (metadata) of maps:
	int i = 1;
	while (rc != 0) {
		lseek(fd, metadata[i-1].data_size, SEEK_CUR);  // skipping over data

		rc = read(fd, &metadata[i], sizeof(metadata[i]));
		if (rc == 0) {
			break;
		}
		while (rc < sizeof(metadata[i])) {
			rc += read(fd, &metadata[i] + rc, sizeof(metadata[i]) - rc);
		}
		assert(rc == sizeof(metadata[i]));
	
		i = i + 1;
	}

	// 3. printing metadata after reading myckpt.dat:
	int j;
	for (j = 0; j < i; j++) {
		if (metadata[j].is_register_context == 1) {
			printf("%d; metadata.name: %s\n", j, metadata[j].name);
			printf("\tis_register_context: %d\n", metadata[j].is_register_context);
		}
		else {  // i.e., a map
			printf("%d; metadata.name: %s\n", j, metadata[j].name);
			printf("\tstart - end: %p - %p\n", metadata[j].start, metadata[j].end);
			printf("\trwxp: %c%c%c\n", metadata[j].rwxp[0], 
				metadata[j].rwxp[1], 
				metadata[j].rwxp[2]);
			printf("\tis_register_context: %d\n",
					metadata[j].is_register_context);
			printf("\tdata_size: %d\n", metadata[j].data_size);
		}
	}
	
	// 4. closing file
	close(fd);
}

/**
 * Program entry point.
 */
int main() {
	struct ckpt_sgmt metadata[1000];
	int fd = open(FILE_NAME, O_RDONLY);
	if (fd == -1) { perror("open"); }
	readckpt(fd, metadata); 

	return 0;
}
