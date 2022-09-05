#include "tterm.h"
#include "gterm.h"
#include "term.h"

static const uint32_t col256[] = {
    0x000000, 0x00005f, 0x000087, 0x0000af, 0x0000d7, 0x0000ff, 0x005f00, 0x005f5f,
    0x005f87, 0x005faf, 0x005fd7, 0x005fff, 0x008700, 0x00875f, 0x008787, 0x0087af,
    0x0087d7, 0x0087ff, 0x00af00, 0x00af5f, 0x00af87, 0x00afaf, 0x00afd7, 0x00afff,
    0x00d700, 0x00d75f, 0x00d787, 0x00d7af, 0x00d7d7, 0x00d7ff, 0x00ff00, 0x00ff5f,
    0x00ff87, 0x00ffaf, 0x00ffd7, 0x00ffff, 0x5f0000, 0x5f005f, 0x5f0087, 0x5f00af,
    0x5f00d7, 0x5f00ff, 0x5f5f00, 0x5f5f5f, 0x5f5f87, 0x5f5faf, 0x5f5fd7, 0x5f5fff,
    0x5f8700, 0x5f875f, 0x5f8787, 0x5f87af, 0x5f87d7, 0x5f87ff, 0x5faf00, 0x5faf5f,
    0x5faf87, 0x5fafaf, 0x5fafd7, 0x5fafff, 0x5fd700, 0x5fd75f, 0x5fd787, 0x5fd7af,
    0x5fd7d7, 0x5fd7ff, 0x5fff00, 0x5fff5f, 0x5fff87, 0x5fffaf, 0x5fffd7, 0x5fffff,
    0x870000, 0x87005f, 0x870087, 0x8700af, 0x8700d7, 0x8700ff, 0x875f00, 0x875f5f,
    0x875f87, 0x875faf, 0x875fd7, 0x875fff, 0x878700, 0x87875f, 0x878787, 0x8787af,
    0x8787d7, 0x8787ff, 0x87af00, 0x87af5f, 0x87af87, 0x87afaf, 0x87afd7, 0x87afff,
    0x87d700, 0x87d75f, 0x87d787, 0x87d7af, 0x87d7d7, 0x87d7ff, 0x87ff00, 0x87ff5f,
    0x87ff87, 0x87ffaf, 0x87ffd7, 0x87ffff, 0xaf0000, 0xaf005f, 0xaf0087, 0xaf00af,
    0xaf00d7, 0xaf00ff, 0xaf5f00, 0xaf5f5f, 0xaf5f87, 0xaf5faf, 0xaf5fd7, 0xaf5fff,
    0xaf8700, 0xaf875f, 0xaf8787, 0xaf87af, 0xaf87d7, 0xaf87ff, 0xafaf00, 0xafaf5f,
    0xafaf87, 0xafafaf, 0xafafd7, 0xafafff, 0xafd700, 0xafd75f, 0xafd787, 0xafd7af,
    0xafd7d7, 0xafd7ff, 0xafff00, 0xafff5f, 0xafff87, 0xafffaf, 0xafffd7, 0xafffff,
    0xd70000, 0xd7005f, 0xd70087, 0xd700af, 0xd700d7, 0xd700ff, 0xd75f00, 0xd75f5f,
    0xd75f87, 0xd75faf, 0xd75fd7, 0xd75fff, 0xd78700, 0xd7875f, 0xd78787, 0xd787af,
    0xd787d7, 0xd787ff, 0xd7af00, 0xd7af5f, 0xd7af87, 0xd7afaf, 0xd7afd7, 0xd7afff,
    0xd7d700, 0xd7d75f, 0xd7d787, 0xd7d7af, 0xd7d7d7, 0xd7d7ff, 0xd7ff00, 0xd7ff5f,
    0xd7ff87, 0xd7ffaf, 0xd7ffd7, 0xd7ffff, 0xff0000, 0xff005f, 0xff0087, 0xff00af,
    0xff00d7, 0xff00ff, 0xff5f00, 0xff5f5f, 0xff5f87, 0xff5faf, 0xff5fd7, 0xff5fff,
    0xff8700, 0xff875f, 0xff8787, 0xff87af, 0xff87d7, 0xff87ff, 0xffaf00, 0xffaf5f,
    0xffaf87, 0xffafaf, 0xffafd7, 0xffafff, 0xffd700, 0xffd75f, 0xffd787, 0xffd7af,
    0xffd7d7, 0xffd7ff, 0xffff00, 0xffff5f, 0xffff87, 0xffffaf, 0xffffd7, 0xffffff,
    0x080808, 0x121212, 0x1c1c1c, 0x262626, 0x303030, 0x3a3a3a, 0x444444, 0x4e4e4e,
    0x585858, 0x626262, 0x6c6c6c, 0x767676, 0x808080, 0x8a8a8a, 0x949494, 0x9e9e9e,
    0xa8a8a8, 0xb2b2b2, 0xbcbcbc, 0xc6c6c6, 0xd0d0d0, 0xdadada, 0xe4e4e4, 0xeeeeee
};

