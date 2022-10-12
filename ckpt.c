#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>  // dirname
#include <sys/stat.h>
#include <unistd.h>  // exec
#include <assert.h>

int main(int argc, char* argv[]) {
	// PART 1. CHECK FOR GOOD INPUT:
	char **target_argv = argv+1;
	struct stat stat_buf;
	if (target_argv[0]==NULL || stat(target_argv[0], &stat_buf)==-1) {
		fprintf(stderr, "*** Missing target executable or no such file.\n");
		exit(1);
	}

	// PART 2. DEFINING ENVIRONMENT VARIABLE LD_PRELOAD:
	char buf[1000];  // get ./libckpt.so, in same folder as ./ckpt
	buf[sizeof(buf)-1] = '\0';
	snprintf(buf, sizeof buf, "%s/libckpt.so", dirname(argv[0]));
	assert(buf[sizeof(buf)-1] == '\0');  // ensuring no buffer overrun
	setenv("LD_PRELOAD", buf, 1);
	setenv("TARGET", argv[1], 1);
	
	// PART 3. CALLING EXEC ON target_argv[0]:
	int rc = execvp(target_argv[0], target_argv);	
	
	// PART 4. EXEC WAS CALLED ABOVE; THE FOLLOWING SHOULD NOT RUN:
	fprintf(stderr, "Executable '%s' not found.\n", target_argv[0]);
	return 1;
}
