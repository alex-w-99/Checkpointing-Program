#ifndef CKPT_SGMT_H
#define CKPT_SGMT_H

#define NAME_LEN 80

char* FILE_NAME = "myckpt.dat";

struct ckpt_sgmt {  // header/metadata
	void* start;
	void* end;
	char rwxp[4];
	char name[NAME_LEN];
	int is_register_context;  // 0 if memory segment, 1 if context from getcontext()
	int data_size;  // size of data that follows this header
};

#endif /* CKPT_SGMT_H */
