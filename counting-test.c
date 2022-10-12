#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>  // used for INT_MAX
#include <assert.h>

int main(int argc, char* argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: \"%s [POSITIVE_INTEGER]\"\n", argv[0]);
		exit(1);
	}

	int num = strtol(argv[1], NULL, 10);
	if (num > INT_MAX - 20) {
		fprintf(stderr, "%s: Overflow; requested number is larger than %d (i.e., INT_MAX)\n", 
				argv[0], INT_MAX - 20);
		exit(1);
	}
	assert(num > 0);

	printf("Counting from number %d\n", num);
	int i;
	for (i = num; i < num + 10; i++) {
		printf("The next counting number is: %d\n", i);
		sleep(1);
	}

	return 0;
}
