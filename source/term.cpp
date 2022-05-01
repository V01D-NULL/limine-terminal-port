#include "tterm.hpp"
#include "gterm.hpp"
#include "term.hpp"

void term_t::init(callback_t callback, bool bios)
{
    if (initialised == true) return;

    this->callback = callback;
    this->bios = bios;

    gterm = static_cast<gterm_t*>(alloc_mem(sizeof(gterm_t)));
    tterm = static_cast<tterm_t*>(alloc_mem(sizeof(tterm_t)));

    initialised = true;
}

term_t::term_t(callback_t callback, bool bios)
{
    init(callback, bios);
}

void term_t::deinit()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->deinit();
    return;

    notready();
}

void term_t::reinit()
{
    if (initialised == false) return;

    context.escape_offset = 0;
    context.control_sequence = false;
    context.csi = false;
    context.escape = false;
    context.rrr = false;
    context.discard_next = false;
    context.bold = false;
    context.reverse_video = false;
    context.dec_private = false;
    context.esc_values_i = 0;
    context.saved_cursor_x = 0;
    context.saved_cursor_y = 0;
    context.current_primary = static_cast<size_t>(-1);
    context.insert_mode = false;
    context.scroll_top_margin = 0;
    context.scroll_bottom_margin = rows;
    context.current_charset = 0;
    context.g_select = 0;
    context.charsets[0] = CHARSET_DEFAULT;
    context.charsets[1] = CHARSET_DEC_SPECIAL;
    autoflush = true;
}

void term_t::vbe(framebuffer_t frm, font_t font, style_t style, background_t back)
{
    if (initialised == false) return;
    if (term_backend != VBE) deinit();

    if (!gterm->init(this, frm, font, style, back) && bios)
    {
        textmode();
        return;
    }

    reinit();
    term_backend = VBE;
}

void term_t::textmode()
{
    if (initialised == false) return;

    deinit();
    tterm->init(this, rows, cols);
    reinit();

    term_backend = TEXTMODE;
}

void term_t::notready()
{
    term_backend = NOT_READY;
    cols = 80;
    rows = 24;
}

void term_t::putchar(uint8_t c)
{
    if (initialised == false) return;

    if (context.discard_next || (runtime == true && (c == 0x18 || c == 0x1A)))
    {
        context.discard_next = false;
        context.escape = false;
        context.csi = false;
        context.control_sequence = false;
        context.g_select = 0;
        return;
    }

    if (context.escape == true)
    {
        escape_parse(c);
        return;
    }

    if (context.g_select)
    {
        context.g_select--;
        switch (c)
        {
            case 'B':
                context.charsets[context.g_select] = CHARSET_DEFAULT;
                break;
            case '0':
                context.charsets[context.g_select] = CHARSET_DEC_SPECIAL;
                break;
        }
        context.g_select = 0;
        return;
    }

    size_t x, y;
    get_cursor_pos(x, y);

    switch (c)
    {
        case 0x00:
        case 0x7F:
            return;
        case 0x9B:
            context.csi = true;
            // FALLTHRU
        case '\e':
            context.escape_offset = 0;
            context.escape = true;
            return;
        case '\t':
            if ((x / TERM_TABSIZE + 1) >= cols)
            {
                set_cursor_pos(cols - 1, y);
                return;
            }
            set_cursor_pos((x / TERM_TABSIZE + 1) * TERM_TABSIZE, y);
            return;
        case 0x0b:
        case 0x0c:
        case '\n':
            if (y == context.scroll_bottom_margin - 1)
            {
                scroll();
                set_cursor_pos(0, y);
            }
            else set_cursor_pos(0, y + 1);
            return;
        case '\b':
            set_cursor_pos(x - 1, y);
            return;
        case '\r':
            set_cursor_pos(0, y);
            return;
        case '\a':
            if (callback != nullptr)
            {
                if (arg != 0) callback(arg, TERM_CB_BELL, 0, 0, 0);
                else callback(TERM_CB_BELL, 0, 0, 0, 0);
            }
            return;
        case 14:
            context.current_charset = 1;
            return;
        case 15:
            context.current_charset = 0;
            return;
    }

    if (context.insert_mode == true)
    {
        for (size_t i = cols - 1; ; i--)
        {
            move_character(i + 1, y, i, y);
            if (i == x) break;
        }
    }

    switch (context.charsets[context.current_charset])
    {
        case CHARSET_DEFAULT:
            break;
        case CHARSET_DEC_SPECIAL:
            c = dec_special_to_cp437(c);
            break;
    }

    raw_putchar(c);
}

