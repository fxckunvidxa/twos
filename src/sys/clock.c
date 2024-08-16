#define PRINT_FMT(x) "clock: " x
#include <clock.h>
#include <kstring.h>
#include <printk.h>

#define NUM_CL 8
static struct clock_info g_clocks[NUM_CL];

bool clock_register(const char *name, u64 freq, u64 mask, u64 (*rd)())
{
    for (int i = 0; i < NUM_CL; i++) {
        if (!g_clocks[i].read) {
            memcpy(g_clocks[i].name, name, 15);
            g_clocks[i].frequency = freq;
            g_clocks[i].mask = mask;
            g_clocks[i].read = rd;
            print("registered < %s > id=%d, freq=%llu Hz, mask=%016zx\n", name, i, freq, mask);
            return true;
        }
    }
    return false;
}

struct clock_info *clock_get_by_id(int id)
{
    if (id >= 0 && id < NUM_CL && g_clocks[id].read) {
        return &g_clocks[id];
    }
    return NULL;
}

struct clock_info *clock_get(const char *name) 
{
    for (int i = 0; i < NUM_CL; i++) {
        if (g_clocks[i].read && !strcmp(name, g_clocks[i].name)) {
            return &g_clocks[i];
        }
    }
    return NULL;
}