#include <arch/x86_64/asm.h>
#include <pm.h>
#include <printk.h>
#include <vm.h>

static union pme *current_pml4;

union pme *vm_get_cr3()
{
    return current_pml4;
}

void vm_set_cr3(union pme *pml4)
{
    current_pml4 = PHYS(pml4);
    asm volatile("mov %0, %%cr3" ::"r"(current_pml4) : "memory");
}

static uptr vm_fnd_free(const union pme *pml4, uptr start, uptr end, usize num)
{
    usize found = 0;
    uptr startfnd = NULL;
    pml4 = VIRT(pml4);
    const struct va_bits *va_st = &start, *va_end = &end;
    for (usize pml4i = va_st->pml4; pml4i <= va_end->pml4; pml4i++) {
        if (!pml4[pml4i].data.prsnt) {
            if (!startfnd) {
                startfnd = pml4i == va_st->pml4 ? start : ((0xffffULL << 48) | (pml4i << 39));
            }

            if (pml4i == va_st->pml4) {
                found += (1 << 27) - ((start >> 12) & ((1 << 27) - 1));
            } else if (pml4i == va_end->pml4) {
                found += ((end >> 12) & ((1 << 27) - 1));
            } else {
                found += (1 << 27);
            }

            if (found >= num) {
                return startfnd;
            }
            continue;
        }
        const union pme *pdpt = VIRT(pml4[pml4i].data.addr << 12);
        usize pdpti;
        for (usize pdptist = pdpti = ((pml4i == va_st->pml4) ? va_st->pdpt : 0),
                   pdptiend = ((pml4i == va_end->pml4) ? va_end->pdpt : 511);
             pdpti <= pdptiend; pdpti++) {
            if (!pdpt[pdpti].data.prsnt) {
                if (!startfnd) {
                    startfnd = (pml4i == va_st->pml4 && pdpti == pdptist)
                                   ? start
                                   : ((0xffffULL << 48) | (pml4i << 39) | (pdpti << 30));
                }

                if (pml4i == va_st->pml4 && pdpti == pdptist) {
                    found += (1 << 18) - ((start >> 12) & ((1 << 18) - 1));
                } else if (pml4i == va_end->pml4 && pdpti == pdptiend) {
                    found += ((end >> 12) & ((1 << 18) - 1));
                } else {
                    found += (1 << 18);
                }

                if (found >= num) {
                    return startfnd;
                }
                continue;
            }
            const union pme *pd = VIRT(pdpt[pdpti].data.addr << 12);
            usize pdist;
            for (usize pdi = (pdist = ((pdpti == pdptist && pml4i == va_st->pml4) ? va_st->pd : 0)),
                       pdiend = ((pdpti == pdptiend && pml4i == va_end->pml4) ? va_end->pd : 511);
                 pdi <= pdiend; pdi++) {
                if (!pd[pdi].data.prsnt) {
                    if (!startfnd) {
                        startfnd = (pml4i == va_st->pml4 && pdpti == pdptist && pdi == pdist)
                                       ? start
                                       : ((0xffffULL << 48) | (pml4i << 39) | (pdpti << 30) | (pdi << 21));
                    }

                    if (pml4i == va_st->pml4 && pdpti == pdptist && pdi == pdist) {
                        found += (1 << 9) - ((start >> 12) & ((1 << 9) - 1));
                    } else if (pml4i == va_end->pml4 && pdpti == pdptiend && pdi == pdiend) {
                        found += ((end >> 12) & ((1 << 9) - 1));
                    } else {
                        found += (1 << 9);
                    }

                    if (found >= num) {
                        return startfnd;
                    }
                    continue;
                }
                if (pd[pdi].data.size) {
                    found = 0;
                    startfnd = NULL;
                    continue;
                }
                const union pme *pt = VIRT(pd[pdi].data.addr << 12);
                for (usize pti = ((pdi == pdist && pdpti == pdptist && pml4i == va_st->pml4) ? va_st->pt : 0),
                           ptiend = ((pdi == pdiend && pdpti == pdptiend && pml4i == va_end->pml4) ? va_end->pt : 511);
                     pti <= ptiend; pti++) {
                    if (!pt[pti].data.prsnt) {
                        if (!startfnd) {
                            startfnd = (0xffffULL << 48) | (pml4i << 39) | (pdpti << 30) | (pdi << 21) | (pti << 12);
                        }
                        found++;
                        if (found >= num) {
                            return startfnd;
                        }
                    } else {
                        found = 0;
                        startfnd = NULL;
                    }
                }
            }
        }
    }
    return NULL;
}