#if defined(__i386__)
#define TERM_XFER_CHUNK 8192

static uint8_t xfer_buf[TERM_XFER_CHUNK];
#endif

void term_t::write(uint64_t buf, uint64_t count)
{
    if (initialised == false || term_backend == NOT_READY) return;

    switch (count)
    {
        case TERM_CTX_SIZE:
        {
            uint64_t ret = context_size();
            memcpy(reinterpret_cast<void*>(buf), &ret, sizeof(uint64_t));
            return;
        }
        case TERM_CTX_SAVE:
            context_save(buf);
            return;
        case TERM_CTX_RESTORE:
            context_restore(buf);
            return;
        case TERM_FULL_REFRESH:
            full_refresh();
            return;
    }

    bool native = false;
#if defined(__x86_64__)
    native = true;
#endif

    if (!runtime || native)
    {
        const char *s = reinterpret_cast<const char*>(buf);
        for (size_t i = 0; i < count; i++) putchar(s[i]);
    }
    else
    {
#if defined(__i386__)
        while (count != 0)
        {
            uint64_t chunk;
            if (count > TERM_XFER_CHUNK) chunk = TERM_XFER_CHUNK;
            else chunk = count;

            memcpy(xfer_buf, reinterpret_cast<void*>(buf), chunk);

            for (size_t i = 0; i < chunk; i++) putchar(xfer_buf[i]);

            count -= chunk;
            buf += chunk;
        }
#endif
    }

    if (autoflush) double_buffer_flush();
}

void term_t::print(const char *str)
{
    if (str == nullptr) return;
    size_t length = 0;
    while(str[length]) length++;

    this->write(reinterpret_cast<uint64_t>(str), length);
}

void term_t::sgr()
{
    size_t i = 0;

    if (!context.esc_values_i) goto def;

    for (; i < context.esc_values_i; i++)
    {
        size_t offset;

        if (context.esc_values[i] == 0)
        {
def:
            if (context.reverse_video)
            {
                context.reverse_video = false;
                swap_palette();
            }
            context.bold = false;
            context.current_primary = static_cast<size_t>(-1);
            set_text_bg_default();
            set_text_fg_default();
            continue;
        }
        else if (context.esc_values[i] == 1)
        {
            context.bold = true;
            if (context.current_primary != static_cast<size_t>(-1))
            {
                if (!context.reverse_video) set_text_fg_bright(context.current_primary);
                else set_text_bg_bright(context.current_primary);
            }
            continue;
        }
        else if (context.esc_values[i] == 22)
        {
            context.bold = false;
            if (context.current_primary != static_cast<size_t>(-1))
            {
                if (!context.reverse_video) set_text_fg(context.current_primary);
                else set_text_bg(context.current_primary);
            }
            continue;
        }
        else if (context.esc_values[i] >= 30 && context.esc_values[i] <= 37)
        {
            offset = 30;
            context.current_primary = context.esc_values[i] - offset;

            if (context.reverse_video) goto set_bg;
set_fg:
            if (context.bold && !context.reverse_video)
            {
                set_text_fg_bright(context.esc_values[i] - offset);
            }
            else set_text_fg(context.esc_values[i] - offset);
            continue;
        }
        else if (context.esc_values[i] >= 40 && context.esc_values[i] <= 47)
        {
            offset = 40;
            if (context.reverse_video) goto set_fg;
set_bg:
            if (context.bold && context.reverse_video)
            {
                set_text_bg_bright(context.esc_values[i] - offset);
            }
            else set_text_bg(context.esc_values[i] - offset);
            continue;
        }
        else if (context.esc_values[i] >= 90 && context.esc_values[i] <= 97)
        {
            offset = 90;
            context.current_primary = context.esc_values[i] - offset;

            if (context.reverse_video) goto set_bg_bright;
set_fg_bright:
            set_text_fg_bright(context.esc_values[i] - offset);
            continue;
        }
        else if (context.esc_values[i] >= 100 && context.esc_values[i] <= 107) {
            offset = 100;
            if (context.reverse_video) goto set_fg_bright;
set_bg_bright:
            set_text_bg_bright(context.esc_values[i] - offset);
            continue;
        }
        else if (context.esc_values[i] == 39)
        {
            context.current_primary = static_cast<size_t>(-1);

            if (context.reverse_video) swap_palette();
            set_text_fg_default();
            if (context.reverse_video) swap_palette();

            continue;
        }
        else if (context.esc_values[i] == 49)
        {
            if (context.reverse_video) swap_palette();
            set_text_bg_default();
            if (context.reverse_video) swap_palette();

            continue;
        }
        else if (context.esc_values[i] == 7)
        {
            if (!context.reverse_video)
            {
                context.reverse_video = true;
                swap_palette();
            }
            continue;
        }
        else if (context.esc_values[i] == 27)
        {
            if (context.reverse_video)
            {
                context.reverse_video = false;
                swap_palette();
            }
            continue;
        }
    }
}

