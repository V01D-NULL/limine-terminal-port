#include "image.hpp"
#include "gterm.hpp"
#include "term.hpp"

static uint64_t sqrt(uint64_t a_nInput)
{
    uint64_t op  = a_nInput;
    uint64_t res = 0;
    uint64_t one = 1UL << 62;

    while (one > op) one >>= 2;

    while (one != 0)
    {
        if (op >= res + one)
        {
            op = op - (res + one);
            res = res +  2 * one;
        }
        res >>= 1;
        one >>= 2;
    }

    return res;
}

void gterm_t::save_state()
{
    context.saved_state_text_fg = context.text_fg;
    context.saved_state_text_bg = context.text_bg;
    context.saved_state_cursor_x = context.cursor_x;
    context.saved_state_cursor_y = context.cursor_y;
}

void gterm_t::restore_state()
{
    context.text_fg = context.saved_state_text_fg;
    context.text_bg = context.saved_state_text_bg;
    context.cursor_x = context.saved_state_cursor_x;
    context.cursor_y = context.saved_state_cursor_y;
}

void gterm_t::swap_palette()
{
    uint32_t tmp = context.text_bg;
    context.text_bg = context.text_fg;
    context.text_fg = tmp;
}

#define A(rgb) static_cast<uint8_t>(rgb >> 24)
#define R(rgb) static_cast<uint8_t>(rgb >> 16)
#define G(rgb) static_cast<uint8_t>(rgb >> 8)
#define B(rgb) static_cast<uint8_t>(rgb)
#define ARGB(a, r, g, b) ((a << 24) | (static_cast<uint8_t>(r) << 16) | (static_cast<uint8_t>(g) << 8) | static_cast<uint8_t>(b))

uint32_t gterm_t::colour_blend(uint32_t fg, uint32_t bg)
{
    unsigned alpha = 255 - A(fg);
    unsigned inv_alpha = A(fg) + 1;

    uint8_t r = static_cast<uint8_t>((alpha * R(fg) + inv_alpha * R(bg)) / 256);
    uint8_t g = static_cast<uint8_t>((alpha * G(fg) + inv_alpha * G(bg)) / 256);
    uint8_t b = static_cast<uint8_t>((alpha * B(fg) + inv_alpha * B(bg)) / 256);

    return ARGB(0, r, g, b);
}

void gterm_t::plot_px(size_t x, size_t y, uint32_t hex)
{
    if (x >= framebuffer.width || y >= framebuffer.height) return;

    size_t fb_i = x + (framebuffer.pitch / sizeof(uint32_t)) * y;

    framebuffer_addr[fb_i] = hex;
}

uint32_t gterm_t::blend_gradient_from_box(size_t x, size_t y, uint32_t bg_px, uint32_t hex)
{
    size_t distance, x_distance, y_distance;
    size_t gradient_stop_x = framebuffer.width - margin;
    size_t gradient_stop_y = framebuffer.height - margin;

    if (x < margin) x_distance = margin - x;
    else x_distance = x - gradient_stop_x;

    if (y < margin) y_distance = margin - y;
    else y_distance = y - gradient_stop_y;

    if (x >= margin && x < gradient_stop_x) distance = y_distance;
    else if (y >= margin && y < gradient_stop_y) distance = x_distance;
    else distance = sqrt(static_cast<uint64_t>(x_distance) * static_cast<uint64_t>(x_distance) + static_cast<uint64_t>(y_distance) * static_cast<uint64_t>(y_distance));

    if (distance > margin_gradient) return bg_px;

    uint8_t gradient_step = (0xff - A(hex)) / margin_gradient;
    uint8_t new_alpha = A(hex) + gradient_step * distance;

    return colour_blend((hex & 0xffffff) | (new_alpha << 24), bg_px);
}

