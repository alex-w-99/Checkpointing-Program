# Lightweight Checkpointing Program

A lightweight checkpointing program written in C. This program is to be run with another executable; for example, `./ckpt ./a.out`. The executable will then run as normal until (or if) it receives a `SIGUSR2` signal, at which point a checkpoint image file `myckpt.dat` will be generated and the program will stop. Once this checkpoint image file is generated, the executable can be continued (or "restarted") by running `./restart` and it will pick up right where it left off.

This program is entirely self-contained, meaning any executable that is compiled with the flag `gcc -rdynamic` is compatible with this software. The flag `rdynamic` tells the linker to export symbols for the executable (by default, the linker only exports symbols for shared libraries). This allows us to dynamically load in any other executable that we wish to use with this checkpointing program.

More specifically, this program exploits the fact that any executable's `_start` routine first looks at constructors before it calls `main` (which eventually calls `_exit`). In particular, we can use the `LD_PRELOAD` trick - if we set the `LD_PRELOAD` variable to the path of a share object file, that file will be loaded *before* any other library (including before `libc.so`). This allows us to do some unconventional things, like checkpointing!  



dummy programs to run ckpt with (counting-test, hello-test)

discuss stack smashing

discuss `%fs` register which predates ucontext.h, so that therei is a bad `%fs` register value upon restart, thus leading to a segfault




Concepts covered in this project include (but are not limited to) checkpointing, context switching, signal handling, environment variables, constructor functions, memory layout, system calls like `mmap`, etc. 

Tested on Ubuntu 20.04.1, compiled with `gcc -std=gnu17 ...` (used in Makefile).

## Implementation Details

- asdf:
  - asdf
- <ins>Creating Checkpoint File:</ins> asdf
  - The checkpoint file can be written by running `./ckpt ./a,out ...` and sending the active process a `SIGUSR2` signal (i.e., `kill SIGUSR2 [PID]`).
- <ins>Reading Checkpoint File:</ins> asdf
  - Once the checkpoint file is generated, its contents can be read back via `./restart`.
- <ins>Restarting via Checkpoint File:</ins> asdf
  - Once the checkpoint file is generated, the program can be restarted from the point it received the `SIGUSR2` signal via `./restart`.
- <ins>Test/Dummy Programs:</ins> To illustrate the functionality of this lightweight checkpointing program, the following dummy programs are included (note that they are compiled with the `-rdynamic` flag in the Makefile).
  - <ins>counting-test.c</ins>: A simple program taking a single integer as an argument, printing that number and the following 9 numbers (with 1 second between each print) before exiting.
  - <ins>hello-test.c</ins>: A very simple program that takes no arguments, printing "Hello world!" ten times before printing "Goodbye world!" and exiting. 

## Areas for Future Improvement

- [ ] Save the `%fs` register in `myckpt.dat` (the checkpoint image file), so that when the program restarts via `restart.c` and calls `setcontext` in `libckpt.c`, there is no segmentation fault at the very end of the program due to a bad `%fs` register value (as discussed above, `ucontext.h` predates the `%fs` register - a register which is called in `exit` - and so the register's value is lost when restarting).  
- [ ] Add support for multi-threaded programs. Initially, this project was designed to work only with single-threaded programs; this way, the checkpointing program need not concern itself with the intricacies of things like thread-local storage (TLS), etc.

## Screenshots

asdf

## Acknowledgements 

- Professor Gene Cooperman, my Computer Systems professor.

## Contact Information

- Alexander Wilcox
- Email: alexander.w.wilcox@gmail.com