void term_t::dec_private_parse(uint8_t c)
{
    context.dec_private = false;

    if (context.esc_values_i == 0) return;

    bool set;
    switch (c)
    {
        case 'h':
            set = true;
            break;
        case 'l':
            set = false;
            break;
        default:
            return;
    }

    switch (context.esc_values[0])
    {
        case 25:
            if (set) enable_cursor();
            else disable_cursor();
            return;
    }

    if (callback != nullptr)
    {
        if (arg != 0) callback(arg, TERM_CB_DEC, context.esc_values_i, reinterpret_cast<uintptr_t>(context.esc_values), c);
        else callback(TERM_CB_DEC, context.esc_values_i, reinterpret_cast<uintptr_t>(context.esc_values), c, 0);
    }
}

void term_t::linux_private_parse()
{
    if (context.esc_values_i == 0) return;

    if (callback != nullptr)
    {
        if (arg != 0) callback(arg, TERM_CB_LINUX, context.esc_values_i, reinterpret_cast<uintptr_t>(context.esc_values), 0);
        else callback(TERM_CB_LINUX, context.esc_values_i, reinterpret_cast<uintptr_t>(context.esc_values), 0, 0);
    }
}

void term_t::mode_toggle(uint8_t c)
{
    if (context.esc_values_i == 0) return;

    bool set;
    switch (c)
    {
        case 'h':
            set = true;
            break;
        case 'l':
            set = false;
            break;
        default:
            return;
    }

    switch (context.esc_values[0])
    {
        case 4:
            context.insert_mode = set;
            return;
    }

    if (callback != nullptr)
    {
        if (arg != 0) callback(arg, TERM_CB_MODE, context.esc_values_i, reinterpret_cast<uintptr_t>(context.esc_values), c);
        else callback(TERM_CB_MODE, context.esc_values_i, reinterpret_cast<uintptr_t>(context.esc_values), c, 0);
    }
}

