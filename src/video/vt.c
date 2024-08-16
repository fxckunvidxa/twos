#include <arch/x86_64/rtc.h>
#include <fb.h>
#include <malloc.h>
#include <mt.h>
#include <printk.h>
#include <time.h>
#include <vt.h>

static struct vt_ctx *g_active_ctx;
static struct vt_ctx g_main_scr, g_status_line;
extern u8 vt_font_psf[];

static void vt_init_buf(struct vt_ctx *ctx)
{
    if (!ctx->font.data || !ctx->sw || !ctx->sh) {
        return;
    }
    u16 nc = ctx->sw / ctx->font.w, nl = ctx->sh / ctx->font.h;
    if (ctx->cols != nc || ctx->lns != nl) {
        ctx->cols = nc;
        ctx->lns = nl;
        if (ctx->buf) {
            ctx->buf = krealloc(ctx->buf, nc * nl * sizeof(struct vt_cell));
        } else {
            ctx->buf = kmalloc(nc * nl * sizeof(struct vt_cell));
        }
    }
}

void vt_set_font(struct vt_ctx *ctx, union psf_font *psf)
{
    if (psf->v1.magic == PSF1_MAGIC) {
        ctx->font.data = (uptr)psf + sizeof(psf->v1);
        ctx->font.numg = (psf->v1.mode & PSF1_MODE512) ? 512 : 256;
        ctx->font.w = 8;
        ctx->font.bpg = ctx->font.h = psf->v1.charsize;
        ctx->font.bpl = 1;
        if (psf->v1.mode & PSF1_MODEHASTAB) {
            ctx->font.utbl = kcalloc(0x10000, 2);
            u16 *ud = ctx->font.data + ctx->font.bpg * ctx->font.numg;
            for (u16 g = 0; g < ctx->font.numg; g++) {
                while (*ud != PSF1_SEPARATOR && *ud != PSF1_STARTSEQ) {
                    ctx->font.utbl[*(ud++)] = g;
                }
                if (*ud == PSF1_STARTSEQ) {
                    while (*ud != PSF1_SEPARATOR) {
                        ud++;
                    }
                }
                ud++;
            }
        }
    } else if (psf->v2.magic == PSF2_MAGIC) {
        ctx->font.data = (uptr)psf + psf->v2.headersize;
        ctx->font.numg = psf->v2.length;
        ctx->font.w = psf->v2.width;
        ctx->font.h = psf->v2.height;
        ctx->font.bpg = psf->v2.charsize;
        ctx->font.bpl = ctx->font.bpg / ctx->font.h;
        ctx->font.utbl = NULL;
    }
    vt_init_buf(ctx);
}

static void vt_draw_glyph(struct vt_ctx *ctx, u16 chr, u16 cx, u16 cy, u32 fg, u32 bg)
{
    if (ctx->font.utbl) {
        chr = ctx->font.utbl[chr];
    }
    if (!chr || chr >= ctx->font.numg) {
        chr = '?';
    }
    u8 *glyph = ctx->font.data + ctx->font.bpg * chr;
    usize fb_pixoff = cy * ctx->font.h * ctx->ppsl + cx * ctx->font.w;
    for (u8 y = 0; y < ctx->font.h; y++, glyph += ctx->font.bpl) {
        for (u8 x = 0; x < ctx->font.w; x++) {
            *(ctx->fb_ptr + fb_pixoff + x + y * ctx->ppsl) = glyph[x >> 3] & (0x80 >> (x & 7)) ? fg : bg;
        }
    }
}

void vt_set_colors(struct vt_ctx *ctx, u32 fg, u32 bg)
{
    ctx->fg_clr = fg;
    ctx->bg_clr = bg;
}

static void vt_set_cell(struct vt_ctx *ctx, u16 x, u16 y, u16 chr, u32 fg, u32 bg)
{
    struct vt_cell *c = &ctx->buf[y * ctx->cols + x];
    if (c->chr != chr || c->fg != fg || c->bg != bg) {
        c->chr = chr;
        c->fg = fg;
        c->bg = bg;
        c->dirty = true;
    }
}