static union pme *vm_get_page(union pme *pml4, uptr virt, usize flags)
{
    pml4 = VIRT(pml4);
    const struct va_bits *va = &virt;
    if (!pml4[va->pml4].data.prsnt) {
        if (flags & VM_GP_ALLOC) {
            pml4[va->pml4].raw = pm_alloc_zero_frame() | PG_PRSNT | PG_WR | PG_USR;
        } else {
            return NULL;
        }
    }

    union pme *pdpt = VIRT(pml4[va->pml4].data.addr << 12);

    if (!pdpt[va->pdpt].data.prsnt) {
        if (flags & VM_GP_ALLOC) {
            pdpt[va->pdpt].raw = pm_alloc_zero_frame() | PG_PRSNT | PG_WR | PG_USR;
        } else {
            return NULL;
        }
    }

    union pme *pd = VIRT(pdpt[va->pdpt].data.addr << 12);

    if (pd[va->pd].data.prsnt && pd[va->pd].data.size) {
        if (flags & VM_GP_ONLY_4K) {
            return NULL;
        } else {
            return &pd[va->pd];
        }
    }

    if (!pd[va->pd].data.prsnt) {
        if ((flags & VM_GP_ALLOC) && !(flags & VM_GP_2M)) {
            pd[va->pd].raw = pm_alloc_zero_frame() | PG_PRSNT | PG_WR | PG_USR;
        } else if (flags & VM_GP_2M) {
            return &pd[va->pd];
        } else {
            return NULL;
        }
    } else {
        if (!pd[va->pd].data.size && (flags & VM_GP_2M)) {
            return NULL;
        }
    }

    union pme *pt = VIRT(pd[va->pd].data.addr << 12);

    return &pt[va->pt];
}

static void vm_set_page_flags(union pme *pg, usize flags)
{
    pg->data.write = flags & VM_PG_WR ? 1 : 0;
    pg->data.user = flags & VM_PG_USR ? 1 : 0;
    pg->data.nocache = flags & VM_PG_NC ? 1 : 0;
    pg->data.wrthr = flags & VM_PG_WT ? 1 : 0;
}

void vm_set_flags(uptr virt, usize num, usize flags)
{
    union pme *pg = vm_get_page(current_pml4, virt, 0);
    if (pg) {
        vm_set_page_flags(pg, flags);
        if (pg->data.size) {
            for (usize va = virt + PAGE_SIZE_2M; va < virt + num * PAGE_SIZE_2M; va += PAGE_SIZE_2M) {
                pg = vm_get_page(current_pml4, va, VM_GP_2M);
                if (!pg) {
                    return;
                }
                vm_set_page_flags(pg, flags);
            }
        } else {
            for (usize va = virt + PAGE_SIZE; va < virt + num * PAGE_SIZE; va += PAGE_SIZE) {
                pg = vm_get_page(current_pml4, va, VM_GP_ONLY_4K);
                if (!pg) {
                    return;
                }
                vm_set_page_flags(pg, flags);
            }
        }
    }
}

