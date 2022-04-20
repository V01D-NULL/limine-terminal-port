# This is a port of the runtime terminal for the limine bootloader

**NOTE: This terminal port should work with any bootloader as long as you add support for it's respective protocol (see below)**

# Supported protocols

   > Note: Adding support for a different boot protocol is as simple as changing two parameters in `term_init` and editing the bump allocator in `gterm.c` (The logic should more or less stay the same)

- stivale2

Normally this terminal is provided by the bootloader and can (and should) be used by the kernel during early boot.

It's an extremely fast terminal with a complete "terminfo"/vt100 implementation. There really isn't a reason not to use this terminal.

The only issue here is that it's merely an *early boot console*, and it's located in conventional, lower-half memory.

Once you get you userspace you'll find it very annoying to try and map memory *around* the terminal considering a terminal shouldn't be in lower-half memory in the first place!

That's why I decided to port it, you should be able to include it into your kernel and use it just fine. 

Please let me know if any issues arise, thanks!

# Features / limitations

For the sake of simplicity (and my time) this port is a stripped down version of the original, meaning it does not support fancy things like font scaling, image rendering, etc.

I may or may not add support later on, PR's are welcome if you're eager to have it.

# Usage

First off, make sure to convert your font.bin into an object file and link it with the kernel. The command is `ld -r -b binary font.bin -o font.o`

To initialize the terminal, include `term.h` and pass the stivale2 framebuffer and memory map structures as arguments.

**Note: The datastructures used by the terminal are initialized by a simple bump allocator (see: gterm.c) which takes memory from the mmap provided by the bootloader.
In order to make this fully higherhalf you will have to replace bump() with your kernels heap allocator. It only exists here as a PoC to get you up and running.**

(Pagefaults did occur with the limine's pagetables when the external font was memcpy'd, however this is not an early boot console. You should be using the bootloader provided terminal until you setup your own pagetables)
