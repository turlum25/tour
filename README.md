<h1 align="center">TOUR</h1>
<p align="center">
  <img width="350" height="332.5" alt="Frame 15" src="https://github.com/user-attachments/assets/28257ebd-0480-4a95-a9fc-945c4d311126" />

<p align="center">
  Of course the code worsens your posture
</p>

# What is TOUR?
TOUR (TOUR Offends UNIX Rules) is a lightweight OS made to run on QEMU (without GRUB) and real hardware, made on C and Assembly.

This operating system was made to be modular but it's basically spaghetti code to me now. I die

# What can TOUR do?

- Read and wteri SATA disk sectors
- Detect total RAM and how much RAM is taken
- Detect and write username and hostname in disk (Username: sector 10, hostname: sector 9)
- Look cool and haxxor moed

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
- To run command, run `make run` / `make grubrun` (use `make makedisk`, then `make rundisk`/`make grub-rundisk` to run with SATA R/W support). For deleting the OS IMG, ISO and the .o files, run `make clean`.

# Commands

```
help                  - Displays the command and its usage

ver                   - Shows version of OS (Obsolete)
fetch                 - Shows detailed info about the computer (total RAM, usage, CPU)

peek                  - Displays specific bytes of specific memory addresses in text (Usage : peek 0x[ADDRESS]) 
poke                  - Edits specific bytes of specific memory addresses in text (Usage : poke 0x[ADDRESS] [VALUE])
strpeek               - Displays sector bytes in text (512 bytes, same usage as peek)
strpeek               - Edits sector bytes in text (512 bytes, same usage as poke)

echo                  - Repeats text (usage: echo TEXT)
time                  - Shows time since boot in seconds
sleep                 - Wait for specific time
clear                 - Clear screen

reboot                - Reboot system (might shut down in certain VMs e.g VirtualBox)

```

There are some more debug commands:
```
enabledebug          - Enables int debug in kernel into 1. Not mentioned in help
disabledebug         - Disables int debug in kernel into 0. Not mentioned in help
panic                - Purposefully panics your system (Added 0.05b1)
regdump              - Dumps host computer's registers
```

# Can TOUR run on real hardware?

Oh yes it can! TOUR runs really well in my laptop (Xiaomi Notebook Air 13.3 2016). If it can run on that, then it can run on *your* computer too!

But there is obviously a catch: My terrible bootloader obviously will never work for real silicon. So, simply use the GRUB version.

# Common questions

- Wen eta better TOUR bootloadr?

Never. Use GRUB if you wanna.

- Does it run Doom yet?

It can't even run programs, what do you think?

- Why can't it run on lower hardware specs?

It's not like I'm managing NetBSD myself, you know. Either you're challenging me or you have the worst computer known to man



