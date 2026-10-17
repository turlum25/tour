<h1 align="center">TOUR</h1>
<p align="center">
  <img width="350" height="332.5" alt="Frame 15" src="https://github.com/user-attachments/assets/28257ebd-0480-4a95-a9fc-945c4d311126" />

  
  
# What is TOUR?
TOUR (TOUR Offends UNIX Rules) is a lightweight OS made to run on QEMU (without GRUB) and real hardware, made on C and Assembly.

This operating system was made to be modular but it's basically spaghetti code to me now. I die

# Minimum specs to run TOUR

- 4MB (GRUB version) / 1,005KB (GRUB-less version) RAM
- Intel i486 (always locked here. who uses a 386)

# Building TOUR

First off, you need:
- A computer (mrObvious strikes again)
- Linux or Intel macOS (not mentioning Macs because compiling in Hackintosh is always possible)
- GCC, NASM, Xorriso, QEMU
- At least 4 gigs of RAM.
- A human brain and at least some IQ points to click and type on a terminal.

How to compile:
- CD to your TOUR folder.
- Run command: `make` to make GRUB-less version, `make grub` to make the GRUB version (in case you are not a psychopath and compile it yourself)
- To run command, run `make run` (use `make makedisk`, then `make rundisk` to run with SATA R/W support). For deleting the OS IMG, ISO and the .o files, run `make clean`.

# Common questions

- Wen eta better TOUR bootloadr?

Never. Use GRUB if you wanna.

- Does it run Doom yet?

It can't even run programs, what do you think?

- Why can't it run on lower hardware specs?

It's not like I'm managing NetBSD myself, you know. Either you're challenging me or you have the worst computer known to man



