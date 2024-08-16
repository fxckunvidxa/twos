#include <fb.h>

static struct generic_fb main_fb;

void fb_set(struct generic_fb *fb)
{
    main_fb.base = fb->base;
    main_fb.size = fb->size;
    main_fb.width = fb->width;
    main_fb.height = fb->height;
    main_fb.pitch = fb->pitch;
}

struct generic_fb *fb_get()
{
    return &main_fb;
}