void vm_map_page(union pme *pml4t, uptr phys, uptr virt, usize flags)
{
    union pme *pg = vm_get_page(pml4t, virt, VM_GP_ALLOC | (flags & VM_PG_2M ? VM_GP_2M : 0));

    if (pg) {
        pg->raw = phys | PG_PRSNT;
        pg->data.frame = flags & VM_PG_FR ? 1 : 0;
        pg->data.size = flags & VM_PG_2M ? 1 : 0;
        vm_set_page_flags(pg, flags);
    }

    invlpg(virt);
}

static void vm_unmap_page(union pme *pml4t, uptr virt)
{
    pml4t = VIRT(pml4t);
    const struct va_bits *va = &virt;

    if (!pml4t[va->pml4].data.prsnt) {
        return;
    }

    union pme *pdpt = VIRT(pml4t[va->pml4].data.addr << 12);

    if (!pdpt[va->pdpt].data.prsnt) {
        return;
    }

    union pme *pd = VIRT(pdpt[va->pdpt].data.addr << 12);

    if (!pd[va->pd].data.prsnt) {
        return;
    }

    if (!pd[va->pd].data.size) {
        union pme *pt = VIRT(pd[va->pd].data.addr << 12);

        if (pt[va->pt].data.frame) {
            pm_free_frame(pt[va->pt].data.addr << 12);
        }

        pt[va->pt].raw = 0;

        for (usize i = 0; i < 512; i++) {
            if (pt[i].data.prsnt) {
                goto end;
            }
        }

        pm_free_frame(PHYS(pt));
    }

    pd[va->pd].raw = 0;

    for (usize i = 0; i < 512; i++) {
        if (pd[i].data.prsnt) {
            goto end;
        }
    }

    pm_free_frame(PHYS(pd));
    pdpt[va->pdpt].raw = 0;

    for (usize i = 0; i < 512; i++) {
        if (pdpt[i].data.prsnt) {
            goto end;
        }
    }

    pm_free_frame(PHYS(pdpt));
    pml4t[va->pml4].raw = 0;

end:
    invlpg(virt);
}

uptr vm_mmap(enum vm_map_region reg, uptr start, uptr *ph_arr, usize num)
{
    uptr end;

    switch (reg) {
    case VM_MR_KHEAP:
        end = (uptr)-0x1000;
        if (!start || start < KHEAP_START || start > end) {
            start = KHEAP_START;
        }
        break;
    case VM_MR_KSTACK:
        end = VM_MR_KSTACK_END;
        start = VM_MR_KSTACK_START;
        break;

    default:
        return NULL;
    }

    start = vm_fnd_free(current_pml4, start, end, num);

    if (!start) {
        return NULL;
    }

    if (!ph_arr) {
        for (usize va = start; va < start + num * PAGE_SIZE; va += PAGE_SIZE) {
            vm_map_page(current_pml4, pm_alloc_frame(), va, VM_PG_FR | VM_PG_WR);
        }
    } else {
        for (usize pg = 0; pg < num; pg++) {
            vm_map_page(current_pml4, ph_arr[pg], start + pg * PAGE_SIZE, VM_PG_WR);
        }
    }

    return start;
}

bool vm_munmap(uptr start, usize num)
{
    union pme *pg = vm_get_page(current_pml4, start, 0);
    if (!pg) {
        return false;
    }

    if (pg->data.size) {
        for (uptr va = start; va < start + num * 0x200000; va += 0x200000) {
            vm_unmap_page(current_pml4, va);
        }
    } else {
        for (uptr va = start; va < start + num * PAGE_SIZE; va += PAGE_SIZE) {
            vm_unmap_page(current_pml4, va);
        }
    }
    return true;
}

uptr vm_map_mmio(uptr phys_start, usize num)
{
    for (usize i = 0; i < num; i++) {
        vm_map_page(current_pml4, phys_start + i * PAGE_SIZE, VIRT(phys_start + i * PAGE_SIZE), VM_PG_WR | VM_PG_WT | VM_PG_NC);
    }
    return VIRT(phys_start);
}