void term_init(struct term_t *term, callback_t callback, bool bios)
{
    if (term->initialised == true)
        return;

    term->callback = callback;
    term->bios = bios;
    term->term_backend = NOT_READY;
    term->arg = (uint64_t)term;

    term->gterm = alloc_mem(sizeof(struct gterm_t));
#if defined(__i386__) || defined(__x86_64__)
    if (bios == true)
        term->tterm = alloc_mem(sizeof(struct tterm_t));
#endif

    term->initialised = true;
}

void term_deinit(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_deinit(term->gterm);
    term_notready(term);
}

void term_reinit(struct term_t *term)
{
    if (term->initialised == false)
        return;

    term->context.escape_offset = 0;
    term->context.control_sequence = false;
    term->context.csi = false;
    term->context.escape = false;
    term->context.rrr = false;
    term->context.discard_next = false;
    term->context.bold = false;
    term->context.reverse_video = false;
    term->context.dec_private = false;
    term->context.esc_values_i = 0;
    term->context.saved_cursor_x = 0;
    term->context.saved_cursor_y = 0;
    term->context.current_primary = (size_t)(-1);
    term->context.insert_mode = false;
    term->context.scroll_top_margin = 0;
    term->context.scroll_bottom_margin = term->rows;
    term->context.current_charset = 0;
    term->context.g_select = 0;
    term->context.charsets[0] = CHARSET_DEFAULT;
    term->context.charsets[1] = CHARSET_DEC_SPECIAL;
    term->autoflush = true;
}

void term_vbe(struct term_t *term, struct framebuffer_t frm, struct font_t font, struct style_t style, struct background_t back)
{
    if (term->initialised == false)
        return;

    if (term->term_backend != VBE)
        term_deinit(term);

    if (!gterm_init(term->gterm, term, frm, font, style, back) && term->bios)
    {
#if defined(__i386__) || defined(__x86_64__)
        term_textmode(term);
#endif
        return;
    }

    term_reinit(term);
    term->term_backend = VBE;
}

#if defined(__i386__) || defined(__x86_64__)
void term_textmode(struct term_t *term)
{
    if (term->initialised == false || !term->tterm)
        return;

    term_deinit(term);
    tterm_init(term->tterm, term);
    term_reinit(term);

    term->term_backend = TEXTMODE;
}
#endif

void term_notready(struct term_t *term)
{
    term->term_backend = NOT_READY;
    term->cols = 80;
    term->rows = 24;
}

uint8_t term_dec_special_to_cp437(uint8_t c)
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

void term_putchar(struct term_t *term, uint8_t c)
{
    if (term->initialised == false)
        return;

    if (term->context.discard_next || (term->runtime == true && (c == 0x18 || c == 0x1A)))
    {
        term->context.discard_next = false;
        term->context.escape = false;
        term->context.csi = false;
        term->context.control_sequence = false;
        term->context.g_select = 0;
        return;
    }

    if (term->context.escape == true)
    {
        term_escape_parse(term, c);
        return;
    }

    if (term->context.g_select)
    {
        term->context.g_select--;
        switch (c)
        {
            case 'B':
                term->context.charsets[term->context.g_select] = CHARSET_DEFAULT;
                break;
            case '0':
                term->context.charsets[term->context.g_select] = CHARSET_DEC_SPECIAL;
                break;
        }
        term->context.g_select = 0;
        return;
    }

    size_t x, y;
    term_get_cursor_pos(term, &x, &y);

    switch (c)
    {
        case 0x00:
        case 0x7F:
            return;
        case 0x9B:
            term->context.csi = true;
            // FALLTHRU
        case '\e':
            term->context.escape_offset = 0;
            term->context.escape = true;
            return;
        case '\t':
            if ((x / TERM_TABSIZE + 1) >= term->cols)
            {
                term_set_cursor_pos(term, term->cols - 1, y);
                return;
            }
            term_set_cursor_pos(term, (x / TERM_TABSIZE + 1) * TERM_TABSIZE, y);
            return;
        case 0x0b:
        case 0x0c:
        case '\n':
            if (y == term->context.scroll_bottom_margin - 1)
            {
                term_scroll(term);
                term_set_cursor_pos(term, 0, y);
            }
            else term_set_cursor_pos(term, 0, y + 1);
            return;
        case '\b':
            term_set_cursor_pos(term, x - 1, y);
            return;
        case '\r':
            term_set_cursor_pos(term, 0, y);
            return;
        case '\a':
            if (term->callback)
            {
                if (term->arg != 0)
                    term->callback(term->arg, TERM_CB_BELL, 0, 0, 0);
                else
                    term->callback(TERM_CB_BELL, 0, 0, 0, 0);
            }
            return;
        case 14:
            term->context.current_charset = 1;
            return;
        case 15:
            term->context.current_charset = 0;
            return;
    }

    if (term->context.insert_mode == true)
    {
        for (size_t i = term->cols - 1; ; i--)
        {
            term_move_character(term, i + 1, y, i, y);
            if (i == x)
                break;
        }
    }

    switch (term->context.charsets[term->context.current_charset])
    {
        case CHARSET_DEFAULT:
            break;
        case CHARSET_DEC_SPECIAL:
            c = term_dec_special_to_cp437(c);
            break;
    }

    term_raw_putchar(term, c);
}

