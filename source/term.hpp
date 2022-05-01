#pragma once

#include "image.hpp"

extern void *alloc_mem(size_t size);
extern void free_mem(void *ptr, size_t size);
extern void *memcpy(void *dest, const void *src, size_t len);
extern void *memset(void *dest, int ch, size_t n);

static constexpr uint16_t VGA_FONT_MAX = 16384;
static constexpr uint16_t VGA_FONT_GLYPHS = 256;

static constexpr uint8_t TERM_TABSIZE = 8;
static constexpr uint8_t MAX_ESC_VALUES = 16;

static constexpr uint64_t TERM_CTX_SIZE = static_cast<uint64_t>(-1);
static constexpr uint64_t TERM_CTX_SAVE = static_cast<uint64_t>(-2);
static constexpr uint64_t TERM_CTX_RESTORE = static_cast<uint64_t>(-3);
static constexpr uint64_t TERM_FULL_REFRESH = static_cast<uint64_t>(-4);

static constexpr uint8_t CHARSET_DEFAULT = 0;
static constexpr uint8_t CHARSET_DEC_SPECIAL = 1;

static uint32_t DEFAULT_ANSI_COLOURS[8] = { 0x00000000, 0x00AA0000, 0x0000AA00, 0x00AA5500, 0x000000AA, 0x00AA00AA, 0x0000AAAA, 0x00AAAAAA };
static uint32_t DEFAULT_ANSI_BRIGHT_COLOURS[8] = { 0x00555555, 0x00FF5555, 0x0055FF55, 0x00FFFF55, 0x005555FF, 0x00FF55FF, 0x0055FFFF, 0x00FFFFFF };

static constexpr uint32_t DEFAULT_BACKGROUND = 0x00000000; // Black
static constexpr uint32_t DEFAULT_FOREGROUND = 0x00AAAAAA; // Grey

using callback_t = void (*)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);
using fixedp6 = size_t;

static inline size_t fixedp6_to_int(fixedp6 value)
{
    return value / 64;
}
static inline fixedp6 int_to_fixedp6(size_t value)
{
    return value * 64;
}

enum callbacks
{
    TERM_CB_DEC = 10,
    TERM_CB_BELL = 20,
    TERM_CB_PRIVATE_ID = 30,
    TERM_CB_STATUS_REPORT = 40,
    TERM_CB_POS_REPORT = 50,
    TERM_CB_KBD_LEDS = 60,
    TERM_CB_MODE = 70,
    TERM_CB_LINUX = 80
};

enum term_type
{
    NOT_READY,
    VBE,
    TEXTMODE
};

struct framebuffer_t
{
    uint64_t address;
    uint16_t width;
    uint16_t height;
    uint16_t pitch;
};

struct font_t
{
    uint64_t address;
    uint8_t width = 8;
    uint8_t height = 16;
    uint8_t spacing = 1;
    uint8_t scale_x = 0;
    uint8_t scale_y = 0;
};

struct style_t
{
    uint32_t *ansi_colours = DEFAULT_ANSI_COLOURS;
    uint32_t *ansi_bright_colours = DEFAULT_ANSI_BRIGHT_COLOURS;
    uint32_t background = DEFAULT_BACKGROUND;
    uint32_t foreground = DEFAULT_FOREGROUND;
    uint16_t margin = -1;
    uint16_t margin_gradient = -1;
};

struct background_t
{
    image_t *background = nullptr;
    image_style style = TILED;
    uint32_t backdrop = 0x000000;
};

struct term_context
{
    bool control_sequence;
    bool csi;
    bool escape;
    bool rrr;
    bool discard_next;
    bool bold;
    bool reverse_video;
    bool dec_private;
    bool insert_mode;
    uint8_t g_select;
    uint8_t charsets[2];
    size_t current_charset;
    size_t escape_offset;
    size_t esc_values_i;
    size_t saved_cursor_x;
    size_t saved_cursor_y;
    size_t current_primary;
    size_t scroll_top_margin;
    size_t scroll_bottom_margin;
    uint32_t esc_values[MAX_ESC_VALUES];

    bool saved_state_bold;
    bool saved_state_reverse_video;
    size_t saved_state_current_charset;
    size_t saved_state_current_primary;
};

class gterm_t;
class tterm_t;

class term_t
{
    private:
    term_context context;
    gterm_t *gterm;
    tterm_t *tterm;

    bool bios = false;

    void sgr();
    void dec_private_parse(uint8_t c);
    void linux_private_parse();
    void mode_toggle(uint8_t c);
    void control_sequence_parse(uint8_t c);
    void escape_parse(uint8_t c);
    uint8_t dec_special_to_cp437(uint8_t c);

    public:
    friend class gterm_t;
    friend class tterm_t;

    bool initialised = false;

    term_type term_backend = NOT_READY;
    size_t rows = 0, cols = 0;
    bool runtime;

    uint64_t arg;
    bool autoflush;

    void init(callback_t callback, bool bios);
    term_t(callback_t callback, bool bios);

    void reinit();
    void deinit();
    void vbe(framebuffer_t frm, font_t font, style_t style = style_t(), background_t back = background_t());
    void textmode();
    void notready();
    void putchar(uint8_t c);
    void write(uint64_t buf, uint64_t count);
    void print(const char *str);

    void raw_putchar(uint8_t c);
    void clear(bool move);
    void enable_cursor();
    bool disable_cursor();
    void set_cursor_pos(size_t x, size_t y);
    void get_cursor_pos(size_t &x, size_t &y);
    void set_text_fg(size_t fg);
    void set_text_bg(size_t bg);
    void set_text_fg_bright(size_t fg);
    void set_text_bg_bright(size_t bg);
    void set_text_fg_default();
    void set_text_bg_default();
    bool scroll_disable();
    void scroll_enable();
    void move_character(size_t new_x, size_t new_y, size_t old_x, size_t old_y);
    void scroll();
    void revscroll();
    void swap_palette();
    void save_state();
    void restore_state();

    void double_buffer_flush();

    uint64_t context_size();
    void context_save(uint64_t ptr);
    void context_restore(uint64_t ptr);
    void full_refresh();

    callback_t callback;
};