#include "image.hpp"
#include "term.hpp"

#define DIV_ROUNDUP(A, B) \
({ \
    typeof(A) _a_ = A; \
    typeof(B) _b_ = B; \
    (_a_ + (_b_ - 1)) / _b_; \
})

#define ALIGN_UP(A, B) \
({ \
    typeof(A) _a__ = A; \
    typeof(B) _b__ = B; \
    DIV_ROUNDUP(_a__, _b__) * _b__; \
})

bool image_t::bmp_open_image(uint64_t file, uint64_t size)
{
    bmp_header header = bmp_header();
    memcpy(&header, reinterpret_cast<uint8_t*>(file), sizeof(bmp_header));

    char signature[2]
    {
        static_cast<char>(header.bf_signature),
        static_cast<char>(header.bf_signature >> 8)
    };
    if (signature[0] != 'B' || signature[1] != 'M') return false;

    if (header.bi_bpp % 8 != 0) return false;

    uint32_t bf_size;
    if (header.bf_offset + header.bf_size > size)
    {
        bf_size = size - header.bf_offset;
    }
    else bf_size = header.bf_size;

    img = static_cast<uint8_t*>(alloc_mem(bf_size));
    memcpy(img, reinterpret_cast<uint8_t*>(file) + header.bf_offset, bf_size);

    allocated_size = header.bf_size;

    x_size = header.bi_width;
    y_size = header.bi_height;
    pitch = ALIGN_UP(header.bi_width * header.bi_bpp, 32) / 8;
    bpp = header.bi_bpp;
    img_width = header.bi_width;
    img_height = header.bi_height;

    return true;
}

void image_t::make_centered(int frame_x_size, int frame_y_size, uint32_t back_colour)
{
    type = CENTERED;

    x_displacement = frame_x_size / 2 - x_size / 2;
    y_displacement = frame_y_size / 2 - y_size / 2;
    this->back_colour = back_colour;
}

void image_t::make_stretched(int new_x_size, int new_y_size)
{
    type = STRETCHED;

    x_size = new_x_size;
    y_size = new_y_size;
}

bool image_t::open(uint64_t file, uint64_t size)
{
    type = TILED;
    if (bmp_open_image(file, size)) return true;
    return false;
}

void image_t::close()
{
    free_mem(img, allocated_size);
}

image_t::image_t(uint64_t file, uint64_t size)
{
    open(file, size);
}

image_t::~image_t()
{
    close();
}