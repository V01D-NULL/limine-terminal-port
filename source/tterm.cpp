#include "tterm.hpp"
#include "term.hpp"

static void outb(uint16_t port, uint8_t val)
{
    asm volatile (".att_syntax prefix\n\toutb %0, %1" : : "a"(val), "Nd"(port));
}

void tterm_t::init(term_t *term, size_t &rows, size_t &cols)
{
    this->term = term;

    if (back_buffer == nullptr) back_buffer = static_cast<uint8_t*>(alloc_mem(VD_ROWS * VD_COLS));
    else memset(back_buffer, 0, VD_ROWS * VD_COLS);
    if (front_buffer == nullptr) front_buffer = static_cast<uint8_t*>(alloc_mem(VD_ROWS * VD_COLS));
    else memset(front_buffer, 0, VD_ROWS * VD_COLS);

    context.cursor_offset = 0;
    context.cursor_status = true;
    context.text_palette = 0x07;
    context.scroll_enabled = true;

    clear(false);

    term->rows = VD_ROWS;
    term->cols = VD_COLS / 2;

    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);

    double_buffer_flush();
}

void tterm_t::putchar(uint8_t c)
{
    back_buffer[context.cursor_offset] = c;
    back_buffer[context.cursor_offset + 1] = context.text_palette;
    if (context.cursor_offset / VD_COLS == term->context.scroll_bottom_margin - 1 && context.cursor_offset % VD_COLS == VD_COLS - 2)
    {
        if (context.scroll_enabled)
        {
            scroll();
            context.cursor_offset -= context.cursor_offset % VD_COLS;
        }
    }
    else if (context.cursor_offset >= (VIDEO_BOTTOM - 1))
    {
        context.cursor_offset -= context.cursor_offset % VD_COLS;
    }
    else context.cursor_offset += 2;
}

void tterm_t::clear(bool move)
{
    for (size_t i = 0; i < VIDEO_BOTTOM; i += 2)
    {
        back_buffer[i] = ' ';
        back_buffer[i + 1] = context.text_palette;
    }
    if (move) context.cursor_offset = 0;
}

void tterm_t::draw_cursor()
{
    uint8_t pal = back_buffer[context.cursor_offset + 1];
    video_mem[context.cursor_offset + 1] = ((pal & 0xF0) >> 4) | ((pal & 0x0F) << 4);
}

void tterm_t::enable_cursor()
{
    context.cursor_status = true;
}

bool tterm_t::disable_cursor()
{
    bool ret = context.cursor_status;
    context.cursor_status = false;
    return ret;
}

void tterm_t::set_cursor_pos(size_t x, size_t y)
{
    if (x >= VD_COLS / 2)
    {
        if (static_cast<int>(x) < 0) x = 0;
        else x = VD_COLS / 2 - 1;
    }
    if (y >= VD_ROWS)
    {
        if (static_cast<int>(y) < 0) y = 0;
        else y = VD_ROWS - 1;
    }
    context.cursor_offset = y * VD_COLS + x * 2;
}

void tterm_t::get_cursor_pos(size_t &x, size_t &y)
{
    x = (context.cursor_offset % VD_COLS) / 2;
    y = context.cursor_offset / VD_COLS;
}

static uint8_t ansi_colours[] = { 0, 4, 2, 6, 1, 5, 3, 7 };
void tterm_t::set_text_fg(size_t fg)
{
    context.text_palette = (context.text_palette & 0xF0) | ansi_colours[fg];
}

void tterm_t::set_text_bg(size_t bg)
{
    context.text_palette = (context.text_palette & 0x0F) | (ansi_colours[bg] << 4);
}

void tterm_t::set_text_fg_bright(size_t fg)
{
    context.text_palette = (context.text_palette & 0xF0) | (ansi_colours[fg] | (1 << 3));
}

void tterm_t::set_text_bg_bright(size_t bg)
{
    context.text_palette = (context.text_palette & 0x0F) | ((ansi_colours[bg] | (1 << 3)) << 4);
}

void tterm_t::set_text_fg_default()
{
    context.text_palette = (context.text_palette & 0xF0) | 7;
}

void tterm_t::set_text_bg_default()
{
    context.text_palette &= 0x0F;
}