// #if !defined(__x86_64__) && !defined(__aarch64__)
// #define TERM_XFER_CHUNK 8192

// static uint8_t xfer_buf[TERM_XFER_CHUNK];
// #endif

void term_write(struct term_t *term, uint64_t buf, uint64_t count)
{
    if (term->initialised == false || term->term_backend == NOT_READY)
        return;

    switch (count)
    {
        case TERM_CTX_SIZE:
        {
            uint64_t ret = term_context_size(term);
            memcpy((void*)(buf), &ret, sizeof(uint64_t));
            return;
        }
        case TERM_CTX_SAVE:
            term_context_save(term, buf);
            return;
        case TERM_CTX_RESTORE:
            term_context_restore(term, buf);
            return;
        case TERM_FULL_REFRESH:
            term_full_refresh(term);
            return;
    }

//     bool native = false;
// #if defined(__x86_64__) || defined(__aarch64__)
//     native = true;
// #endif

//    if (!term->runtime || native)
    if (term->runtime == false)
    {
        const char *s = (const char*)(buf);
        for (size_t i = 0; i < count; i++)
            term_putchar(term, s[i]);
    }
//     else
//     {
// #if !defined(__x86_64__) && !defined(__aarch64__)
//         while (count != 0)
//         {
//             uint64_t chunk;
//             if (count > TERM_XFER_CHUNK)
//                 chunk = TERM_XFER_CHUNK;
//             else
//                 chunk = count;

//             memcpy(xfer_buf, (void*)(buf), chunk);

//             for (size_t i = 0; i < chunk; i++)
//                 term_putchar(term, xfer_buf[i]);

//             count -= chunk;
//             buf += chunk;
//         }
// #endif
//     }

    if (term->autoflush)
        term_double_buffer_flush(term);
}

void term_print(struct term_t *term, const char *str)
{
    if (str == NULL)
        return;

    size_t length = 0;
    while(str[length])
        length++;

    term_write(term, (uint64_t)str, length);
}

