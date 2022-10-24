# Lightweight Checkpointing Program

A lightweight checkpointing program written in C. This program is to be run with another executable; for example, `./ckpt ./a.out ...`. The executable will then run as normal until (or if) it receives a `SIGUSR2` signal, at which point a checkpoint image file `myckpt.dat` will be generated and the program will stop. Once this checkpoint image file is generated, the executable can be continued (or "restarted") at any time thereafter by running `./restart` and the program `a.out` will pick up right where it left off. Please see the example video [linked here](https://www.youtube.com/watch?v=FrD10-QyvNs).

This program is entirely self-contained; any executable compiled with the flag `gcc -rdynamic` is compatible with this software. The flag `-rdynamic` tells the linker to export symbols for the executable (by default, the linker only exports symbols for shared libraries). This allows us to dynamically load in any other executable that we wish to use with this checkpointing program. More specifically, this program exploits the fact that any executable's `_start` routine first looks at constructors before `main` is called (with the program eventually exiting upon calling `_exit`). In particular, we can use the `LD_PRELOAD` trick - if we set the `LD_PRELOAD` variable to the path of a share object file, that file will be loaded *before* any other library (including before `libc.so`). This allows us to do some unconventional things, like checkpointing!  

Concepts covered in this project include (but are not limited to) checkpointing, context switching, signal handling, environment variables, constructor functions, memory layout, system calls like `mmap`, etc. 

Tested on Ubuntu 20.04.1, special/necessary compilation flags used in Makefile.

## Items to Note When Using Program
- <ins>Quick Sample Usage in Makefile:</ins> By running the command `make check`, the Makefile will compile all included files and execute an example of the functionality described above.
- <ins>Segmentation Fault Following Successful Restart:</ins> After a checkpoint image file is created, you will observe that running `./restart` will eventually `SEGFAULT` upon completion of the original program. Please know that this segmentation fault is wholly inconsequential to the restart and execution of the program. 
  - This benign `SEGFAULT` results as a consequence of the `ucontext_t` context variable that was written to and read from the checkpoint image file. This context variable is used to store certain register values of the process at the time the `SIGUSR2` signal was received. More specifically, the `ucontext_t` variable from `ucontext.h` does *not* store the data of the `%fs` register. Therefore, upon restart, the program's `%fs` register contains a garbage value which differs from the original `%fs` value at the time the original process was inturrupted. Because it so happens that `exit` uses this register value, the bad `%fs` register value here results in a segmentation fault. 
  - Therefore, this problem could be corrected by manually writing and later reading the value of this register somewhere in the checkpoint image file, and then setting its value equal to the register upon restart (please see the Areas for Future Improvement section below).
- <ins>Stack Smashing:</ins> On some computers, running `./restart` after generating a checkpoint image file will result in a stack smashing detected error. This issue can be avoided by compiling libckpt.c with the flag `-fno-stack-protector`.

## Implementation Details
- <ins>Creating Checkpoint File:</ins> The checkpoint image file `myckpt.dat` can be obtained by running `./ckpt ./a.out ...` and sending the active process a `SIGUSR2` signal from another terminal.
- <ins>Contents of Checkpoint File:</ins> The data of the checkpoint image file is written in the format of {HEADER1, DATA1, HEADER2, DATA2, ...}, where the header contains basic details on the data succeeding it (for more on the headers, pleasesee `struct ckpt_sgmt` in libckpt.c).
  - More specifically, HEADER1 and DATA1 correspond to the `ucontext_t` variable, which contains the context information at the moment in time the executing program received a `SIGUSR2` signal; all subsequent header-data pairs correspond to the memory layout at this time (use the command `cat /proc/self/maps` in the terminal to get a better sense of what this data looks like).
- <ins>Reading Checkpoint File:</ins> Once the checkpoint image file is created, its contents can be printed to `stdout` by running the command `./readckpt`.
  - Note: Only the data corresponding to the memory layout are printed, *not* the data corresponding to the `ucontext_t` context variable.
- <ins>Restarting via Checkpoint File:</ins> Once a checkpoint image file is generated, the program can be restarted at any time by running the command `./restart`.
  - Please see section above on inconsequential segmentation fault upon completion of restart.
- <ins>Test/Dummy Programs:</ins> To illustrate the functionality of this lightweight checkpointing program, the following dummy programs are included (note that they are compiled with the `-rdynamic` flag in the Makefile).
  - <ins>counting-test.c</ins>: A simple program taking a single integer as an argument, printing that number and the following 9 numbers (with 1 second between each print) before exiting.
  - <ins>hello-test.c</ins>: A very simple program that takes no arguments, printing "Hello world!" ten times before printing "Goodbye world!" and exiting. 

## Areas for Future Improvement

- [ ] Save the `%fs` register in `myckpt.dat` (the checkpoint image file), so that when the program restarts via `restart.c` and calls `setcontext` in `libckpt.c`, there is no segmentation fault at the very end of the program due to a bad `%fs` register value (as discussed above, `ucontext.h` predates the `%fs` register - a register which is called in `exit` - and so the register's value is lost when restarting).  
- [ ] Add support for multi-threaded programs. Initially, this project was designed to work only with single-threaded programs; this way, the checkpointing program need not concern itself with the intricacies of things like thread-local storage (TLS), etc.

## Screenshots & Video

  - To see this program in action, please click the screen recording of the example [linked here](https://www.youtube.com/watch?v=FrD10-QyvNs).

<p align="center">
  <img src="https://github.com/alex-w-99/Checkpointing-Program/blob/main/Images/checkpointing_screenshot1.png" width="500">
</p>

<p align="center">
<img src="https://github.com/alex-w-99/Checkpointing-Program/blob/main/Images/checkpointing_screenshot2.png" width="500">
</p>

## Acknowledgements 

- Professor Gene Cooperman, my Computer Systems professor.

## Contact Information

- Alexander Wilcox
- Email: alexander.w.wilcox@gmail.com
