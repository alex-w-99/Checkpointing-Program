#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <ucontext.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/mman.h>

char* FILE_NAME = "myckpt.dat";

#define NAME_LEN 80
struct ckpt_sgmt {  // header
	void* start;
	void* end;
	char rwxp[4];
	char name[NAME_LEN];
	int is_register_context;  // 1 if context from getcontext
	int data_size;
} ckpt_sgmt;

void print_metadata(struct ckpt_sgmt metadata, int i) {
	if (metadata.is_register_context == 1) {
		printf("%d; metadata.name: %s\n", i, metadata.name);
		printf("\tmetadata.is_register_context: %d\n", metadata.is_register_context);
	}
	else {
		printf("%d; metadata.name: %s\n", i, metadata.name);
		printf("\tstart - end: %p - %p\n", metadata.start, metadata.end);
		printf("\trwxp: %c%c%c\n", 
				metadata.rwxp[0], metadata.rwxp[1], metadata.rwxp[2]);
		printf("\tis_register_context: %d\n", metadata.is_register_context);
		printf("\tdata_size: %d\n", metadata.data_size);
	}
}

void restart() {
	ucontext_t old_context;

	// 1. opening FILE_NAME ("myckpt.dat") file
	int fd = open(FILE_NAME, O_RDONLY);
	if (fd == -1) { perror("open"); }

	// 2. setting up array of ckpt_sgmt "headers":
	struct ckpt_sgmt metadata[1000];

	// 3. reading metadata, context, and maps:
	int i = 0;  // keep track of metadata header index
	int rc = 0;  // placeholder value
	while (1) {
		// 3.1. reading header:
		rc = read(fd, &metadata[i], sizeof(metadata[i]));
		if (rc == -1) {
			perror("read");
			exit(1);
		}
		else if (rc == 0) {  // i.e., EOF
			break;
		}
		while (rc < sizeof(metadata[i])) {
			rc += read(fd, &metadata[i] + rc, sizeof(metadata[i]) - rc);
		}
		assert(rc == sizeof(metadata[i]));
		
		// 3.2. reading data (either ucontext_t or maps):
		if (metadata[i].is_register_context == 1) {  // 3.2.1; header is for ucontext_t
			rc = read(fd, &old_context, sizeof(old_context));
			while (rc < sizeof(old_context)) {
				rc += read(fd, &old_context + rc, sizeof(old_context) - rc);
			}
			assert(rc == sizeof(old_context));
		}
		else {  // 3.2.2; header is NOT for ucontext_t (i.e., a map)
			void *addr = mmap(metadata[i].start,  // start address for new mapping
					metadata[i].data_size,  // length of the mapping
					PROT_READ|PROT_WRITE|PROT_EXEC,  // mem map protections
					MAP_FIXED|MAP_PRIVATE|MAP_ANONYMOUS,  // flags
					-1,  // fd==-1 for MAP_ANONYMOUS
					0);  // offset==0 for MAP_ANONYMOUS
			if (addr == MAP_FAILED) { perror("mmap"); }			
				
			rc = read(fd, (char *)addr, metadata[i].data_size);
			while (rc < metadata[i].data_size) {
				rc += read(fd, (char *)addr + rc, metadata[i].data_size - rc);
			}
			assert(rc == metadata[i].data_size);
		}
		//print_metadata(metadata[i], i);
		i = i + 1;
	}

	// 4. closing fd:
	close(fd);
	
	// 5. setting context:
	if (setcontext(&old_context) == -1) {
		perror("setcontext");
		exit(1);
	}
}

// used to grow the stack with many call frames, so that latest call frame
// will be at an address with no conflict
void recursive(int levels) {
	if (levels > 0) {
		recursive(levels - 1);
	}
	else {  // base case
		restart();
	}
}

int main() {
	recursive(1000);
	
	return 0;
}