void gterm_t::genloop(size_t xstart, size_t xend, size_t ystart, size_t yend, uint32_t (gterm_t::*blend)(size_t x, size_t y, uint32_t orig))
{
    uint8_t *img = background->img;
    const size_t img_width = background->img_width, img_height = background->img_height, img_pitch = background->pitch, colsize = background->bpp / 8;

    switch (background->type)
    {
        case TILED:
            for (size_t y = ystart; y < yend; y++)
            {
                size_t image_y = y % img_height, image_x = xstart % img_width;
                const size_t off = img_pitch * (img_height - 1 - image_y);
                size_t canvas_off = framebuffer.width * y, fb_off = framebuffer.pitch / 4 * y;
                for (size_t x = xstart; x < xend; x++)
                {
                    uint32_t img_pixel = *reinterpret_cast<uint32_t*>(img + image_x * colsize + off);
                    uint32_t i = (this->*blend)(x, y, img_pixel);
                    bg_canvas[canvas_off + x] = i;
                    framebuffer_addr[fb_off + x] = i;
                    if (image_x++ == img_width) image_x = 0;
                }
            }
            break;

        case CENTERED:
            for (size_t y = ystart; y < yend; y++)
            {
                size_t image_y = y - background->y_displacement;
                const size_t off = img_pitch * (img_height - 1 - image_y);
                size_t canvas_off = framebuffer.width * y, fb_off = framebuffer.pitch / 4 * y;
                if (image_y >= background->y_size)
                {
                    for (size_t x = xstart; x < xend; x++)
                    {
                        uint32_t i = (this->*blend)(x, y, background->back_colour);
                        bg_canvas[canvas_off + x] = i;
                        framebuffer_addr[fb_off + x] = i;
                    }
                }
                else
                {
                    for (size_t x = xstart; x < xend; x++)
                    {
                        size_t image_x = (x - background->x_displacement);
                        bool x_external = image_x >= background->x_size;
                        uint32_t img_pixel = *reinterpret_cast<uint32_t*>(img + image_x * colsize + off);
                        uint32_t i = (this->*blend)(x, y, x_external ? background->back_colour : img_pixel);
                        bg_canvas[canvas_off + x] = i;
                        framebuffer_addr[fb_off + x] = i;
                    }
                }
            }
            break;
        case STRETCHED:
            for (size_t y = ystart; y < yend; y++)
            {
                size_t img_y = (y * img_height) / framebuffer.height;
                size_t off = img_pitch * (img_height - 1 - img_y);
                size_t canvas_off = framebuffer.width * y, fb_off = framebuffer.pitch / 4 * y;

                size_t ratio = int_to_fixedp6(img_width) / framebuffer.width;
                fixedp6 img_x = ratio * xstart;
                for (size_t x = xstart; x < xend; x++)
                {
                    uint32_t img_pixel = *reinterpret_cast<uint32_t*>(img + fixedp6_to_int(img_x) * colsize + off);
                    uint32_t i = (this->*blend)(x, y, img_pixel);
                    bg_canvas[canvas_off + x] = i; framebuffer_addr[fb_off + x] = i;
                    img_x += ratio;
                }
            }
            break;
    }
}

void gterm_t::generate_canvas()
{
    if (this->background)
    {
        int64_t margin_no_gradient = static_cast<int64_t>(margin) - margin_gradient;

        if (margin_no_gradient < 0) margin_no_gradient = 0;

        size_t scan_stop_x = framebuffer.width - margin_no_gradient;
        size_t scan_stop_y = framebuffer.height - margin_no_gradient;

        loop_external(0, framebuffer.width, 0, margin_no_gradient);
        loop_external(0, framebuffer.width, scan_stop_y, framebuffer.height);
        loop_external(0, margin_no_gradient, margin_no_gradient, scan_stop_y);
        loop_external(scan_stop_x, framebuffer.width, margin_no_gradient, scan_stop_y);

        size_t gradient_stop_x = framebuffer.width - margin;
        size_t gradient_stop_y = framebuffer.height - margin;

        if (margin_gradient)
        {
            loop_margin(margin_no_gradient, scan_stop_x, margin_no_gradient, margin);
            loop_margin(margin_no_gradient, scan_stop_x, gradient_stop_y, scan_stop_y);
            loop_margin(margin_no_gradient, margin, margin, gradient_stop_y);
            loop_margin(gradient_stop_x, scan_stop_x, margin, gradient_stop_y);
        }

        loop_internal(margin, gradient_stop_x, margin, gradient_stop_y);
    }
    else
    {
        for (size_t y = 0; y < framebuffer.height; y++)
        {
            for (size_t x = 0; x < framebuffer.width; x++)
            {
                bg_canvas[y * framebuffer.width + x] = default_bg;
                plot_px(x, y, default_bg);
            }
        }
    }
}