bool tterm_t::scroll_disable()
{
    bool ret = context.scroll_enabled;
    context.scroll_enabled = false;
    return ret;
}

void tterm_t::scroll_enable()
{
    context.scroll_enabled = true;
}

void tterm_t::move_character(size_t new_x, size_t new_y, size_t old_x, size_t old_y)
{
    if (old_x >= VD_COLS / 2 || old_y >= VD_ROWS || new_x >= VD_COLS / 2 || new_y >= VD_ROWS) return;
    back_buffer[new_y * VD_COLS + new_x * 2] = back_buffer[old_y * VD_COLS + old_x * 2];
}

void tterm_t::scroll()
{
    for (size_t i = term->context.scroll_top_margin * VD_COLS; i < (term->context.scroll_bottom_margin - 1) * VD_COLS; i++)
    {
        back_buffer[i] = back_buffer[i + VD_COLS];
    }

    for (size_t i = (term->context.scroll_bottom_margin - 1) * VD_COLS; i < term->context.scroll_bottom_margin * VD_COLS; i += 2)
    {
        back_buffer[i] = ' ';
        back_buffer[i + 1] = context.text_palette;
    }
}

void tterm_t::revscroll()
{
    for (size_t i = (term->context.scroll_bottom_margin - 1) * VD_COLS - 2; ; i--)
    {
        back_buffer[i + VD_COLS] = back_buffer[i];
        if (i == term->context.scroll_top_margin * VD_COLS) break;
    }

    for (size_t i = term->context.scroll_top_margin * VD_COLS; i < (term->context.scroll_top_margin + 1) * VD_COLS; i += 2)
    {
        back_buffer[i] = ' ';
        back_buffer[i + 1] = context.text_palette;
    }
}

void tterm_t::swap_palette()
{
    context.text_palette = (context.text_palette << 4) | (context.text_palette >> 4);
}

void tterm_t::save_state()
{
    context.saved_state_text_palette = context.text_palette;
    context.saved_state_cursor_offset = context.cursor_offset;
}

void tterm_t::restore_state()
{
    context.text_palette = context.saved_state_text_palette;
    context.cursor_offset = context.saved_state_cursor_offset;
}

void tterm_t::double_buffer_flush()
{
    if (context.cursor_status) draw_cursor();

    if (context.cursor_offset != old_cursor_offset || context.cursor_status == false)
    {
        video_mem[old_cursor_offset + 1] = back_buffer[old_cursor_offset + 1];
    }

    for (size_t i = 0; i < VD_ROWS * VD_COLS; i++)
    {
        if (back_buffer[i] == front_buffer[i]) continue;

        if (context.cursor_status && i == context.cursor_offset + 1) continue;

        front_buffer[i] = back_buffer[i];
        video_mem[i] = back_buffer[i];
    }

    if (context.cursor_status) old_cursor_offset = context.cursor_offset;
}

uint64_t tterm_t::context_size()
{
    uint64_t ret = 0;

    ret += sizeof(tterm_context);
    ret += VD_ROWS * VD_COLS;

    return ret;
}

void tterm_t::context_save(uint64_t ptr)
{
    memcpy(reinterpret_cast<void*>(ptr), &context, sizeof(tterm_context));
    ptr += sizeof(tterm_context);

    memcpy(reinterpret_cast<void*>(ptr), front_buffer, VD_ROWS * VD_COLS);
}

void tterm_t::context_restore(uint64_t ptr)
{
    memcpy(&context, reinterpret_cast<void*>(ptr), sizeof(tterm_context));
    ptr += sizeof(tterm_context);

    memcpy(front_buffer, reinterpret_cast<void*>(ptr), VD_ROWS * VD_COLS);

    for (size_t i = 0; i < VD_ROWS * VD_COLS; i++)
    {
        video_mem[i] = front_buffer[i];
        back_buffer[i] = front_buffer[i];
    }

    if (context.cursor_status)
    {
        draw_cursor();
        old_cursor_offset = context.cursor_offset;
    }
}

void tterm_t::full_refresh()
{
    for (size_t i = 0; i < VD_ROWS * VD_COLS; i++)
    {
        video_mem[i] = front_buffer[i];
        back_buffer[i] = front_buffer[i];
    }

    if (context.cursor_status)
    {
        draw_cursor();
        old_cursor_offset = context.cursor_offset;
    }
}