static void vt_redraw_cell(struct vt_ctx *ctx, u16 x, u16 y, bool force)
{
    struct vt_cell *cell = &ctx->buf[y * ctx->cols + x];
    if (!cell->dirty && !force) {
        return;
    }
    vt_draw_glyph(ctx, cell->chr, x, y, cell->fg, cell->bg);
    cell->dirty = false;
}

static void vt_draw_cursor(struct vt_ctx *ctx, u16 x, u16 y)
{
    if (!ctx->cur_enabled) {
        vt_redraw_cell(ctx, x, y, true);
        return;
    }
    struct vt_cell *cell = &ctx->buf[y * ctx->cols + x];
    vt_draw_glyph(ctx, cell->chr, x, y, cell->bg, cell->fg);
    cell->dirty = true;
}

void vt_set_cursor_state(struct vt_ctx *ctx, bool enabled)
{
    ctx->cur_enabled = enabled;
    if (!enabled) {
        vt_redraw_cell(ctx, ctx->cur_x, ctx->cur_y, false);
    } else {
        vt_draw_cursor(ctx, ctx->cur_x, ctx->cur_y);
    }
}

void vt_redraw(struct vt_ctx *ctx, bool force)
{
    for (u16 y = 0; y < ctx->lns; y++) {
        for (u16 x = 0; x < ctx->cols; x++) {
            if (ctx->cur_x == x && ctx->cur_y == y) {
                vt_draw_cursor(ctx, x, y);
            } else {
                vt_redraw_cell(ctx, x, y, force);
            }
        }
    }
}

static void vt_scroll(struct vt_ctx *ctx)
{
    for (u16 y = 0; y < ctx->lns - 1; y++) {
        for (u16 x = 0; x < ctx->cols; x++) {
            struct vt_cell *c = &ctx->buf[(y + 1) * ctx->cols + x];
            vt_set_cell(ctx, x, y, c->chr, c->fg, c->bg);
        }
    }
    for (u16 x = 0; x < ctx->cols; x++) {
        vt_set_cell(ctx, x, ctx->lns - 1, ' ', ctx->fg_clr, ctx->bg_clr);
    }
}

static void vt_move_cursor(struct vt_ctx *ctx, u16 x, u16 y)
{
    vt_redraw_cell(ctx, ctx->cur_x, ctx->cur_y, false);

    ctx->cur_x = x;
    ctx->cur_y = y;

    if (ctx->cur_x >= ctx->cols) {
        ctx->cur_x = 0;
        ctx->cur_y++;
    }

    if (ctx->cur_y >= ctx->lns) {
        ctx->cur_x = 0;
        ctx->cur_y = ctx->lns - 1;
        vt_scroll(ctx);
        vt_redraw(ctx, false);
    } else {
        vt_draw_cursor(ctx, ctx->cur_x, ctx->cur_y);
    }
}

static void vt_clear_buf(struct vt_ctx *ctx)
{
    for (u16 y = 0; y < ctx->lns; y++) {
        for (u16 x = 0; x < ctx->cols; x++) {
            vt_set_cell(ctx, x, y, ' ', ctx->fg_clr, ctx->bg_clr);
        }
    }
    ctx->cur_x = ctx->cur_y = 0;
}

void vt_clear(struct vt_ctx *ctx)
{
    vt_clear_buf(ctx);
    vt_redraw(ctx, false);
}

void vt_putc(struct vt_ctx *ctx, u16 c)
{
    switch (c) {
    case '\0':
        vt_putc(ctx, '?');
        break;
    case '\n':
        vt_move_cursor(ctx, 0, ctx->cur_y + 1);
        break;
    case '\t':
        vt_move_cursor(ctx, ctx->cur_x + 8 - (ctx->cur_x % 8), ctx->cur_y);
        break;
    case '\r':
        vt_move_cursor(ctx, 0, ctx->cur_y);
        break;
    case '\b':
        if (ctx->cur_x == 0 && ctx->cur_y > 0) {
            vt_set_cell(ctx, ctx->cols - 1, ctx->cur_y - 1, ' ', ctx->fg_clr, ctx->bg_clr);
            vt_move_cursor(ctx, ctx->cols - 1, ctx->cur_y - 1);
        } else if (ctx->cur_x > 0) {
            vt_set_cell(ctx, ctx->cur_x - 1, ctx->cur_y, ' ', ctx->fg_clr, ctx->bg_clr);
            vt_move_cursor(ctx, ctx->cur_x - 1, ctx->cur_y);
        }
        break;
    default:
        vt_set_cell(ctx, ctx->cur_x, ctx->cur_y, c, ctx->fg_clr, ctx->bg_clr);
        vt_move_cursor(ctx, ctx->cur_x + 1, ctx->cur_y);
        break;
    }
}