void term_sgr(struct term_t *term)
{
    size_t i = 0;

    if (!term->context.esc_values_i)
        goto def;

    for (; i < term->context.esc_values_i; i++)
    {
        size_t offset;

        if (term->context.esc_values[i] == 0)
        {
def:
            if (term->context.reverse_video)
            {
                term->context.reverse_video = false;
                term_swap_palette(term);
            }
            term->context.bold = false;
            term->context.current_primary = (size_t)(-1);
            term_set_text_bg_default(term);
            term_set_text_fg_default(term);
            continue;
        }
        else if (term->context.esc_values[i] == 1)
        {
            term->context.bold = true;
            if (term->context.current_primary != (size_t)(-1))
            {
                if (!term->context.reverse_video)
                    term_set_text_fg_bright(term, term->context.current_primary);
                else
                    term_set_text_bg_bright(term, term->context.current_primary);
            }
            continue;
        }
        else if (term->context.esc_values[i] == 22)
        {
            term->context.bold = false;
            if (term->context.current_primary != (size_t)(-1))
            {
                if (!term->context.reverse_video)
                    term_set_text_fg(term, term->context.current_primary);
                else
                    term_set_text_bg(term, term->context.current_primary);
            }
            continue;
        }
        else if (term->context.esc_values[i] >= 30 && term->context.esc_values[i] <= 37)
        {
            offset = 30;
            term->context.current_primary = term->context.esc_values[i] - offset;

            if (term->context.reverse_video)
                goto set_bg;
set_fg:
            if (term->context.bold && !term->context.reverse_video)
                term_set_text_fg_bright(term, term->context.esc_values[i] - offset);
            else
                term_set_text_fg(term, term->context.esc_values[i] - offset);

            continue;
        }
        else if (term->context.esc_values[i] >= 40 && term->context.esc_values[i] <= 47)
        {
            offset = 40;
            if (term->context.reverse_video)
                goto set_fg;
set_bg:
            if (term->context.bold && term->context.reverse_video)
                term_set_text_bg_bright(term, term->context.esc_values[i] - offset);
            else
                term_set_text_bg(term, term->context.esc_values[i] - offset);
            continue;
        }
        else if (term->context.esc_values[i] >= 90 && term->context.esc_values[i] <= 97)
        {
            offset = 90;
            term->context.current_primary = term->context.esc_values[i] - offset;

            if (term->context.reverse_video)
                goto set_bg_bright;
set_fg_bright:
            term_set_text_fg_bright(term ,term->context.esc_values[i] - offset);
            continue;
        }
        else if (term->context.esc_values[i] >= 100 && term->context.esc_values[i] <= 107) {
            offset = 100;
            if (term->context.reverse_video)
                goto set_fg_bright;
set_bg_bright:
            term_set_text_bg_bright(term, term->context.esc_values[i] - offset);
            continue;
        }
        else if (term->context.esc_values[i] == 39)
        {
            term->context.current_primary = (size_t)(-1);

            if (term->context.reverse_video)
                term_swap_palette(term);
            term_set_text_fg_default(term);
            if (term->context.reverse_video)
                term_swap_palette(term);

            continue;
        }
        else if (term->context.esc_values[i] == 49)
        {
            if (term->context.reverse_video)
                term_swap_palette(term);

            term_set_text_bg_default(term);

            if (term->context.reverse_video)
                term_swap_palette(term);

            continue;
        }
        else if (term->context.esc_values[i] == 7)
        {
            if (!term->context.reverse_video)
            {
                term->context.reverse_video = true;
                term_swap_palette(term);
            }
            continue;
        }
        else if (term->context.esc_values[i] == 27)
        {
            if (term->context.reverse_video)
            {
                term->context.reverse_video = false;
                term_swap_palette(term);
            }
            continue;
        }
        else if (term->context.esc_values[i] == 38 || term->context.esc_values[i] == 48)
        {
            bool fg = term->context.esc_values[i] == 38;

            if (++i >= term->context.esc_values_i)
                break;

            switch (term->context.esc_values[i])
            {
                case 2:
                {
                    if (i + 3 >= term->context.esc_values_i)
                        goto out;

                    uint32_t rgb_value = 0;

                    rgb_value |= term->context.esc_values[i + 1] << 16;
                    rgb_value |= term->context.esc_values[i + 2] << 8;
                    rgb_value |= term->context.esc_values[i + 3];

                    i += 3;

                    fg ? term_set_text_fg_rgb(term, rgb_value) : term_set_text_bg_rgb(term, rgb_value);

                    break;
                }
                case 5:
                {
                    if (i + 1 >= term->context.esc_values_i)
                        goto out;

                    uint32_t col = term->context.esc_values[++i];

                    if (col < 8)
                        fg ? term_set_text_fg(term, col) : term_set_text_bg(term, col);
                    else if (col < 16)
                        fg ? term_set_text_fg_bright(term, col - 8) : term_set_text_bg_bright(term, col - 8);
                    else
                    {
                        uint32_t rgb_value = col256[col - 16];
                        fg ? term_set_text_fg_rgb(term, rgb_value) : term_set_text_bg_rgb(term, rgb_value);
                    }

                    break;
                }
                default:
                    continue;
            }
        }
    }

out:;
}

void term_dec_private_parse(struct term_t *term, uint8_t c)
{
    term->context.dec_private = false;

    if (term->context.esc_values_i == 0)
        return;

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

    switch (term->context.esc_values[0])
    {
        case 25:
            if (set == true)
                term_enable_cursor(term);
            else
                term_disable_cursor(term);
            return;
    }

    if (term->callback)
    {
        if (term->arg != 0)
            term->callback(term->arg, TERM_CB_DEC, term->context.esc_values_i, (uintptr_t)(term->context.esc_values), c);
        else
            term->callback(TERM_CB_DEC, term->context.esc_values_i, (uintptr_t)(term->context.esc_values), c, 0);
    }
}

