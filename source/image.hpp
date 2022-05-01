#pragma once

#include <stddef.h>
#include <stdint.h>

enum image_style
{
    CENTERED,
    STRETCHED,
    TILED
};

struct bmp_header
{
    uint16_t bf_signature;
    uint32_t bf_size;
    uint32_t reserved;
    uint32_t bf_offset;

    uint32_t bi_size;
    uint32_t bi_width;
    uint32_t bi_height;
    uint16_t bi_planes;
    uint16_t bi_bpp;
    uint32_t bi_compression;
    uint32_t bi_image_size;
    uint32_t bi_xcount;
    uint32_t bi_ycount;
    uint32_t bi_clr_used;
    uint32_t bi_clr_important;
    uint32_t red_mask;
    uint32_t green_mask;
    uint32_t blue_mask;
} __attribute__((packed));

class image_t
{
    private:
    bool bmp_open_image(uint64_t file, uint64_t size);

    public:
    size_t allocated_size;
    size_t x_size;
    size_t y_size;
    image_style type;
    uint8_t *img;
    int bpp;
    int pitch;
    size_t img_width;
    size_t img_height;
    size_t x_displacement;
    size_t y_displacement;
    uint32_t back_colour;

    image_t(uint64_t file, uint64_t size);
    image_t() { }
    ~image_t();

    void make_centered(int frame_x_size, int frame_y_size, uint32_t back_colour);
    void make_stretched(int new_x_size, int new_y_size);
    bool open(uint64_t file, uint64_t size);
    void close();
};