void term_t::control_sequence_parse(uint8_t c)
{
    if (context.escape_offset == 2)
    {
        switch (c)
        {
            case '[':
                context.discard_next = true;
                goto cleanup;
            case '?':
                context.dec_private = true;
                return;
        }
    }

    if (c >= '0' && c <= '9')
    {
        if (context.esc_values_i == MAX_ESC_VALUES) return;
        context.rrr = true;
        context.esc_values[context.esc_values_i] *= 10;
        context.esc_values[context.esc_values_i] += c - '0';
        return;
    }

    if (context.rrr == true)
    {
        context.esc_values_i++;
        context.rrr = false;
        if (c == ';') return;
    }
    else if (c == ';')
    {
        if (context.esc_values_i == MAX_ESC_VALUES) return;
        context.esc_values[context.esc_values_i] = 0;
        context.esc_values_i++;
        return;
    }

    size_t esc_default;
    switch (c)
    {
        case 'J':
        case 'K':
        case 'q':
            esc_default = 0;
            break;
        default:
            esc_default = 1;
            break;
    }

    for (size_t i = context.esc_values_i; i < MAX_ESC_VALUES; i++)
    {
        context.esc_values[i] = esc_default;
    }

    if (context.dec_private == true)
    {
        dec_private_parse(c);
        goto cleanup;
    }

    bool r;
    r = scroll_disable();
    size_t x, y;
    get_cursor_pos(x, y);

    switch (c) {
        case 'F':
            x = 0;
            // FALLTHRU
        case 'A':
        {
            if (context.esc_values[0] > y) context.esc_values[0] = y;
            size_t orig_y = y;
            size_t dest_y = y - context.esc_values[0];
            bool will_be_in_scroll_region = false;
            if ((context.scroll_top_margin >= dest_y && context.scroll_top_margin <= orig_y) || (context.scroll_bottom_margin >= dest_y && context.scroll_bottom_margin <= orig_y))
            {
                will_be_in_scroll_region = true;
            }
            if (will_be_in_scroll_region && dest_y < context.scroll_top_margin)
            {
                dest_y = context.scroll_top_margin;
            }
            set_cursor_pos(x, dest_y);
            break;
        }
        case 'E':
            x = 0;
            // FALLTHRU
        case 'e':
        case 'B':
        {
            if (y + context.esc_values[0] > rows - 1) context.esc_values[0] = (rows - 1) - y;
            size_t orig_y = y;
            size_t dest_y = y + context.esc_values[0];
            bool will_be_in_scroll_region = false;
            if ((context.scroll_top_margin >= orig_y && context.scroll_top_margin <= dest_y) || (context.scroll_bottom_margin >= orig_y && context.scroll_bottom_margin <= dest_y))
            {
                will_be_in_scroll_region = true;
            }
            if (will_be_in_scroll_region && dest_y >= context.scroll_bottom_margin)
            {
                dest_y = context.scroll_bottom_margin - 1;
            }
            set_cursor_pos(x, dest_y);
            break;
        }
        case 'a':
        case 'C':
            if (x + context.esc_values[0] > cols - 1) context.esc_values[0] = (cols - 1) - x;
            set_cursor_pos(x + context.esc_values[0], y);
            break;
        case 'D':
            if (context.esc_values[0] > x) context.esc_values[0] = x;
            set_cursor_pos(x - context.esc_values[0], y);
            break;
        case 'c':
            if (callback != nullptr)
            {
                if (arg != 0) callback(arg, TERM_CB_PRIVATE_ID, 0, 0, 0);
                else callback(TERM_CB_PRIVATE_ID, 0, 0, 0, 0);
            }
            break;
        case 'd':
            context.esc_values[0] -= 1;
            if (context.esc_values[0] >= rows) context.esc_values[0] = rows - 1;
            set_cursor_pos(x, context.esc_values[0]);
            break;
        case 'G':
        case '`':
            context.esc_values[0] -= 1;
            if (context.esc_values[0] >= cols) context.esc_values[0] = cols - 1;
            set_cursor_pos(context.esc_values[0], y);
            break;
        case 'H':
        case 'f':
            context.esc_values[0] -= 1;
            context.esc_values[1] -= 1;
            if (context.esc_values[1] >= cols) context.esc_values[1] = cols - 1;
            if (context.esc_values[0] >= rows) context.esc_values[0] = rows - 1;
            set_cursor_pos(context.esc_values[1], context.esc_values[0]);
            break;
        case 'n':
            switch (context.esc_values[0])
            {
                case 5:
                    if (callback != nullptr)
                    {
                        if (arg != 0) callback(arg, TERM_CB_STATUS_REPORT, 0, 0, 0);
                        else callback(TERM_CB_STATUS_REPORT, 0, 0, 0, 0);
                    }
                    break;
                case 6:
                    if (callback != nullptr)
                    {
                        if (arg != 0) callback(arg, TERM_CB_POS_REPORT, x + 1, y + 1, 0);
                        else callback(TERM_CB_POS_REPORT, x + 1, y + 1, 0, 0);
                    }
                    break;
            }
            break;
        case 'q':
            if (callback != nullptr)
            {
                if (arg != 0) callback(arg, TERM_CB_KBD_LEDS, context.esc_values[0], 0, 0);
                else callback(TERM_CB_KBD_LEDS, context.esc_values[0], 0, 0, 0);
            }
            break;
        case 'J':
            switch (context.esc_values[0])
            {
                case 0:
                {
                    size_t rows_remaining = rows - (y + 1);
                    size_t cols_diff = cols - (x + 1);
                    size_t to_clear = rows_remaining * cols + cols_diff;
                    for (size_t i = 0; i < to_clear; i++) raw_putchar(' ');
                    set_cursor_pos(x, y);
                    break;
                }
                case 1:
                {
                    set_cursor_pos(0, 0);
                    bool b = false;
                    for (size_t yc = 0; yc < rows; yc++)
                    {
                        for (size_t xc = 0; xc < cols; xc++)
                        {
                            raw_putchar(' ');
                            if (xc == x && yc == y)
                            {
                                set_cursor_pos(x, y);
                                b = true;
                                break;
                            }
                        }
                        if (b == true) break;
                    }
                    break;
                }
                case 2:
                case 3:
                    clear(false);
                    break;
            }
            break;
        case '@':
            for (size_t i = cols - 1; ; i--)
            {
                move_character(i + context.esc_values[0], y, i, y);
                set_cursor_pos(i, y);
                raw_putchar(' ');
                if (i == x) break;
            }
            set_cursor_pos(x, y);
            break;
        case 'P':
            for (size_t i = x + context.esc_values[0]; i < cols; i++)
            {
                move_character(i - context.esc_values[0], y, i, y);
            }
            set_cursor_pos(cols - context.esc_values[0], y);
            // FALLTHRU
        case 'X':
            for (size_t i = 0; i < context.esc_values[0]; i++) raw_putchar(' ');
            set_cursor_pos(x, y);
            break;
        case 'm':
            sgr();
            break;
        case 's':
            get_cursor_pos(context.saved_cursor_x, context.saved_cursor_y);
            break;
        case 'u':
            set_cursor_pos(context.saved_cursor_x, context.saved_cursor_y);
            break;
        case 'K':
            switch (context.esc_values[0])
            {
                case 0:
                    for (size_t i = x; i < cols; i++) raw_putchar(' ');
                    set_cursor_pos(x, y);
                    break;
                case 1:
                    set_cursor_pos(0, y);
                    for (size_t i = 0; i < x; i++) raw_putchar(' ');
                    break;
                case 2:
                    set_cursor_pos(0, y);
                    for (size_t i = 0; i < cols; i++) raw_putchar(' ');
                    set_cursor_pos(x, y);
                    break;
            }
            break;
        case 'r':
            context.scroll_top_margin = 0;
            context.scroll_bottom_margin = rows;
            if (context.esc_values_i > 0)
            {
                context.scroll_top_margin = context.esc_values[0] - 1;
            }
            if (context.esc_values_i > 1)
            {
                context.scroll_bottom_margin = context.esc_values[1];
            }
            if (context.scroll_top_margin >= rows || context.scroll_bottom_margin > rows || context.scroll_top_margin >= (context.scroll_bottom_margin - 1))
            {
                context.scroll_top_margin = 0;
                context.scroll_bottom_margin = rows;
            }
            set_cursor_pos(0, 0);
            break;
        case 'l':
        case 'h':
            mode_toggle(c);
            break;
        case ']':
            linux_private_parse();
            break;
    }

    if (r) scroll_enable();

cleanup:
    context.control_sequence = false;
    context.escape = false;
}

