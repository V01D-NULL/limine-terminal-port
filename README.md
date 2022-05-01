# Port of the Limine bootloader terminal

Normally this terminal is provided by the Limine bootloader and can (and should) be used by the kernel during early boot.

It's an extremely fast with a complete "terminfo"/vt100 implementation. There really isn't a reason not to use this terminal.

The only issue here is that it's merely an *early boot console*, and it's located in conventional, lower-half memory.

Once you get to userspace you'll find it very annoying to try and map memory *around* the it considering that terminal shouldn't be in lower-half memory in the first place!

That's what this port is for.
You should be able to include it into your kernel and use it just fine.

Please let us know if any issues arise, thank you!

## Features
* Almost every feature that Limine terminal supports
* Multiple terminals support in VBE mode

## Limitations
* Currently background loading is unsupported (broken)
* Text mode should work but is untested

## Usage

1. First off, choose a font from fonts/ folder or create your own and load it in your os (link it directly to the kernel, load it from filesystem, as a module, etc)

2. To initialize the terminal, include `term.hpp` and provide some basic functions declared in the header file.

3. Create new term_t object and run init() or use constructor (If you set bios to false, you will not be able to use text mode)

4. To use vbe mode with framebuffer, run `term_object->vbe(arguments);` (Example shown below)

5. To use text mode, run `term_object->textmode();`

## Example
```c
#include <term.hpp>

void *alloc_mem(size_t size)
{
   // Allocate memory
}
void free_mem(void *ptr, size_t size)
{
   // Free memory
}
void *memcpy(void *dest, const void *src, size_t len)
{
   // Memcpy
}
void *memset(void *dest, int ch, size_t n)
{
   // Memset
}

framebuffer_t frm
{
   address, // Framebuffer address
   width, // Framebuffer width
   height, // Framebuffer height
   pitch // Framebuffer pitch
};

font_t font
{
   font_address, // Address of font file
   8, // Font width
   16, // Font height
   1, // Character spacing
   0, // Font scaling x
   0 // Font scaling y
};

style_t style
{
   DEFAULT_ANSI_COLOURS, // Default terminal palette
   DEFAULT_ANSI_BRIGHT_COLOURS, // Default terminal bright palette
   0xA0000000, // Background colour
   0xFFFFFF, // Foreground colour
   64, // Terminal margin
   0 // Terminal margin gradient
};

// Background not working
image_t *image = new image(backgroundAddress, size);
background_t back
{
   image,
   STRETCHED, // STRETCHED, CENTERED or TILED
   0x00000000 // Terminal backdrop colour
};

callback_t callback = [](uint64_t, uint64_t, uint64_t, uint64_t, uint64_t) { handleCallback(); };
term_t *term = new term_t(callback, isBootedInBiosMode);

// VBE mode
// In VBE mode you can create more terminals for different framebuffers
term->vbe(frm, font, style); // Also pass `back` as argument for background
term->print("Hello, World!");

// Text mode
term->textmode();
term->print("Hello, World!");
```

Credits: https://github.com/limine-bootloader/limine