void gterm_t::plot_char(gterm_char *c, size_t x, size_t y)
{
    if (x >= cols || y >= rows) return;

    x = offset_x + x * glyph_width;
    y = offset_y + y * glyph_height;

    bool *glyph = &vga_font_bool[c->c * vga_font_height * vga_font_width];

    for (size_t gy = 0; gy < glyph_height; gy++)
    {
        uint8_t fy = gy / vga_font_scale_y;
        volatile uint32_t *fb_line = framebuffer_addr + x + (y + gy) * (framebuffer.pitch / 4);
        uint32_t *canvas_line = bg_canvas + x + (y + gy) * framebuffer.width;
        for (size_t fx = 0; fx < vga_font_width; fx++)
        {
            bool draw = glyph[fy * vga_font_width + fx];
            for (size_t i = 0; i < vga_font_scale_x; i++)
            {
                size_t gx = vga_font_scale_x * fx + i;
                uint32_t bg = c->bg == 0xffffffff ? canvas_line[gx] : c->bg;
                uint32_t fg = c->fg == 0xffffffff ? canvas_line[gx] : c->fg;
                fb_line[gx] = draw ? fg : bg;
            }
        }
    }
}

void gterm_t::plot_char_fast(gterm_char *old, gterm_char *c, size_t x, size_t y)
{
    if (x >= cols || y >= rows) return;

    x = offset_x + x * glyph_width;
    y = offset_y + y * glyph_height;

    bool *new_glyph = &vga_font_bool[c->c * vga_font_height * vga_font_width];
    bool *old_glyph = &vga_font_bool[old->c * vga_font_height * vga_font_width];
    for (size_t gy = 0; gy < glyph_height; gy++)
    {
        uint8_t fy = gy / vga_font_scale_y;
        volatile uint32_t *fb_line = framebuffer_addr + x + (y + gy) * (framebuffer.pitch / 4);
        uint32_t *canvas_line = bg_canvas + x + (y + gy) * framebuffer.width;
        for (size_t fx = 0; fx < vga_font_width; fx++)
        {
            bool old_draw = old_glyph[fy * vga_font_width + fx];
            bool new_draw = new_glyph[fy * vga_font_width + fx];
            if (old_draw == new_draw) continue;

            for (size_t i = 0; i < vga_font_scale_x; i++)
            {
                size_t gx = vga_font_scale_x * fx + i;
                uint32_t bg = c->bg == 0xffffffff ? canvas_line[gx] : c->bg;
                uint32_t fg = c->fg == 0xffffffff ? canvas_line[gx] : c->fg;
                fb_line[gx] = new_draw ? fg : bg;
            }
        }
    }
}

bool gterm_t::compare_char(gterm_char *a, gterm_char *b)
{
    return !(a->c != b->c || a->bg != b->bg || a->fg != b->fg);
}

void gterm_t::push_to_queue(gterm_char *c, size_t x, size_t y)
{
    if (x >= cols || y >= rows) return;

    size_t i = y * cols + x;

    gterm_queue_item *q = map[i];

    if (q == nullptr)
    {
        if (compare_char(&grid[i], c)) return;
        q = &queue[queue_i++];
        q->x = x;
        q->y = y;
        map[i] = q;
    }

    q->c = *c;
}

bool gterm_t::scroll_disable()
{
    bool ret = context.scroll_enabled;
    context.scroll_enabled = false;
    return ret;
}

void gterm_t::scroll_enable()
{
    context.scroll_enabled = true;
}