void term_t::escape_parse(uint8_t c)
{
    context.escape_offset++;

    if (context.control_sequence == true)
    {
        control_sequence_parse(c);
        return;
    }

    if (context.csi == true)
    {
        context.csi = false;
        goto is_csi;
    }

    size_t x, y;
    get_cursor_pos(x, y);

    switch (c)
    {
        case '[':
is_csi:
            for (size_t i = 0; i < MAX_ESC_VALUES; i++) context.esc_values[i] = 0;
            context.esc_values_i = 0;
            context.rrr = false;
            context.control_sequence = true;
            return;
        case '7':
            save_state();
            break;
        case '8':
            restore_state();
            break;
        case 'c':
            reinit();
            clear(true);
            break;
        case 'D':
            if (y == context.scroll_bottom_margin - 1)
            {
                scroll();
                set_cursor_pos(x, y);
            }
            else set_cursor_pos(x, y + 1);
            break;
        case 'E':
            if (y == context.scroll_bottom_margin - 1)
            {
                scroll();
                set_cursor_pos(0, y);
            }
            else set_cursor_pos(0, y + 1);
            break;
        case 'M':
            if (y == context.scroll_top_margin)
            {
                revscroll();
                set_cursor_pos(0, y);
            }
            else set_cursor_pos(0, y - 1);
            break;
        case 'Z':
            if (callback != nullptr)
            {
                if (arg != 0) callback(arg, TERM_CB_PRIVATE_ID, 0, 0, 0);
                else callback(TERM_CB_PRIVATE_ID, 0, 0, 0, 0);
            }
            break;
        case '(':
        case ')':
            context.g_select = c - '\'';
            break;
        case '\e':
            if (runtime == false) raw_putchar(c);
            break;
    }

    context.escape = false;
}

