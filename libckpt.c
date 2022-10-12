#define _GNU_SOURCE  // for constructor attribute (GNU extension)
#define _POSIX_C_SOURCE  // for sigsetjmp/siglongjmp
#include <ucontext.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char* FILE_NAME = "myckpt.dat" ;

#define NAME_LEN 80
struct ckpt_sgmt {
	void* start;
	void* end;
	char rwxp[4];
	char name[NAME_LEN];
	int is_register_context;  // 1 if context from getcontext
	int data_size;  // size of data after this header; sizeof(context)
};

// 'context' variable includes certain register values to be saved/restored
ucontext_t context;

// Returns 0 on success
int match_one_line(int proc_maps_fd, struct ckpt_sgmt *process, char* filename) {
	unsigned long int start, end;
	char rwxp[4];
	char tmp[10];

	int tmp_stdin = dup(0);  // make a copy of stdin
	dup2(proc_maps_fd, 0);  // duplicate proc_maps_fd to stdin
	
	// scanf reads stdin (fd:0); it is a dup of proc_maps_fd (same struct file)
	int rc = scanf("%lx-%lx %4c %*s %*s %*[0-9 ]%[^\n]\n",
			&start, &end, rwxp, filename);

	// fseek removes the EOF indicator on stdin for any future calls to scanf
	assert(fseek(stdin, 0, SEEK_CUR) == 0);
	dup2(tmp_stdin, 0);  // restore original stidn; proc_maps_Fd offset was advanced
	close(tmp_stdin);

	if (rc == EOF || rc == 0) {
		process->start = NULL;
		process->end = NULL;
		process->data_size = 0;
		return EOF;
	}
	// else...
	if (rc == 3) {
		strncpy(process->name, "ANONYMOUS_SEGMENT", strlen("ANONYMOUS_SEGMENT")+1);
	}
	else {
		assert(rc == 4);
		strncpy(process->name, filename, NAME_LEN-1);
		process->name[NAME_LEN-1] = '\0';
	}
	process->start = (void *)start;
	process->end = (void *)end;
	process->data_size = process->end - process->start;
	memcpy(process->rwxp, rwxp, 4);
	return 0;
}

int proc_self_maps(struct ckpt_sgmt processes[]) {
	// NOTE: fopen() calls malloc() and potentially extends the heap segment
	int proc_maps_fd = open("/proc/self/maps", O_RDONLY);
	char filename[100];
	int i = 0;
	int rc = -2;  // any value that will not terminate 'for' loop
	for (i = 0; rc != EOF; i++) {
		rc = match_one_line(proc_maps_fd, &processes[i], filename);
	}
	close(proc_maps_fd);
	return 0;
}

void get_processes(struct ckpt_sgmt processes[]) {
	// getting proc/self/maps "header" (struct ckpt_sgmt) info:
	assert(proc_self_maps(processes) == 0);
	//assert(proc_self_maps(processes) == 0);
	return;
}

void print_processes_headers(struct ckpt_sgmt processes[]) {	
	int i;
	for (i = 0; processes[i].start != NULL; i++) {
		// ensure process i is not "vdso", "vsyscall", or "vvar"
		if (strcmp(processes[i].name, "[vsyscall]") == 0
				|| strcmp(processes[i].name, "[vdso]") == 0
				|| strcmp(processes[i].name, "[vvar]") == 0) {
			continue;
		}

		// ensure process i does not have rwxp of "---_"
		if (processes[i].rwxp[0]=='-' 
				&& processes[i].rwxp[1]=='-' 
				&& processes[i].rwxp[2]=='-') {
			continue;
		}

		printf("%d; process name: %s\n", i, processes[i].name);
		printf("\tstart - end: %p - %p\n", processes[i].start, processes[i].end);
		printf("\trwxp: %c%c%c\n",
			processes[i].rwxp[0], processes[i].rwxp[1], processes[i].rwxp[2]);
		printf("\tis_register_context: %d\n", processes[i].is_register_context);
		printf("\tdata_size: %d\n", processes[i].data_size);
	}
}

