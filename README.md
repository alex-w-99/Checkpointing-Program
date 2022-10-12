# Lightweight Checkpointing Program

asdf

fs register, predated by ucontext, etc

rdynamic compilation flag

dummy programs to run ckpt with (counting-test, hello-test)

Tested on Ubuntu 20.04.1, compiled with `gcc -std=gnu17 ...` (used in Makefile).

## Implementation Details

- asdf:
  - asdf

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