void term_linux_private_parse(struct term_t *term)
{
    if (term->context.esc_values_i == 0)
        return;

    if (term->callback)
    {
        if (term->arg != 0)
            term->callback(term->arg, TERM_CB_LINUX, term->context.esc_values_i, (uintptr_t)(term->context.esc_values), 0);
        else
            term->callback(TERM_CB_LINUX, term->context.esc_values_i, (uintptr_t)(term->context.esc_values), 0, 0);
    }
}

void term_mode_toggle(struct term_t *term, uint8_t c)
{
    if (term->context.esc_values_i == 0)
        return;

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

    switch (term->context.esc_values[0])
    {
        case 4:
            term->context.insert_mode = set;
            return;
    }

    if (term->callback)
    {
        if (term->arg != 0)
            term->callback(term->arg, TERM_CB_MODE, term->context.esc_values_i, (uintptr_t)(term->context.esc_values), c);
        else
            term->callback(TERM_CB_MODE, term->context.esc_values_i, (uintptr_t)(term->context.esc_values), c, 0);
    }
}

void term_control_sequence_parse(struct term_t *term, uint8_t c)
{
    if (term->context.escape_offset == 2)
    {
        switch (c)
        {
            case '[':
                term->context.discard_next = true;
                goto cleanup;
            case '?':
                term->context.dec_private = true;
                return;
        }
    }

    if (c >= '0' && c <= '9')
    {
        if (term->context.esc_values_i == MAX_ESC_VALUES)
            return;
        term->context.rrr = true;
        term->context.esc_values[term->context.esc_values_i] *= 10;
        term->context.esc_values[term->context.esc_values_i] += c - '0';
        return;
    }

    if (term->context.rrr == true)
    {
        term->context.esc_values_i++;
        term->context.rrr = false;
        if (c == ';')
            return;
    }
    else if (c == ';')
    {
        if (term->context.esc_values_i == MAX_ESC_VALUES)
            return;
        term->context.esc_values[term->context.esc_values_i] = 0;
        term->context.esc_values_i++;
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

    for (size_t i = term->context.esc_values_i; i < MAX_ESC_VALUES; i++)
        term->context.esc_values[i] = esc_default;

    if (term->context.dec_private == true)
    {
        term_dec_private_parse(term, c);
        goto cleanup;
    }

    bool r;
    r = term_scroll_disable(term);
    size_t x, y;
    term_get_cursor_pos(term, &x, &y);

    switch (c) {
        case 'F':
            x = 0;
            // FALLTHRU
        case 'A':
        {
            if (term->context.esc_values[0] > y) term->context.esc_values[0] = y;
            size_t orig_y = y;
            size_t dest_y = y - term->context.esc_values[0];
            bool will_be_in_scroll_region = false;
            if ((term->context.scroll_top_margin >= dest_y && term->context.scroll_top_margin <= orig_y) || (term->context.scroll_bottom_margin >= dest_y && term->context.scroll_bottom_margin <= orig_y))
                will_be_in_scroll_region = true;

            if (will_be_in_scroll_region && dest_y < term->context.scroll_top_margin)
                dest_y = term->context.scroll_top_margin;

            term_set_cursor_pos(term, x, dest_y);
            break;
        }
        case 'E':
            x = 0;
            // FALLTHRU
        case 'e':
        case 'B':
        {
            if (y + term->context.esc_values[0] > term->rows - 1) term->context.esc_values[0] = (term->rows - 1) - y;
            size_t orig_y = y;
            size_t dest_y = y + term->context.esc_values[0];
            bool will_be_in_scroll_region = false;
            if ((term->context.scroll_top_margin >= orig_y && term->context.scroll_top_margin <= dest_y) || (term->context.scroll_bottom_margin >= orig_y && term->context.scroll_bottom_margin <= dest_y))
                will_be_in_scroll_region = true;

            if (will_be_in_scroll_region && dest_y >= term->context.scroll_bottom_margin)
                dest_y = term->context.scroll_bottom_margin - 1;

            term_set_cursor_pos(term, x, dest_y);
            break;
        }
        case 'a':
        case 'C':
            if (x + term->context.esc_values[0] > term->cols - 1)
                term->context.esc_values[0] = (term->cols - 1) - x;
            term_set_cursor_pos(term, x + term->context.esc_values[0], y);
            break;
        case 'D':
            if (term->context.esc_values[0] > x)
                term->context.esc_values[0] = x;
            term_set_cursor_pos(term, x - term->context.esc_values[0], y);
            break;
        case 'c':
            if (term->callback)
            {
                if (term->arg != 0)
                    term->callback(term->arg, TERM_CB_PRIVATE_ID, 0, 0, 0);
                else
                    term->callback(TERM_CB_PRIVATE_ID, 0, 0, 0, 0);
            }
            break;
        case 'd':
            term->context.esc_values[0] -= 1;

            if (term->context.esc_values[0] >= term->rows)
                term->context.esc_values[0] = term->rows - 1;

            term_set_cursor_pos(term, x, term->context.esc_values[0]);
            break;
        case 'G':
        case '`':
            term->context.esc_values[0] -= 1;

            if (term->context.esc_values[0] >= term->cols)
                term->context.esc_values[0] = term->cols - 1;

            term_set_cursor_pos(term, term->context.esc_values[0], y);
            break;
        case 'H':
        case 'f':
            term->context.esc_values[0] -= 1;
            term->context.esc_values[1] -= 1;

            if (term->context.esc_values[1] >= term->cols)
                term->context.esc_values[1] = term->cols - 1;

            if (term->context.esc_values[0] >= term->rows)
                term->context.esc_values[0] = term->rows - 1;

            term_set_cursor_pos(term, term->context.esc_values[1], term->context.esc_values[0]);
            break;
        case 'n':
            switch (term->context.esc_values[0])
            {
                case 5:
                    if (term->callback)
                    {
                        if (term->arg != 0)
                            term->callback(term->arg, TERM_CB_STATUS_REPORT, 0, 0, 0);
                        else
                            term->callback(TERM_CB_STATUS_REPORT, 0, 0, 0, 0);
                    }
                    break;
                case 6:
                    if (term->callback)
                    {
                        if (term->arg != 0)
                            term->callback(term->arg, TERM_CB_POS_REPORT, x + 1, y + 1, 0);
                        else
                            term->callback(TERM_CB_POS_REPORT, x + 1, y + 1, 0, 0);
                    }
                    break;
            }
            break;
        case 'q':
            if (term->callback)
            {
                if (term->arg != 0)
                    term->callback(term->arg, TERM_CB_KBD_LEDS, term->context.esc_values[0], 0, 0);
                else
                    term->callback(TERM_CB_KBD_LEDS, term->context.esc_values[0], 0, 0, 0);
            }
            break;
        case 'J':
            switch (term->context.esc_values[0])
            {
                case 0:
                {
                    size_t rows_remaining = term->rows - (y + 1);
                    size_t cols_diff = term->cols - (x + 1);
                    size_t to_clear = rows_remaining * term->cols + cols_diff;

                    for (size_t i = 0; i < to_clear; i++)
                        term_raw_putchar(term, ' ');

                    term_set_cursor_pos(term, x, y);
                    break;
                }
                case 1:
                {
                    term_set_cursor_pos(term, 0, 0);
                    bool b = false;
                    for (size_t yc = 0; yc < term->rows; yc++)
                    {
                        for (size_t xc = 0; xc < term->cols; xc++)
                        {
                            term_raw_putchar(term, ' ');
                            if (xc == x && yc == y)
                            {
                                term_set_cursor_pos(term, x, y);
                                b = true;
                                break;
                            }
                        }
                        if (b == true)
                            break;
                    }
                    break;
                }
                case 2:
                case 3:
                    term_clear(term, false);
                    break;
            }
            break;
        case '@':
            for (size_t i = term->cols - 1; ; i--)
            {
                term_move_character(term, i + term->context.esc_values[0], y, i, y);
                term_set_cursor_pos(term, i, y);
                term_raw_putchar(term, ' ');
                if (i == x)
                    break;
            }
            term_set_cursor_pos(term, x, y);
            break;
        case 'P':
            for (size_t i = x + term->context.esc_values[0]; i < term->cols; i++)
                term_move_character(term, i - term->context.esc_values[0], y, i, y);
            term_set_cursor_pos(term, term->cols - term->context.esc_values[0], y);
            // FALLTHRU
        case 'X':
            for (size_t i = 0; i < term->context.esc_values[0]; i++)
                term_raw_putchar(term, ' ');
            term_set_cursor_pos(term, x, y);
            break;
        case 'm':
            term_sgr(term);
            break;
        case 's':
            term_get_cursor_pos(term, &term->context.saved_cursor_x, &term->context.saved_cursor_y);
            break;
        case 'u':
            term_set_cursor_pos(term, term->context.saved_cursor_x, term->context.saved_cursor_y);
            break;
        case 'K':
            switch (term->context.esc_values[0])
            {
                case 0:
                    for (size_t i = x; i < term->cols; i++)
                        term_raw_putchar(term, ' ');
                    term_set_cursor_pos(term, x, y);
                    break;
                case 1:
                    term_set_cursor_pos(term, 0, y);
                    for (size_t i = 0; i < x; i++)
                        term_raw_putchar(term, ' ');
                    break;
                case 2:
                    term_set_cursor_pos(term, 0, y);
                    for (size_t i = 0; i < term->cols; i++)
                        term_raw_putchar(term, ' ');
                    term_set_cursor_pos(term, x, y);
                    break;
            }
            break;
        case 'r':
            term->context.scroll_top_margin = 0;
            term->context.scroll_bottom_margin = term->rows;
            if (term->context.esc_values_i > 0)
                term->context.scroll_top_margin = term->context.esc_values[0] - 1;

            if (term->context.esc_values_i > 1)
                term->context.scroll_bottom_margin = term->context.esc_values[1];

            if (term->context.scroll_top_margin >= term->rows || term->context.scroll_bottom_margin > term->rows || term->context.scroll_top_margin >= (term->context.scroll_bottom_margin - 1))
            {
                term->context.scroll_top_margin = 0;
                term->context.scroll_bottom_margin = term->rows;
            }
            term_set_cursor_pos(term, 0, 0);
            break;
        case 'l':
        case 'h':
            term_mode_toggle(term, c);
            break;
        case ']':
            term_linux_private_parse(term);
            break;
    }

    if (r == true)
        term_scroll_enable(term);

cleanup:
    term->context.control_sequence = false;
    term->context.escape = false;
}

void term_escape_parse(struct term_t *term, uint8_t c)
{
    term->context.escape_offset++;

    if (term->context.control_sequence == true)
    {
        term_control_sequence_parse(term, c);
        return;
    }

    if (term->context.csi == true)
    {
        term->context.csi = false;
        goto is_csi;
    }

    size_t x, y;
    term_get_cursor_pos(term, &x, &y);

    switch (c)
    {
        case '[':
is_csi:
            for (size_t i = 0; i < MAX_ESC_VALUES; i++)
                term->context.esc_values[i] = 0;
            term->context.esc_values_i = 0;
            term->context.rrr = false;
            term->context.control_sequence = true;
            return;
        case '7':
            term_save_state(term);
            break;
        case '8':
            term_restore_state(term);
            break;
        case 'c':
            term_reinit(term);
            term_clear(term, true);
            break;
        case 'D':
            if (y == term->context.scroll_bottom_margin - 1)
            {
                term_scroll(term);
                term_set_cursor_pos(term, x, y);
            }
            else term_set_cursor_pos(term, x, y + 1);
            break;
        case 'E':
            if (y == term->context.scroll_bottom_margin - 1)
            {
                term_scroll(term);
                term_set_cursor_pos(term, 0, y);
            }
            else term_set_cursor_pos(term, 0, y + 1);
            break;
        case 'M':
            if (y == term->context.scroll_top_margin)
            {
                term_revscroll(term);
                term_set_cursor_pos(term, 0, y);
            }
            else term_set_cursor_pos(term, 0, y - 1);
            break;
        case 'Z':
            if (term->callback)
            {
                if (term->arg != 0)
                    term->callback(term->arg, TERM_CB_PRIVATE_ID, 0, 0, 0);
                else
                    term->callback(TERM_CB_PRIVATE_ID, 0, 0, 0, 0);
            }
            break;
        case '(':
        case ')':
            term->context.g_select = c - '\'';
            break;
        case '\e':
            if (term->runtime == false)
                term_raw_putchar(term, c);
            break;
    }

    term->context.escape = false;
}

void term_raw_putchar(struct term_t *term, uint8_t c)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_putchar(term->gterm, c);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_putchar(term->tterm, c);
#endif
}