void gterm_t::revscroll() {
    for (size_t i = (term->context.scroll_bottom_margin - 1) * cols - 1; ; i--)
    {
        gterm_char *c;
        gterm_queue_item *q = map[i];
        if (q != nullptr) c = &q->c;
        else c = &grid[i];

        push_to_queue(c, (i + cols) % cols, (i + cols) / cols);
        if (i == term->context.scroll_top_margin * cols) break;
    }

    gterm_char empty;
    empty.c  = ' ';
    empty.fg = context.text_fg;
    empty.bg = context.text_bg;

    for (size_t i = term->context.scroll_top_margin * cols; i < (term->context.scroll_top_margin + 1) * cols; i++)
    {
        push_to_queue(&empty, i % cols, i / cols);
    }
}

void gterm_t::scroll()
{
    for (size_t i = (term->context.scroll_top_margin + 1) * cols; i < term->context.scroll_bottom_margin * cols; i++)
    {
        gterm_char *c;
        gterm_queue_item *q = map[i];
        if (q != nullptr) c = &q->c;
        else c = &grid[i];
        push_to_queue(c, (i - cols) % cols, (i - cols) / cols);
    }

    gterm_char empty;
    empty.c  = ' ';
    empty.fg = context.text_fg;
    empty.bg = context.text_bg;
    for (size_t i = (term->context.scroll_bottom_margin - 1) * cols; i < term->context.scroll_bottom_margin * cols; i++)
    {
        push_to_queue(&empty, i % cols, i / cols);
    }
}

void gterm_t::clear(bool move)
{
    gterm_char empty;
    empty.c  = ' ';
    empty.fg = context.text_fg;
    empty.bg = context.text_bg;
    for (size_t i = 0; i < rows * cols; i++)
    {
        push_to_queue(&empty, i % cols, i / cols);
    }

    if (move)
    {
        context.cursor_x = 0;
        context.cursor_y = 0;
    }
}

void gterm_t::enable_cursor()
{
    context.cursor_status = true;
}

bool gterm_t::disable_cursor()
{
    bool ret = context.cursor_status;
    context.cursor_status = false;
    return ret;
}

void gterm_t::set_cursor_pos(size_t x, size_t y)
{
    if (x >= cols)
    {
        if (static_cast<int>(x) < 0) x = 0;
        else x = cols - 1;
    }
    if (y >= rows)
    {
        if (static_cast<int>(y) < 0) y = 0;
        else y = rows - 1;
    }
    context.cursor_x = x;
    context.cursor_y = y;
}

void gterm_t::get_cursor_pos(size_t &x, size_t &y)
{
    x = context.cursor_x;
    y = context.cursor_y;
}

void gterm_t::move_character(size_t new_x, size_t new_y, size_t old_x, size_t old_y)
{
    if (old_x >= cols || old_y >= rows || new_x >= cols || new_y >= rows) return;

    size_t i = old_x + old_y * cols;

    gterm_char *c;
    gterm_queue_item *q = map[i];
    if (q != nullptr) c = &q->c;
    else c = &grid[i];

    push_to_queue(c, new_x, new_y);
}

void gterm_t::set_text_fg(size_t fg)
{
    context.text_fg = ansi_colours[fg];
}

void gterm_t::set_text_bg(size_t bg)
{
    context.text_bg = ansi_colours[bg];
}

void gterm_t::set_text_fg_bright(size_t fg)
{
    context.text_fg = ansi_bright_colours[fg];
}

void gterm_t::set_text_bg_bright(size_t bg)
{
    context.text_bg = ansi_bright_colours[bg];
}

void gterm_t::set_text_fg_default()
{
    context.text_fg = default_fg;
}

void gterm_t::set_text_bg_default()
{
    context.text_bg = 0xFFFFFFFF;
}

