#pragma once
#include <types.h>

#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MAGIC 0x0436
#define PSF1_SEPARATOR 0xFFFF
#define PSF1_STARTSEQ 0xFFFE

#define PSF2_HAS_UNICODE_TABLE 0x01
#define PSF2_SEPARATOR 0xFF
#define PSF2_STARTSEQ 0xFE
#define PSF2_MAGIC 0x864ab572

struct vt_cell {
    u32 fg, bg;
    u16 chr;
    bool dirty;
};

struct vt_font {
    uptr data;
    u16 *utbl;
    u16 numg;
    u8 bpg, bpl, w, h;
};

struct vt_ctx {
    struct vt_font font;
    struct vt_cell *buf;
    u32 *fb_ptr;
    u32 fg_clr, bg_clr;
    u16 cols, lns;
    u16 cur_x, cur_y;
    u16 sw, sh, ppsl;
    bool cur_enabled;
};

union psf_font {
    struct {
        u16 magic;
        u8 mode;
        u8 charsize;
    } __attribute__((packed)) v1;
    struct {
        u32 magic;
        u32 version;
        u32 headersize; /* offset of bitmaps in file */
        u32 flags;
        u32 length;        /* number of glyphs */
        u32 charsize;      /* number of bytes for each character */
        u32 height, width; /* max dimensions of glyphs */
                           /* charsize = height * ((width + 7) / 8) */
    } __attribute__((packed)) v2;
};

void vt_init();
void vt_set_font(struct vt_ctx *ctx, union psf_font *psf);
void vt_set_colors(struct vt_ctx *ctx, u32 fg, u32 bg);
void vt_clear(struct vt_ctx *ctx);
void vt_putc_utf8(u8 c);
void vt_set_cursor_state(struct vt_ctx *ctx, bool enabled);
struct vt_ctx *vt_get_active_ctx();
void vt_redraw(struct vt_ctx *ctx, bool force);
void vt_init_ctx(struct vt_ctx *ctx, struct generic_fb *fb, u16 off_x, u16 off_y, u16 w, u16 h);