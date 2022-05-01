#pragma once

#include "term.hpp"

static constexpr uint8_t VD_COLS = 160;
static constexpr uint8_t VD_ROWS = 25;
static constexpr uint16_t VIDEO_BOTTOM = (160 * 25) - 1;

struct tterm_context
{
    size_t cursor_offset;
    bool cursor_status;
    uint8_t text_palette;
    bool scroll_enabled;

    uint8_t saved_state_text_palette;
    size_t saved_state_cursor_offset;
};

class tterm_t
{
    private:
    volatile uint8_t *video_mem = reinterpret_cast<uint8_t*>(0xB8000);
    uint8_t *back_buffer = nullptr;
    uint8_t *front_buffer = nullptr;

    size_t old_cursor_offset = 0;

    tterm_context context;
    term_t *term;

    void draw_cursor();

    public:
    void init(term_t *term, size_t &rows, size_t &cols);

    void putchar(uint8_t c);
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
};