void gterm_t::draw_cursor()
{
    size_t i = context.cursor_x + context.cursor_y * cols;
    gterm_char c;
    gterm_queue_item *q = map[i];
    if (q != nullptr) c = q->c;
    else c = grid[i];

    uint32_t tmp = c.fg;
    c.fg = c.bg;
    c.bg = tmp;
    plot_char(&c, context.cursor_x, context.cursor_y);
    if (q != nullptr)
    {
        grid[i] = q->c;
        map[i] = nullptr;
    }
}

void gterm_t::double_buffer_flush()
{
    if (context.cursor_status) draw_cursor();

    for (size_t i = 0; i < queue_i; i++)
    {
        gterm_queue_item *q = &queue[i];
        size_t offset = q->y * cols + q->x;
        if (map[offset] == nullptr) continue;

        gterm_char *old = &grid[offset];
        if (q->c.bg == old->bg && q->c.fg == old->fg)
        {
            plot_char_fast(old, &q->c, q->x, q->y);
        }
        else plot_char(&q->c, q->x, q->y);

        grid[offset] = q->c;
        map[offset] = nullptr;
    }

    if ((old_cursor_x != context.cursor_x || old_cursor_y != context.cursor_y) || context.cursor_status == false)
    {
        plot_char(&grid[old_cursor_x + old_cursor_y * cols], old_cursor_x, old_cursor_y);
    }

    old_cursor_x = context.cursor_x;
    old_cursor_y = context.cursor_y;

    queue_i = 0;
}

void gterm_t::putchar(uint8_t c)
{
    gterm_char ch;
    ch.c  = c;
    ch.fg = context.text_fg;
    ch.bg = context.text_bg;
    push_to_queue(&ch, context.cursor_x++, context.cursor_y);
    if (context.cursor_x == cols && (context.cursor_y < term->context.scroll_bottom_margin - 1 || context.scroll_enabled))
    {
        context.cursor_x = 0;
        context.cursor_y++;
    }
    if (context.cursor_y == term->context.scroll_bottom_margin)
    {
        context.cursor_y--;
        scroll();
    }
}