void vt_putc_utf8(u8 c)
{
    if (c > 0xff) {
        vt_putc(g_active_ctx, c);
        return;
    }
    static u32 chr;
    static u8 n, cn = 0;
    if (cn == 0) {
        if (c >> 7) {
            if ((c >> 5) == 0b110) {
                n = 2;
                chr = (c & 0b11111) << 6;
            } else if ((c >> 4) == 0b1110) {
                n = 3;
                chr = (c & 0b1111) << 12;
            } else if ((c >> 3) == 0b11110) {
                n = 4;
                chr = (c & 0b111) << 18;
            }
            cn++;
        } else {
            vt_putc(g_active_ctx, c);
        }
    } else {
        chr |= (c & 0b111111) << ((n - (cn++) - 1) * 6);
        if (cn == n) {
            cn = 0;
            if (chr > 0xffff) {
                chr = '?';
            }
            vt_putc(g_active_ctx, chr);
        }
    }
}

struct vt_ctx *vt_get_active_ctx()
{
    return g_active_ctx;
}

struct vt_ctx *vt_set_active_ctx(struct vt_ctx *nw)
{
    struct vt_ctx *old = g_active_ctx;
    g_active_ctx = nw;
    return old;
}

void vt_init_ctx(struct vt_ctx *ctx, struct generic_fb *fb, u16 off_x, u16 off_y, u16 w, u16 h)
{
    ctx->fb_ptr = (uptr)(fb->base + off_y * fb->pitch + off_x * 4);
    ctx->sw = w;
    ctx->sh = h;
    ctx->ppsl = fb->pitch / 4;
    vt_set_font(ctx, vt_font_psf);
}

static void vt_update_status_line()
{
    u64 prev_update = 0;
    while (1) {
        while (prev_update && (time_get_up_ns() - prev_update < 1000000000)) {
            mt_switch_thread();
        }
        prev_update = time_get_up_ns();
        struct rtc_time tm;
        rtc_read(&tm);
        vt_clear_buf(&g_status_line);
        vt_move_cursor(&g_status_line, (g_status_line.cols - 34) / 2, 0);
        asm volatile("cli");
        struct vt_ctx *old = vt_set_active_ctx(&g_status_line);
        printk("TinkyWinkyOS | %02hhu.%02hhu.%04hu %02hhu:%02hhu:%02hhu", tm.day, tm.mon, tm.year, tm.hour,
               tm.min, tm.sec);
        vt_move_cursor(&g_status_line, 1, 0);
        pm_print_used_pages();
        vt_set_active_ctx(old);
        asm volatile("sti");
        mt_switch_thread();
    }
}

void vt_init()
{
    struct generic_fb *fb = fb_get();

    vt_init_ctx(&g_main_scr, fb, 0, 16, fb->width, fb->height - 16);
    vt_set_colors(&g_main_scr, 0xaaaaaa, 0);
    vt_clear_buf(&g_main_scr);
    vt_redraw(&g_main_scr, true);
    vt_set_cursor_state(&g_main_scr, true);
    vt_set_active_ctx(&g_main_scr);

    vt_init_ctx(&g_status_line, fb, 0, 0, fb->width, 16);
    vt_set_colors(&g_status_line, 0xffffff, 0xff0000);
    vt_clear_buf(&g_status_line);
    vt_redraw(&g_status_line, true);
    vt_set_cursor_state(&g_status_line, false);

    mt_start_thread(mt_create_kthread(vt_update_status_line, NULL));
}