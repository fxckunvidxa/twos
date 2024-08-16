#pragma once
#include <types.h>

struct bmp_fh {
    u16 bfType;
    u32 bfSize;
    u16 bfReserved1;
    u16 bfReserved2;
    u32 bfOffBits;
} __attribute__((packed));

struct bmp_ih {
    u32 biSize;
    u32 biWidth;
    u32 biHeight;
    u16 biPlanes;
    u16 biBitCount;
    u32 biCompression;
    u32 biSizeImage;
    u32 biXPelsPerMeter;
    u32 biYPelsPerMeter;
    u32 biClrUsed;
    u32 biClrImportant;
} __attribute__((packed));

struct img_info {
    u32 width;
    u32 height;
    u16 bit_count;
    u8 *data;
};

void bmp_parse(u8 *data, struct img_info *inf);
void bmp_draw32(u32 *fb, u32 ppsl, struct img_info *img, u32 destx, u32 desty);
void bmp_fill_rect32(u32 *fb, u32 ppsl, u32 x0, u32 y0, u32 w, u32 h, u32 rgb);
void bmp_copy_rect32(u32 *src, u32 *dest, u32 sppsl, u32 dppsl, u32 x0, u32 y0, u32 w, u32 h, u32 dx, u32 dy);