uint8_t term_t::dec_special_to_cp437(uint8_t c)
{
    switch (c)
    {
        case '`': return 0x04;
        case '0': return 0xDB;
        case '-': return 0x18;
        case ',': return 0x1B;
        case '.': return 0x19;
        case 'a': return 0xB1;
        case 'f': return 0xF8;
        case 'g': return 0xF1;
        case 'h': return 0xB0;
        case 'j': return 0xD9;
        case 'k': return 0xBf;
        case 'l': return 0xDa;
        case 'm': return 0xC0;
        case 'n': return 0xC5;
        case 'q': return 0xC4;
        case 's': return 0x5F;
        case 't': return 0xC3;
        case 'u': return 0xB4;
        case 'v': return 0xC1;
        case 'w': return 0xC2;
        case 'x': return 0xB3;
        case 'y': return 0xF3;
        case 'z': return 0xF2;
        case '~': return 0xFA;
        case '_': return 0xFF;
        case '+': return 0x1A;
        case '{': return 0xE3;
        case '}': return 0x9C;
    }

    return c;
}

void term_t::raw_putchar(uint8_t c)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->putchar(c);
    else if (term_backend == TEXTMODE && tterm) return tterm->putchar(c);
}
void term_t::clear(bool move)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->clear(move);
    else if (term_backend == TEXTMODE && tterm) return tterm->clear(move);
}
void term_t::enable_cursor()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->enable_cursor();
    else if (term_backend == TEXTMODE && tterm) return tterm->enable_cursor();
}
bool term_t::disable_cursor()
{
    if (initialised == false) return false;

    if (term_backend == VBE && gterm) return gterm->disable_cursor();
    else if (term_backend == TEXTMODE && tterm) return tterm->disable_cursor();
    return false;
}
void term_t::set_cursor_pos(size_t x, size_t y)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_cursor_pos(x, y);
    else if (term_backend == TEXTMODE && tterm) return tterm->set_cursor_pos(x, y);
}
void term_t::get_cursor_pos(size_t &x, size_t &y)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->get_cursor_pos(x, y);
    else if (term_backend == TEXTMODE && tterm) return tterm->get_cursor_pos(x, y);
}
void term_t::set_text_fg(size_t fg)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_text_fg(fg);
    else if (term_backend == TEXTMODE && tterm) return tterm->set_text_fg(fg);
}
void term_t::set_text_bg(size_t bg)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_text_bg(bg);
    else if (term_backend == TEXTMODE && tterm) return tterm->set_text_bg(bg);
}
void term_t::set_text_fg_bright(size_t fg)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_text_fg_bright(fg);
    else if (term_backend == TEXTMODE && tterm) return tterm->set_text_fg_bright(fg);
}
void term_t::set_text_bg_bright(size_t bg)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_text_bg_bright(bg);
    else if (term_backend == TEXTMODE && tterm) return tterm->set_text_bg_bright(bg);
}
void term_t::set_text_fg_default()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_text_fg_default();
    else if (term_backend == TEXTMODE && tterm) return tterm->set_text_fg_default();
}
void term_t::set_text_bg_default()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->set_text_bg_default();
    else if (term_backend == TEXTMODE && tterm) return tterm->set_text_bg_default();
}
bool term_t::scroll_disable()
{
    if (initialised == false) return false;

    if (term_backend == VBE && gterm) return gterm->scroll_disable();
    else if (term_backend == TEXTMODE && tterm) return tterm->scroll_disable();
    return false;
}
void term_t::scroll_enable()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->scroll_enable();
    else if (term_backend == TEXTMODE && tterm) return tterm->scroll_enable();
}
void term_t::move_character(size_t new_x, size_t new_y, size_t old_x, size_t old_y)
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->move_character(new_x, new_y, old_x, old_y);
    else if (term_backend == TEXTMODE && tterm) return tterm->move_character(new_x, new_y, old_x, old_y);
}
void term_t::scroll()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->scroll();
    else if (term_backend == TEXTMODE && tterm) return tterm->scroll();
}
void term_t::revscroll()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->revscroll();
    else if (term_backend == TEXTMODE && tterm) return tterm->revscroll();
}
void term_t::swap_palette()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->swap_palette();
    else if (term_backend == TEXTMODE && tterm) return tterm->swap_palette();
}
void term_t::save_state()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->save_state();
    else if (term_backend == TEXTMODE && tterm) return tterm->save_state();

    context.saved_state_bold = context.bold;
    context.saved_state_reverse_video = context.reverse_video;
    context.saved_state_current_charset = context.current_charset;
    context.saved_state_current_primary = context.current_primary;
}
void term_t::restore_state()
{
    if (initialised == false) return;

    context.bold = context.saved_state_bold;
    context.reverse_video = context.saved_state_reverse_video;
    context.current_charset = context.saved_state_current_charset;
    context.current_primary = context.saved_state_current_primary;

    if (term_backend == VBE && gterm) return gterm->restore_state();
    else if (term_backend == TEXTMODE && tterm) return tterm->restore_state();
}

