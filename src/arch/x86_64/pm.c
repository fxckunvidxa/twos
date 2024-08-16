#include <arch/x86_64/bootinfo.h>
#include <kstring.h>
#include <panic.h>
#include <pm.h>
#include <printk.h>
#include <vm.h>

#define PHYS_MAP_MAX 0x100000000
#define PHYS_MAP_SIZE (PHYS_MAP_MAX / PAGE_SIZE / 8)

static u8 phys_map[PHYS_MAP_SIZE];
static uptr phys_end = 0;

static usize used_pages = 0;

void pm_init(struct efi_memory_info *mi)
{
    memset(phys_map, 0xff, PHYS_MAP_SIZE);
    for (usize i = 0; i < mi->no_entries; i++) {
        struct efi_memory_descriptor *d = (uptr)mi->map + i * mi->desc_size;
        if (d->physical_start < PHYS_MAP_MAX &&
            (d->type == EfiConventionalMemory || d->type == EfiLoaderCode || d->type == EfiBootServicesCode)) {
            if (d->physical_start + d->number_of_pages * PAGE_SIZE > phys_end) {
                phys_end = d->physical_start + d->number_of_pages * PAGE_SIZE;
            }
            for (usize idx = PG_IDX(d->physical_start);
                 idx < PG_IDX(d->physical_start) + d->number_of_pages && idx < PG_IDX(PHYS_MAP_MAX); idx++) {
                phys_map[idx / 8] &= ~(1 << (idx % 8));
            }
        }
    }
}

uptr pm_alloc_frame()
{
    used_pages++;
    for (usize i = 0; i < (phys_end > PHYS_MAP_MAX ? PHYS_MAP_SIZE : phys_end / PAGE_SIZE / 8); i++) {
        if (phys_map[i] != 0xff) {
            for (u8 bit = 0; bit < 8; bit++) {
                if ((phys_map[i] & (1 << bit)) == 0) {
                    phys_map[i] |= (1 << bit);
                    return (i * 8 + bit) * PAGE_SIZE;
                }
            }
        }
    }
    panic("pm: out of memory");
}

uptr pm_alloc_zero_frame()
{
    uptr fr = pm_alloc_frame();
    memset(VIRT(fr), 0, PAGE_SIZE);
    return fr;
}

void pm_free_frame(uptr addr)
{
    if (addr < phys_end && addr < PHYS_MAP_MAX && (addr & 0xfff) == 0) {
        phys_map[PG_IDX(addr) >> 3] &= ~(1 << (PG_IDX(addr) & 7));
        used_pages--;
    }
}

void pm_print_used_pages()
{
    printk("Memory usage: %llu KiB", used_pages * 4);
}