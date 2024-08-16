#include <arch/x86_64/asm.h>
#include <arch/x86_64/bootinfo.h>
#include <bmp.h>
#include <fb.h>
#include <printk.h>
#include <time.h>
#include <vm.h>
#include <vt.h>
#include <malloc.h>
#include <arch/x86_64/pci.h>

extern struct bootinfo *g_bi;

void kmain()
{
    /*printk("   _______       __        _       ___       __         ____  _____\n"
           "  /_  __(_)___  / /____  _| |     / (_)___  / /____  __/ __ \\/ ___/\n"
           "   / / / / __ \\/ //_/ / / / | /| / / / __ \\/ //_/ / / / / / /\\__ \\\n"
           "  / / / / / / / ,< / /_/ /| |/ |/ / / / / / ,< / /_/ / /_/ /___/ / \n"
           " /_/ /_/_/ /_/_/|_|\\__, / |__/|__/_/_/ /_/_/|_|\\__, /\\____//____/ \n"
           "                  /____/                      /____/               \n\n");*/

    printk("\nWelcome to TinkyWinkyOS! Built on " __DATE__ " " __TIME__
           "\nMade in Russia. (C) 2022-2024, Fufelschmertz Evil Inc.\n\n");
           
    /*struct timespec t;
    time_get_uptime(&t);
    for (int i = 2; i > 0; i--) {
        time_t cur = t.tv_sec;
        while (t.tv_sec == cur) {
            time_get_uptime(&t);
            cpu_pause();
        }
    }

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

    struct img_info img;
    bmp_parse(g_bi->data[1].base, &img);
    u32 *bmp = kmalloc(img.width * img.height * 4);
    bmp_draw32(bmp, img.width, &img, 0, 0);
    struct generic_fb *fb = fb_get();
    bmp_fill_rect32(fb->base, fb->pitch / 4, 0, 16, fb->width, fb->height - 16, 0);
    u32 *sbuf = kzalloc(fb->width * (fb->height - 16) * 4);
    u64 t0 = time_get_up_ns();
    const u64 fps = 60;
    const u64 v_num = 1, v_den = 15;
    u32 px = 0, py = 0;
    while (1) {
        u64 t_ms = (time_get_up_ns() - t0) / 1000000;
        u64 p = t_ms * v_num / v_den;
        u32 w = fb->width - img.width;
        u32 h = fb->height - 16 - img.height;
        u32 x = (p / w) % 2 ? w - (p % w) : p % w;
        u32 y = (p / h) % 2 ? h - (p % h) : p % h;
        bmp_fill_rect32(sbuf, fb->width, px, py, img.width, img.height, 0);
        bmp_copy_rect32(bmp, sbuf, img.width, fb->width, 0, 0, img.width, img.height, x, y);
        u32 x0 = MIN(px, x), y0 = MIN(py, y), x1 = MAX(px, x), y1 = MAX(py, y);
        bmp_copy_rect32(sbuf, fb->base, fb->width, fb->pitch / 4, x0, y0, img.width + x1 - x0, img.height + y1 - y0, x0, y0 + 16);
        px = x;
        py = y;
        while ((time_get_up_ns() - t0) / 1000000 < t_ms + 1000 / fps) {
            mt_switch_thread();
        }
    }*/
}