void term_t::double_buffer_flush()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->double_buffer_flush();
    else if (term_backend == TEXTMODE && tterm) return tterm->double_buffer_flush();
}

uint64_t term_t::context_size()
{
    if (initialised == false) return 0;

    uint64_t ret = sizeof(term_context);

    if (term_backend == VBE && gterm) ret += gterm->context_size();
    else if (term_backend == TEXTMODE && tterm) ret += tterm->context_size();

    return ret;
}
void term_t::context_save(uint64_t ptr)
{
    if (initialised == false) return;

    memcpy(reinterpret_cast<void*>(ptr), &context, sizeof(term_context));
    ptr += sizeof(term_context);

    if (term_backend == VBE && gterm) return gterm->context_save(ptr);
    else if (term_backend == TEXTMODE && tterm) return tterm->context_save(ptr);
}
void term_t::context_restore(uint64_t ptr)
{
    if (initialised == false) return;

    memcpy(&context, reinterpret_cast<void*>(ptr), sizeof(term_context));
    ptr += sizeof(term_context);

    if (term_backend == VBE && gterm) return gterm->context_restore(ptr);
    else if (term_backend == TEXTMODE && tterm) return tterm->context_restore(ptr);
}
void term_t::full_refresh()
{
    if (initialised == false) return;

    if (term_backend == VBE && gterm) return gterm->full_refresh();
    else if (term_backend == TEXTMODE && tterm) return tterm->full_refresh();
}