void term_clear(struct term_t *term, bool move)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_clear(term->gterm, move);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_clear(term->tterm, move);
#endif
}

void term_enable_cursor(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_enable_cursor(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_enable_cursor(term->tterm);
#endif
}

bool term_disable_cursor(struct term_t *term)
{
    if (term->initialised == false)
        return false;

    if (term->term_backend == VBE && term->gterm)
        return gterm_disable_cursor(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        return tterm_disable_cursor(term->tterm);
#endif
    return false;
}

void term_set_cursor_pos(struct term_t *term, size_t x, size_t y)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_cursor_pos(term->gterm, x, y);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_cursor_pos(term->tterm, x, y);
#endif
}

void term_get_cursor_pos(struct term_t *term, size_t *x, size_t *y)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_get_cursor_pos(term->gterm, x, y);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_get_cursor_pos(term->tterm, x, y);
#endif
}

void term_set_text_fg(struct term_t *term, size_t fg)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_fg(term->gterm, fg);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_text_fg(term->tterm, fg);
#endif
}

void term_set_text_bg(struct term_t *term, size_t bg)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_bg(term->gterm, bg);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_text_bg(term->tterm, bg);
#endif
}

void term_set_text_fg_bright(struct term_t *term, size_t fg)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_fg_bright(term->gterm, fg);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_text_fg_bright(term->tterm, fg);
#endif
}

