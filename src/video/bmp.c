#include <bmp.h>
#include <printk.h>

void bmp_parse(u8 *data, struct img_info *inf)
{
    struct bmp_fh *fh = data;
    if (fh->bfType != 0x4d42) {
        printk("Not a BMP!\n");
        return;
    }
    struct bmp_ih *ih = data + sizeof(struct bmp_fh);

    if (ih->biSize != 40) {
        printk("Unsupported BMP version!\n");
        return;
    }

    inf->width = ih->biWidth;
    inf->height = ih->biHeight;
    inf->bit_count = ih->biBitCount;
    inf->data = data + fh->bfOffBits;
}

void bmp_draw32(u32 *fb, u32 ppsl, struct img_info *img, u32 destx, u32 desty)
{
    switch (img->bit_count) {
    case 24:
        for (u32 y = 0; y < img->height; y++) {
            for (u32 x = 0; x < img->width; x++) {
                u32 *rgb = img->data + (img->height - y - 1) * ((3 * img->width + 3) & ~3) + x * 3;
                fb[(desty + y) * ppsl + destx + x] = *rgb & 0xffffff;
            }
        }
        break;
    case 32:
        u32 *pxs = img->data;
        for (u32 y = 0; y < img->height; y++) {
            for (u32 x = 0; x < img->width; x++) {
                fb[(desty + y) * ppsl + destx + x] = pxs[(img->height - y - 1) * img->width + x];
            }
        }
        break;
    default:
        printk("Unsupported BMP bit count!\n");
        break;
    }
}

void bmp_fill_rect32(u32 *fb, u32 ppsl, u32 x0, u32 y0, u32 w, u32 h, u32 rgb)
{
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            fb[(y0 + y) * ppsl + x0 + x] = rgb;
        }
    }
}

void bmp_copy_rect32(u32 *src, u32 *dest, u32 sppsl, u32 dppsl, u32 x0, u32 y0, u32 w, u32 h, u32 dx, u32 dy)
{
    for (u32 y = 0; y < h; y++) {
        for (u32 x = 0; x < w; x++) {
            dest[(dy + y) * dppsl + dx + x] = src[(y0 + y) * sppsl + x0 + x];
        }
    }
}