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
#include "ckpt_sgmt.h"

ucontext_t context;  // contains certain register values to be saved/restored


/**
 * Places the information of 1 process in the /proc/self/maps file into the ckpt_sgmt 
 *     struct process; returns 1 upon success, EOF upon the file's end.
 * @param proc_maps_fd a file descriptor that points to /proc/self/maps.
 * @param process an empty struct that will be populated with informatoin corresponding
 * @param filename an array of char which will have the process's name scanf'd into it. 
 * @return returns 0 upon success, EOF when there are no more processes left to write.
 */
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

/**
 * Loops through /proc/self/maps and captures each process's information into a
 *     ckpt_sgmt struct within the processes[] array.
 * @param processes[] an emtpy array of 1000 ckpt_sgmt structs that are populated
 *     with individual processes one at a time.
 */
int proc_self_maps(struct ckpt_sgmt processes[]) {
	// using open() b/c fopen() calls malloc(), potentially extending the heap
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

/**
 * Takes in an empty array of ckpt_sgmt structs and populates them with processes from
 *     /proc/self/maps; also ensures that this data is written without error.
 * @param processes[] an empty array of 1000 cpkt_sgmt structs which will be populated
 *     with individual processes.
 */
void get_processes(struct ckpt_sgmt processes[]) {
	// getting proc/self/maps "header" (struct ckpt_sgmt) info:
	assert(proc_self_maps(processes) == 0);
	return;
}

/**
 * A "commented out" helper function that prints individual processes from processes[]
 *     which satisify certain constraints (the same ones required to be written to the
 *     dat file).
 * @param processes[] an array of 1000 ckpt_sgmt structs which contain process data.
 */
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

/**
 * Writes ucontext_t context and process information to a dat file with headers/metadata;
 *     accomplishes this in 2 parts:
 *     1. creates a context header and writes it, then writes the context variable,
 *     2. for each process in processes[], ensures the process satisfies certain constraints,
 *         then writes the header followed by the actual data itself.
 * @param processes[] an array of 1000 ckpt_sgmt structs which contain process data. 
 */
void write_context_and_processes(struct ckpt_sgmt processes[]) {
	int fd = open(FILE_NAME, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR);
	if (fd == -1) {	perror("open");	}	
	
	// 1 WRITING CONTEXT:
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
	rc = write(fd, &context, c.data_size);
	if (rc == -1) { perror("write"); }
	while (rc < c.data_size) {
		rc += write(fd, &context + rc, c.data_size - rc);
	}
	assert(rc == c.data_size);

	// 2 WRITING PROCESSES:
	int i;
	for (i = 0; processes[i].start != NULL; i++) {  // for each process struct...
		// 2A.i (CHECK 1): ensure process i is not "vdso", "vsyscall", or "vvar"
		if (strcmp(processes[i].name, "[vsyscall]") == 0
				|| strcmp(processes[i].name, "[vdso]") == 0
				|| strcmp(processes[i].name, "[vvar]") == 0) {
			continue;
		}

		// 2A.ii (CHECK 2): ensure process i does not have rwxp of "---_"
		if (processes[i].rwxp[0]=='-' 
				&& processes[i].rwxp[1]=='-' 
				&& processes[i].rwxp[2]=='-') {
			continue;
		}

		// 2B.i (WRITE 1): writing header (ckpt_sgmt process i struct)
		int rc = write(fd, &processes[i], sizeof(struct ckpt_sgmt));
		if (rc == -1) { perror("write"); }
		while (rc < sizeof(struct ckpt_sgmt)) {
			rc += write(fd, (char *)&processes[i] + rc, sizeof(struct ckpt_sgmt) - rc);
		}
		assert(rc == sizeof(struct ckpt_sgmt));

		// 2B.ii (WRITE 2): writing data
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

/**
 * Writes the ucontext_t context and process information to a dat file with metadata;
 *     does this by (1) getting all process information (proc/self/maps), and then
 *     (2) writing the process and (global) context information with metadata. 
 */
void do_checkpoint() {
	// 1. GETTING PROCESSES, IN THE FORM OF AN ARRAY OF 1000 STRUCTS:
	struct ckpt_sgmt processes[1000];  // assuming max 1000 processes
	get_processes(processes);
	//print_processes_headers(processes);

	// 2. WRITING CONTEXT AND PROCESSES:
	write_context_and_processes(processes);
}

/**
 * A custom signal handler; calls getcontext, which sets the value of the global context
 *     variable; if we are not "restarting" (is_restart = 0), then do_checkpoint()
 *     writes the context information to the checkpoint dat file; if we are restarting,
 *     then we need not worry about writing (an already existing) checkpoint dat file.
 * @param signum an integer which corresponds to a signal number.
 */
void signal_handler(int signum) {
	static int is_restart = 0;  // static local variables are stored in data segment
	int gc = getcontext(&context);
	if (gc == -1) {
		perror("getcontext");
		exit(1);
	}

	if (is_restart == 1) {  // if we are restarting
		is_restart = 0;
		printf("*** RESTARTING ***\n");
		return;
	} 
	else {  // if we are not yet restarting
		is_restart = 1;
		printf("*** WRITING CHECKPOINT IMAGE FILE ***\n");
		do_checkpoint();
	}
}

/**
 * Defines a constructor which is executed when _start first looks for constructors
 *     before running main(); in particular, my_constructor() registers a custom
 *     signal handler (see signal_handler() above). 
 */
void __attribute__((constructor)) my_constructor() {
	// constructor calls signal to register a signal handler
	signal(SIGUSR2, &signal_handler);
	fprintf(stderr, "*** We are running, using ckpt ***\n");
}
