# Mmap Implementation for XV6
This is a simple implementation of the mmap system call for XV6. It meets all of the requirements for the mmap lab assigned in MIT's 6.1810 course on operating systems, found [here.](https://pdos.csail.mit.edu/6.828/2024/) This implementation also meets some optional requirements, such as the sharing of physical memory between a parent and forked child process who share a memory mapped region of virtual memory.

Updates are still being made to add functionality, improve readability, and fix any logic errors not checked by the tests provided by MIT.

To run the tests used to grad this lab, run ```make grade``` in the root directory of this repository. This checks both that the mmap tests pass successfully and that none of XV6's core functionality has been broken by the mmap implementation.

I claim no ownership of the XV6 source code. The only code in this repository that is mine is that used to implement the mmap/munmap system calls.