bool gterm_t::init(term_t *term, framebuffer_t frm, font_t font, style_t style, background_t back)
{
    if (font.address == 0) return false;

    this->term = term;
    framebuffer = frm;
    framebuffer_addr = reinterpret_cast<volatile uint32_t*>(frm.address);

    context.cursor_status = true;
    context.scroll_enabled = true;

    margin = 64;
    margin_gradient = 4;

    memcpy(ansi_colours, style.ansi_colours, 8);
    memcpy(ansi_bright_colours, style.ansi_bright_colours, 8);

    default_bg = style.background;
    default_fg = style.foreground;

    context.text_fg = style.foreground;
    context.text_bg = 0xFFFFFFFF;

    background = back.background;

    if (background == nullptr)
    {
        margin = 0;
        margin_gradient = 0;
    }
    else if (default_bg == 0) default_bg = 0x68000000;

    if (style.margin != static_cast<uint16_t>(-1)) margin = style.margin;
    if (style.margin_gradient != static_cast<uint16_t>(-1)) margin_gradient = style.margin_gradient;

    if (background != nullptr)
    {
        if (back.style == CENTERED)
        {
            background->make_centered(framebuffer.width, framebuffer.height, back.backdrop);
        }
        else if (back.style == STRETCHED)
        {
            background->make_stretched(framebuffer.width, framebuffer.height);
        }
    }

    vga_font_width = font.width;
    vga_font_height = font.height;

    size_t font_bytes = (vga_font_width * vga_font_height * VGA_FONT_GLYPHS) / 8;

    vga_font_bits = static_cast<uint8_t*>(alloc_mem(VGA_FONT_MAX));
    memcpy(vga_font_bits, reinterpret_cast<void*>(font.address), font_bytes);

    vga_font_width += font.spacing;

    vga_font_bool_size = VGA_FONT_GLYPHS * vga_font_height * vga_font_width * sizeof(bool);
    vga_font_bool = static_cast<bool*>(alloc_mem(vga_font_bool_size));

    for (size_t i = 0; i < VGA_FONT_GLYPHS; i++)
    {
        uint8_t *glyph = &vga_font_bits[i * vga_font_height];

        for (size_t y = 0; y < vga_font_height; y++)
        {
            for (size_t x = 0; x < 8; x++)
            {
                size_t offset = i * vga_font_height * vga_font_width + y * vga_font_width + x;

                if ((glyph[y] & (0x80 >> x))) vga_font_bool[offset] = true;
                else vga_font_bool[offset] = false;
            }

            for (size_t x = 8; x < vga_font_width; x++)
            {
                size_t offset = i * vga_font_height * vga_font_width + y *  vga_font_width + x;

                if (i >= 0xC0 && i <= 0xDF) vga_font_bool[offset] = (glyph[y] & 1);
                else vga_font_bool[offset] = false;
            }
        }
    }

    vga_font_scale_x = 1;
    vga_font_scale_y = 1;

    if (font.scale_x || font.scale_y)
    {
        vga_font_scale_x = font.scale_x;
        vga_font_scale_y = font.scale_y;
        if (vga_font_scale_x > 8 || vga_font_scale_y > 8)
        {
            vga_font_scale_x = 1;
            vga_font_scale_y = 1;
        }
    }

    glyph_width = vga_font_width * vga_font_scale_x;
    glyph_height = vga_font_height * vga_font_scale_y;

    cols = term->cols = (framebuffer.width - margin * 2) / glyph_width;
    rows = term->rows = (framebuffer.height - margin * 2) / glyph_height;

    offset_x = margin + ((framebuffer.width - margin * 2) % glyph_width) / 2;
    offset_y = margin + ((framebuffer.height - margin * 2) % glyph_height) / 2;

    grid_size = rows * cols * sizeof(gterm_char);
    grid = static_cast<gterm_char*>(alloc_mem(grid_size));

    queue_size = rows * cols * sizeof(gterm_queue_item);
    queue = static_cast<gterm_queue_item*>(alloc_mem(queue_size));
    queue_i = 0;

    map_size = rows * cols * sizeof(gterm_queue_item*);
    map = static_cast<gterm_queue_item**>(alloc_mem(map_size));

    bg_canvas_size = framebuffer.width * framebuffer.height * sizeof(uint32_t);
    bg_canvas = static_cast<uint32_t*>(alloc_mem(bg_canvas_size));

    generate_canvas();
    clear(true);
    double_buffer_flush();

    return true;
}

void gterm_t::deinit()
{
    free_mem(vga_font_bits, VGA_FONT_MAX);
    free_mem(vga_font_bool, vga_font_bool_size);
    free_mem(grid, grid_size);
    free_mem(queue, queue_size);
    free_mem(map, map_size);
    free_mem(bg_canvas, bg_canvas_size);
}

uint64_t gterm_t::context_size()
{
    uint64_t ret = 0;

    ret += sizeof(gterm_context);
    ret += grid_size;

    return ret;
}

void gterm_t::context_save(uint64_t ptr)
{
    memcpy /* 32to64? */ (reinterpret_cast<void*>(ptr), &context, sizeof(gterm_context));
    ptr += sizeof(gterm_context);

    memcpy /* 32to64? */ (reinterpret_cast<void*>(ptr), grid, grid_size);
}

void gterm_t::context_restore(uint64_t ptr)
{
    memcpy /* 32to64? */ (&context, reinterpret_cast<void*>(ptr), sizeof(gterm_context));
    ptr += sizeof(gterm_context);

    memcpy /* 32to64? */ (grid, reinterpret_cast<void*>(ptr), grid_size);

    for (size_t i = 0; i < static_cast<size_t>(rows) * cols; i++)
    {
        size_t x = i % cols;
        size_t y = i / cols;

        plot_char(&grid[i], x, y);
    }

    if (context.cursor_status) draw_cursor();
}

void gterm_t::full_refresh()
{
    generate_canvas();

    for (size_t i = 0; i < static_cast<size_t>(rows) * cols; i++)
    {
        size_t x = i % cols;
        size_t y = i / cols;

        plot_char(&grid[i], x, y);
    }

    if (context.cursor_status) draw_cursor();
}