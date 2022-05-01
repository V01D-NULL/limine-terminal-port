#pragma once

#include "term.hpp"

struct gterm_char
{
    uint32_t c;
    uint32_t fg;
    uint32_t bg;
};

struct gterm_queue_item
{
    size_t x, y;
    gterm_char c;
};

struct gterm_context
{
    uint32_t text_fg;
    uint32_t text_bg;
    bool cursor_status;
    size_t cursor_x;
    size_t cursor_y;
    bool scroll_enabled;

    uint32_t saved_state_text_fg;
    uint32_t saved_state_text_bg;
    size_t saved_state_cursor_x;
    size_t saved_state_cursor_y;
};

class gterm_t
{
    private:
    framebuffer_t framebuffer;
    volatile uint32_t *framebuffer_addr;

    term_t *term;

    size_t vga_font_width;
    size_t vga_font_height;
    size_t glyph_width = 8;
    size_t glyph_height = 16;

    size_t vga_font_scale_x = 1;
    size_t vga_font_scale_y = 1;

    size_t offset_x, offset_y;

    uint8_t *vga_font_bits = nullptr;
    size_t vga_font_bool_size = 0;
    bool *vga_font_bool = nullptr;

    uint32_t ansi_colours[8];
    uint32_t ansi_bright_colours[8];
    uint32_t default_fg, default_bg;

    image_t *background;

    size_t bg_canvas_size = 0;
    uint32_t *bg_canvas = nullptr;

    size_t rows;
    size_t cols;
    size_t margin;
    size_t margin_gradient;

    size_t grid_size = 0;
    size_t queue_size = 0;
    size_t map_size = 0;

    gterm_char *grid = nullptr;

    gterm_queue_item *queue = nullptr;
    size_t queue_i = 0;

    gterm_queue_item **map = nullptr;

    gterm_context context;

    size_t old_cursor_x = 0;
    size_t old_cursor_y = 0;

    uint32_t colour_blend(uint32_t fg, uint32_t bg);
    void plot_px(size_t x, size_t y, uint32_t hex);
    uint32_t blend_gradient_from_box(size_t x, size_t y, uint32_t bg_px, uint32_t hex);
    void genloop(size_t xstart, size_t xend, size_t ystart, size_t yend, uint32_t (gterm_t::*blend)(size_t x, size_t y, uint32_t orig));
    void generate_canvas();
    void plot_char(gterm_char *c, size_t x, size_t y);
    void plot_char_fast(gterm_char *old, gterm_char *c, size_t x, size_t y);
    bool compare_char(gterm_char *a, gterm_char *b);
    void push_to_queue(gterm_char *c, size_t x, size_t y);
    void draw_cursor();

    uint32_t blend_external(size_t x, size_t y, uint32_t orig)
    {
        return orig;
    }
    uint32_t blend_internal(size_t x, size_t y, uint32_t orig)
    {
        return colour_blend(default_bg, orig);
    }
    uint32_t blend_margin(size_t x, size_t y, uint32_t orig)
    {
        return blend_gradient_from_box(x, y, orig, default_bg);
    }

    void loop_external(size_t xstart, size_t xend, size_t ystart, size_t yend)
    {
        genloop(xstart, xend, ystart, yend, &gterm_t::blend_external);
    }
    void loop_margin(size_t xstart, size_t xend, size_t ystart, size_t yend)
    {
        genloop(xstart, xend, ystart, yend, &gterm_t::blend_margin);
    }
    void loop_internal(size_t xstart, size_t xend, size_t ystart, size_t yend)
    {
        genloop(xstart, xend, ystart, yend, &gterm_t::blend_internal);
    }

    public:
    bool init(term_t *term, framebuffer_t frm, font_t font, style_t style, background_t back);
    void deinit();

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