void write_context_and_processes(ucontext_t *context_p, struct ckpt_sgmt processes[]) {
	int fd = open(FILE_NAME, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {	perror("open");	}	
	
	// PART 1: WRITING CONTEXT
	// 1A. putting ucontext_t "context" variable into struct
	struct ckpt_sgmt c;  // ignoring c.start, c.end, c.rwxp
	strcpy(c.name, "context");
	c.is_register_context = 1;
	c.data_size = sizeof(ucontext_t);  // NOT sizeof(context); context here is a pointer

	// 1B.i (WRITE 1): writing header (ckpt_sgmt context header struct)
	int rc = write(fd, (char*)&c, sizeof(struct ckpt_sgmt));
	if (rc == -1) { perror("write"); }
	while (rc < sizeof(struct ckpt_sgmt)) {
		rc += write(fd, (char *)&c + rc, sizeof(struct ckpt_sgmt) - rc);
	}
	assert(rc == sizeof(struct ckpt_sgmt));

	// 1B.ii (WRITE 2): writing data (ucontext_t "context" variable)
	rc = write(fd, context_p, c.data_size);
	if (rc == -1) { perror("write"); }
	while (rc < c.data_size) {
		rc += write(fd, context_p + rc, c.data_size - rc);
	}
	assert(rc == c.data_size);

	// PART 2: WRITING PROCESSES
	int i;
	for (i = 0; processes[i].start != NULL; i++) {  // for each process struct...
		// 1A.i (CHECK 1): ensure process i is not "vdso", "vsyscall", or "vvar"
		if (strcmp(processes[i].name, "[vsyscall]") == 0
				|| strcmp(processes[i].name, "[vdso]") == 0
				|| strcmp(processes[i].name, "[vvar]") == 0) {
			continue;
		}

		// 1A.ii (CHECK 2): ensure process i does not have rwxp of "---_"
		if (processes[i].rwxp[0]=='-' 
				&& processes[i].rwxp[1]=='-' 
				&& processes[i].rwxp[2]=='-') {
			continue;
		}

		// 1B.i (WRITE 1): writing header (ckpt_sgmt process i struct)
		int rc = write(fd, &processes[i], sizeof(struct ckpt_sgmt));
		if (rc == -1) { perror("write"); }
		while (rc < sizeof(struct ckpt_sgmt)) {
			rc += write(fd, (char *)&processes[i] + rc, sizeof(struct ckpt_sgmt) - rc);
		}
		assert(rc == sizeof(struct ckpt_sgmt));

		// 1B.ii (WRITE 2): writing data
		rc = write(fd, (void*)processes[i].start, processes[i].end - processes[i].start);
		if (rc == -1) { perror("write"); }
		while (rc < processes[i].end - processes[i].start) {
			rc += write(fd, (void*)processes[i].start + rc,
					processes[i].end - processes[i].start - rc);
		}
		assert(rc == processes[i].data_size);
	}
	close(fd);
}

void do_checkpoint(ucontext_t *context_p) {
	// 1. getting processes, in the form of an array of 1000 structs:
	struct ckpt_sgmt processes[1000];  // assuming max 1000 processes
	get_processes(processes);
	//print_processes_headers(processes);

	// 2. writing context and processes:
	write_context_and_processes(context_p, processes);
}

void signal_handler(int signal) {
	static int is_restart = 0;  // static local variables are stored in data segment
	int gc = getcontext(&context);
	if (gc == -1) {
		perror("getcontext");
		exit(1);
	}

	if (is_restart == 1) {
		is_restart = 0;
		printf("*** RESTARTING ***\n");
		return;
	} 
	else {
		is_restart = 1;
		printf("*** WRITING CHECKPOINT IMAGE FILE ***\n");
		do_checkpoint(&context);
	}
}

void __attribute__((constructor)) my_constructor() {
	// constructor calls signal to register a signal handler
	signal(SIGUSR2, &signal_handler);
	fprintf(stderr, "*** We are running, using ckpt ***\n");
}