void term_set_text_bg_bright(struct term_t *term, size_t bg)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_bg_bright(term->gterm, bg);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_text_bg_bright(term->tterm, bg);
#endif
}

void term_set_text_fg_rgb(struct term_t *term, uint32_t fg)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_fg_rgb(term->gterm, fg);
}

void term_set_text_bg_rgb(struct term_t *term, uint32_t bg)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_bg_rgb(term->gterm, bg);
}

void term_set_text_fg_default(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_fg_default(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_text_fg_default(term->tterm);
#endif
}

void term_set_text_bg_default(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_set_text_bg_default(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_set_text_bg_default(term->tterm);
#endif
}

bool term_scroll_disable(struct term_t *term)
{
    if (term->initialised == false)
        return false;

    if (term->term_backend == VBE && term->gterm)
        return gterm_scroll_disable(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        return tterm_scroll_disable(term->tterm);
#endif
    return false;
}

void term_scroll_enable(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_scroll_enable(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_scroll_enable(term->tterm);
#endif
}

void term_move_character(struct term_t *term, size_t new_x, size_t new_y, size_t old_x, size_t old_y)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_move_character(term->gterm, new_x, new_y, old_x, old_y);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_move_character(term->tterm, new_x, new_y, old_x, old_y);
#endif
}

void term_scroll(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_scroll(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_scroll(term->tterm);
#endif
}

void term_revscroll(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_revscroll(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_revscroll(term->tterm);
#endif
}

void term_swap_palette(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_swap_palette(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_swap_palette(term->tterm);
#endif
}

void term_save_state(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_save_state(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_save_state(term->tterm);
#endif

    term->context.saved_state_bold = term->context.bold;
    term->context.saved_state_reverse_video = term->context.reverse_video;
    term->context.saved_state_current_charset = term->context.current_charset;
    term->context.saved_state_current_primary = term->context.current_primary;
}

void term_restore_state(struct term_t *term)
{
    if (term->initialised == false)
        return;

    term->context.bold = term->context.saved_state_bold;
    term->context.reverse_video = term->context.saved_state_reverse_video;
    term->context.current_charset = term->context.saved_state_current_charset;
    term->context.current_primary = term->context.saved_state_current_primary;

    if (term->term_backend == VBE && term->gterm)
        gterm_restore_state(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_restore_state(term->tterm);
#endif
}

void term_double_buffer_flush(struct term_t *term)
{
    if (term->initialised == false)
        return;

    if (term->term_backend == VBE && term->gterm)
        gterm_double_buffer_flush(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_double_buffer_flush(term->tterm);
#endif
}

uint64_t term_context_size(struct term_t *term)
{
    if (term->initialised == false)
        return 0;

    uint64_t ret = sizeof(struct term_context);

    if (term->term_backend == VBE && term->gterm)
        ret += gterm_context_size(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        ret += tterm_context_size(term->tterm);
#endif

    return ret;
}

void term_context_save(struct term_t *term, uint64_t ptr)
{
    if (term->initialised == false)
        return;

    memcpy((void*)ptr, &term->context, sizeof(struct term_context));
    ptr += sizeof(struct term_context);

    if (term->term_backend == VBE && term->gterm)
        gterm_context_save(term->gterm, ptr);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_context_save(term->tterm, ptr);
#endif
}

void term_context_restore(struct term_t *term, uint64_t ptr)
{
    if (term->initialised == false)
        return;

    memcpy(&term->context, (void*)ptr, sizeof(struct term_context));
    ptr += sizeof(struct term_context);

    if (term->term_backend == VBE && term->gterm)
        gterm_context_restore(term->gterm, ptr);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_context_restore(term->tterm, ptr);
#endif
}

void term_full_refresh(struct term_t *term)
{
    if (term->initialised == false) return;

    if (term->term_backend == VBE && term->gterm)
        gterm_full_refresh(term->gterm);
#if defined(__i386__) || defined(__x86_64__)
    else if (term->term_backend == TEXTMODE && term->tterm)
        tterm_full_refresh(term->tterm);
#endif
}